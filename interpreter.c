#include "interpreter.h"
#include "common.h"
#include "lexer.h"
#include "parser.h"
#include "symbol.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Result *newResult(void *data, int nodeType, size_t dataSize) {
  Result *res = (Result *)malloc(sizeof(Result));
  res->NodeType = nodeType;
  if (nodeType == NODE_STRING_LITERAL) {
    res->result = strdup((char *)data);
  } else {
    res->result = (double *)data;
  }
  return res;
}

void freeResult(Result *res) {
  if (res) {
    if (res->result) {
      free(res->result);
    }
    free(res);
  }
}

char *trimQuotes(char *str) {
  size_t len = strlen(str);
  if (str[0] == '"' && str[len - 1] == '"') {
    str[len - 1] = '\0'; // Remove ending quote
    return str + 1;      // Skip starting quote
  }
  return str;
}

static char *inferDatatype(AstNode *node) {
  if (strcmp(node->identifier.type, "iden_num") == 0) {
    return "number";
  }
  if (strcmp(node->identifier.type, "iden_str") == 0) {
    return "string";
  }
  printf("unknown result type\n");
  exit(EXIT_FAILURE);
}

static char *getDataType(Result *res) {
  switch (res->NodeType) {
  case NODE_NUMBER:
    return "number";
  case NODE_STRING_LITERAL:
    return "string";
  }
  printf("unknown result type\n");
  exit(EXIT_FAILURE);
}
static double convertStrToDouble(char *s) {
  double val;
  sscanf(s, "%lf", &val);
  return val;
}

Result *EvalAst(AstNode *node, Parser *p) {
  switch (node->type) {

  case NODE_RETURN: {
    Result *res = EvalAst(node->expr, p);
    return res;
  }
  case NODE_NUMBER: {
    Result *res = newResult(&node->number, NODE_NUMBER, sizeof(node->number));
    return res;
  }

  case NODE_FUNCTION_PARAM: {
    insertSymbol(p->table, node->identifier.name, node->identifier.type, NULL,
                 node->isParam);
    break;
  }

  case NODE_FUNCTION: {
    SymbolTableEntry *sym =
        lookupFnSymbol(p->table, node->function.defination.name);
    if (sym) {
      printParseError(p, "Funcion %s is already decelared",
                      node->function.defination.name);
      exit(EXIT_FAILURE);
    }

    insertFnSymbol(
        p->table, node->function.defination.name,
        node->function.defination.returnType, node->function.defination.params,
        node->function.defination.body, node->function.defination.paramsCount);

    for (int i = 0; i < node->function.defination.paramsCount; i++) {
      Result *res = EvalAst(node->function.defination.params[i], p);
      if (res) {
        free(res->result);
        free(res);
      }
    }
    break;
  }
  case NODE_FUNCTION_CALL: {
    SymbolTableEntry *sym = lookupFnSymbol(p->table, node->function.call.name);
    if (!sym) {
      printParseError(p, "undeclared function %s was called\n",
                      node->function.call.name);
      exit(EXIT_FAILURE);
    }
    // update the func symbol table params with the value of args
    for (int i = 0; i < node->function.call.argsCount; i++) {
      Result *res = EvalAst(node->function.call.args[i], p);
      char *paramType = sym->function.parameters[i]->identifier.type;

      if (strcmp(paramType, getDataType(res)) != 0) {
        printParseError(p, "expected argument of type %s but got %s", paramType,
                        getDataType(res));
        exit(EXIT_FAILURE);
      }

      updateSymbolTableValue(p->table,
                             sym->function.parameters[i]->identifier.name, res,
                             sym->function.parameters[i]->identifier.type);
    }
    Result *value = EvalAst(sym->function.functionBody, p);
    if (strcmp(sym->function.returnType, getDataType(value)) != 0) {
      printParseError(p,
                      " cannot return %s "
                      "from the function with the return type of %s ",
                      getDataType(value), sym->function.returnType);
      exit(EXIT_FAILURE);
    }
    return value;
  }

  case NODE_BINARY_OP: {
    Result *left = EvalAst(node->binaryOp.left, p);
    Result *right = EvalAst(node->binaryOp.right, p);

    if (left->NodeType == NODE_NUMBER && right->NodeType == NODE_NUMBER) {
      double leftVal = *(double *)(left->result);
      double rightVal = *(double *)(right->result);
      Result *res = NULL;
      switch (node->binaryOp.op) {

      case TOKEN_PLUS:
        return newResult(&(double){leftVal + rightVal}, NODE_NUMBER,
                         sizeof(double));
      case TOKEN_MINUS:
        return newResult(&(double){leftVal - rightVal}, NODE_NUMBER,
                         sizeof(double));
      case TOKEN_MODULO:
        return newResult(&(int){(int)leftVal % (int)rightVal}, NODE_NUMBER,
                         sizeof(double));

      case TOKEN_MULTIPLY:
        return newResult(&(double){leftVal * rightVal}, NODE_NUMBER,
                         sizeof(double));
      case TOKEN_DIVIDE:
        return newResult(&(double){leftVal / rightVal}, NODE_NUMBER,
                         sizeof(double));
      case TOKEN_DB_EQUAL:
        return newResult(&(double){leftVal == rightVal}, NODE_NUMBER,
                         sizeof(double));
      case TOKEN_EQ_GREATER:
        return newResult(&(double){leftVal >= rightVal}, NODE_NUMBER,
                         sizeof(double));
      case TOKEN_EQ_LESSER:
        return newResult(&(double){leftVal <= rightVal}, NODE_NUMBER,
                         sizeof(double));
      case TOKEN_LESSER:
        return newResult(&(double){leftVal < rightVal}, NODE_NUMBER,
                         sizeof(double));
      case TOKEN_GREATER:
        return newResult(&(double){leftVal > rightVal}, NODE_NUMBER,
                         sizeof(double));
      case TOKEN_EQ_NOT:
        return newResult(&(double){leftVal != rightVal}, NODE_NUMBER,
                         sizeof(double));
      case TOKEN_AND:
        return newResult(&(double){(leftVal && rightVal)}, NODE_NUMBER,
                         sizeof(double));
      case TOKEN_OR:
        return newResult(&(double){(leftVal || rightVal)}, NODE_NUMBER,
                         sizeof(double));
      default:
        printf("Error: Unknown binary operator\n");
        exit(EXIT_FAILURE);
      }
    } else if (left->NodeType == NODE_STRING_LITERAL &&
               right->NodeType == NODE_STRING_LITERAL) {
      char *leftStr = trimQuotes((char *)left->result);
      char *rightStr = trimQuotes((char *)right->result);

      size_t len1 = strlen(leftStr);
      size_t len2 = strlen(rightStr);

      char *concatenated = (char *)malloc(len1 + len2 + 1);
      if (concatenated == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
      }

      strcpy(concatenated, leftStr);
      strcat(concatenated, rightStr);

      free(left->result);
      free(right->result);
      free(left);
      free(right);

      return newResult(concatenated, NODE_STRING_LITERAL, len1 + len2 + 1);
    } else {

      printf("Error: cannot do ( %s ) operations between %s and %s \n",
             tokenNames[node->binaryOp.op], getDataType(left),
             getDataType(right));
      exit(EXIT_FAILURE);
    }
  }

  case NODE_UNARY_OP: {
    Result *right = EvalAst(node->unaryOp.right, p);
    if (right->NodeType == NODE_NUMBER) {
      double rightVal = *(double *)(right->result);
      switch (node->unaryOp.op) {
      case TOKEN_NOT: {
        Result *node =
            newResult(&(double){!rightVal}, NODE_NUMBER, sizeof(double));
        free(right->result);
        free(right);
        return node;
      }
      default:
        printf("Error: Unknown unary operator\n");
        exit(EXIT_FAILURE);
      }
    } else {
      printf("Error: Invalid type for unary operation\n");
      exit(EXIT_FAILURE);
    }
  }

  case NODE_IDENTIFIER_VALUE: {
    SymbolTableEntry *var =
        lookupSymbol(p->table, node->identifier.name, node->isParam);
    if (!var) {
      printParseError(p, "%s is not decleared\n", node->identifier.name);
      exit(EXIT_FAILURE);
    }

    if (strcmp(var->type, "string") == 0) {
      return newResult(var->value, NODE_STRING_LITERAL, sizeof(var->value));
    }
    return newResult(var->value, NODE_NUMBER, sizeof(var->value));
  }

  case NODE_IDENTIFIER_MUTATION: {
    SymbolTableEntry *var =
        lookupSymbol(p->table, node->identifier.name, node->isParam);

    if (!var) {
      printParseError(p, "%s is not decleared\n", node->identifier.name);
      exit(EXIT_FAILURE);
    }

    Result *res = EvalAst(node->identifier.value, p);
    char *type = getDataType(res);

    if (strcmp(type, var->type) != 0) {
      printParseError(p, "cannot assign type of %s to type of %s", type,
                      var->type);
      free(res->result);
      free(res);
      exit(EXIT_FAILURE);
    }

    updateSymbolTableValue(p->table, node->identifier.name, res, var->type);
    break;
  }

  case NODE_IDENTIFIER_ASSIGNMENT: {
    SymbolTableEntry *var = lookupSymbol(p->table, node->identifier.name, 0);
    if (var) {
      printParseError(p, "cannot redeclare variable %s is already decleared\n",
                      node->identifier.name);
      exit(EXIT_FAILURE);
    }

    Result *res = EvalAst(node->identifier.value, p);

    char *inferedDataType = getDataType(res);

    if (strcmp(node->identifier.type, inferedDataType) != 0) {
      printParseError(p, "cannot assign typeof %s to %s\n", inferedDataType,
                      node->identifier.type);
      free(res->result);
      free(res);
      exit(EXIT_FAILURE);
    }
    insertSymbol(p->table, node->identifier.name, node->identifier.type, res,
                 node->isParam);
    return res;
  }

  case NODE_IDENTIFIER_DECLERATION: {
    SymbolTableEntry *var =
        lookupSymbol(p->table, node->identifier.name, node->isParam);
    if (var) {
      printParseError(p, "cannot redeclare variable %s is already decleared\n",
                      node->identifier.name);
      exit(EXIT_FAILURE);
    }

    insertSymbol(p->table, node->identifier.name, node->identifier.type, NULL,
                 node->isParam);

    break;
  }

  case NODE_STRING_LITERAL: {
    char *str = strdup(node->stringLiteral.value);
    Result *res = newResult(str, NODE_STRING_LITERAL, strlen(str) + 1);
    free(str);
    return res;
  }

  case NODE_BLOCK: {
    enterScope(p->table);
    for (int i = 0; i < node->block.statementCount; i++) {
      AstNode *ast = node->block.statements[i];
      Result *res = EvalAst(ast, p);
      if (ast->type == NODE_RETURN) {
        exitScope(p->table);
        return res;
      }
      freeResult(res);
    };
    exitScope(p->table);
    return NULL;
  }

  case NODE_IF_ELSE: {
    Result *conditionResult = EvalAst(node->ifElseBlock.condition, p);
    if (conditionResult->NodeType != NODE_NUMBER) {
      printParseError(
          p, "Error: Condition in if-else must be a number (interpreted as "
             "boolean)\n");
      exit(EXIT_FAILURE);
    }

    double conditionValue = *(double *)(conditionResult->result);

    if (conditionValue) {
      Result *val = EvalAst(node->ifElseBlock.ifBlock, p);
      if (val && val->NodeType == NODE_RETURN) {
        return val;
      } else if (val) {
        free(val->result);
        free(val);
      }
    } else if (node->ifElseBlock.elseBlock != NULL) {
      Result *val = EvalAst(node->ifElseBlock.elseBlock, p);
      if (val && val->NodeType == NODE_RETURN) {
        return val;
      } else if (val) {
        free(val->result);
        free(val);
      }
    }
    break;
  }

  case NODE_FUNCTION_READ_IN: {
    int initalBufferSize = 100;
    int currentBufferSize = 0;
    char *buffer = (char *)malloc(sizeof(char) * initalBufferSize);
    while (1) {
      if (currentBufferSize >= initalBufferSize - 1) {
        initalBufferSize += 100;
        buffer = realloc(buffer, initalBufferSize);
      }

      char ch = getchar();
      if (ch == '\n') {
        break;
      }
      buffer[currentBufferSize] = ch;
      currentBufferSize++;
    }

    buffer[currentBufferSize] = '\0';
    Result *res = NULL;

    if (strcmp(node->read.type, "string") == 0) {
      res = newResult(buffer, NODE_STRING_LITERAL, strlen(buffer));
    } else if (strcmp(node->read.type, "number") == 0) {
      double numberValue;
      sscanf(buffer, "%lf", &numberValue);

      // Allocate memory for the double value
      double *ptr = (double *)malloc(sizeof(double));
      if (ptr == NULL) {
        perror("Failed to allocate memory");
        exit(EXIT_FAILURE);
      }
      *ptr = numberValue; // Store the parsed value in the allocated memory

      res = newResult((void *)ptr, NODE_NUMBER, strlen(buffer));
    }
    free(buffer);
    return res;
  }

  case NODE_FUNCTION_PRINT: {
    for (int i = 0; i < node->print.statementCount; i++) {
      Result *res = EvalAst(node->print.statments[i], p);
      if (res) {
        if (res->NodeType == NODE_STRING_LITERAL) {
          printf("%s", trimQuotes((char *)res->result));
        } else {
          printf("%lf", *(double *)res->result);
        }
      } else {
        printf("(NULL)\n");
      }
    }
    printf("\n");
    break;
  }
  default:
    printf("Error: Unexpected node type %d\n", node->type);
    exit(EXIT_FAILURE);
  }

  return NULL; // Ensure you return a valid pointer
}

void freeAst(AstNode *node) {
  if (!node) {
    return;
  }

  switch (node->type) {
  case NODE_IDENTIFIER_MUTATION:
  case NODE_IDENTIFIER_ASSIGNMENT:
  case NODE_IDENTIFIER_VALUE:
    if (node->identifier.name) {
      free(node->identifier.name);
      node->identifier.name = NULL;
    }
    if (node->identifier.type) {
      free(node->identifier.type);
      node->identifier.type = NULL;
    }
    if (node->identifier.value) {
      freeAst(node->identifier.value);
      node->identifier.value = NULL;
    }
    break;
  case NODE_IDENTIFIER_DECLERATION:
    if (node->identifier.type) {
      free(node->identifier.type);
      node->identifier.type = NULL;
    }
    if (node->identifier.name) {
      free(node->identifier.name);
      node->identifier.name = NULL;
    }
    break;
  case NODE_FUNCTION:
    if (node->function.defination.returnType) {
      free(node->function.defination.returnType);
      node->function.defination.returnType = NULL;
    }
    if (node->function.defination.name) {
      free(node->function.defination.name);
      node->function.defination.name = NULL;
    }
    if (node->function.defination.body) {
      freeAst(node->function.defination.body);
      node->function.defination.body = NULL;
    }
    for (int i = 0; i < node->function.defination.paramsCount; i++) {
      if (node->function.defination.params[i]) {
        freeAst(node->function.defination.params[i]);
        node->function.defination.params[i] = NULL;
      }
    }
    if (node->function.defination.params) {
      free(node->function.defination.params);
      node->function.defination.params = NULL;
    }
    break;
  case NODE_NUMBER:
    // No dynamic memory to free here
    break;
  case NODE_STRING_LITERAL:
    if (node->stringLiteral.value) {
      free(node->stringLiteral.value);
      node->stringLiteral.value = NULL;
    }
    break;
  case NODE_BLOCK:
    for (int i = 0; i < node->block.statementCount; i++) {
      if (node->block.statements[i]) {
        freeAst(node->block.statements[i]);
        node->block.statements[i] = NULL;
      }
    }
    if (node->block.statements) {
      free(node->block.statements);
      node->block.statements = NULL;
    }
    break;
  case NODE_RETURN:
    if (node->expr) {
      freeAst(node->expr);
      node->expr = NULL;
    }
    break;
  case NODE_UNARY_OP:
    if (node->unaryOp.right) {
      freeAst(node->unaryOp.right);
      node->unaryOp.right = NULL;
    }
    break;
  case NODE_BINARY_OP:
    if (node->binaryOp.left) {
      freeAst(node->binaryOp.left);
      node->binaryOp.left = NULL;
    }
    if (node->binaryOp.right) {
      freeAst(node->binaryOp.right);
      node->binaryOp.right = NULL;
    }
    break;
  case NODE_IF_ELSE:
    if (node->ifElseBlock.ifBlock) {
      freeAst(node->ifElseBlock.ifBlock);
      node->ifElseBlock.ifBlock = NULL;
    }
    if (node->ifElseBlock.elseBlock) {
      freeAst(node->ifElseBlock.elseBlock);
      node->ifElseBlock.elseBlock = NULL;
    }
    if (node->ifElseBlock.condition) {
      freeAst(node->ifElseBlock.condition);
      node->ifElseBlock.condition = NULL;
    }
    break;
  case NODE_FUNCTION_PARAM:
    if (node->identifier.name) {
      free(node->identifier.name);
      node->identifier.name = NULL;
    }
    if (node->identifier.type) {
      free(node->identifier.type);
      node->identifier.type = NULL;
    }
    break;

  case NODE_FUNCTION_PRINT: {
    for (int i = 0; i < node->print.statementCount; i++) {
      freeAst(node->print.statments[i]);
    }
    free(node->print.statments);
    break;
  }

  case NODE_FUNCTION_CALL:
    if (node->function.call.name) {
      free(node->function.call.name);
      node->function.call.name = NULL;
    }
    for (int i = 0; i < node->function.call.argsCount; i++) {
      if (node->function.call.args[i]) {
        freeAst(node->function.call.args[i]);
        node->function.call.args[i] = NULL;
      }
    }
    if (node->function.call.args) {
      free(node->function.call.args);
      node->function.call.args = NULL;
    }
    break;
  }

  // Free the node itself
  free(node);
}

AstNode *parseAst(Parser *p) {
  if (parserIsAtEnd(p)) {

    return NULL; // No more tokens to parse
  }

  // skips if the token is comment
  if (p->current->type == TOKEN_COMMENT) {
    p->idx++;
    p->current = p->tokens[p->idx];
  }

  Token *tkn = p->current;
  switch (tkn->type) {
  case TOKEN_PRINT: {
    AstNode *ast = parsePrint(p);
    consume(TOKEN_SEMI_COLON, p);
    return ast;
  }
  case TOKEN_IDEN: {
    // Handle variable declaration or assignment
    AstNode *ast = varDecleration(p);
    consume(TOKEN_SEMI_COLON, p);
    return ast;
  }
  case TOKEN_FN: {
    AstNode *ast = parseFunction(p);
    return ast;
  }
  case TOKEN_STRING: {
    AstNode *ast = string(p);
    // Assuming statements end with a semicolon
    return ast;
  } // Handle string literals (if needed)
  case TOKEN_NUMBER: {
    // Handle number literals (if needed)
    AstNode *numberNode = factor(p);
    // Assuming statements end with a semicolon
    return numberNode;
  }

  case TOKEN_IF:
    // Handle if-else statements
    return ifElseParser(p);

  case TOKEN_LCURLY: {
    AstNode *st = parseBlockStmt(p);
    return st;
  }
  case TOKEN_RETURN: {
    AstNode *ast = parseReturn(p);
    consume(TOKEN_SEMI_COLON, p);
    return ast;
  }
  case TOKEN_EOF:
    return NULL;

  default:
    printParseError(p, "Unexpected token %s falling through the condition\n",
                    tokenNames[tkn->type]);
    exit(EXIT_FAILURE);
  }
}

void freeSymbolTable(SymbolTable *table) {
  for (int i = 0; i < table->size; i++) {
    SymbolTableEntry *entry = &table->entries[i];
    if (entry->isFn) {
      if (entry->function.name) {
        free(entry->function.name);
      }
      if (entry->function.returnType) {
        free(entry->function.returnType);
      }
      // Handle function parameters and body if necessary
    } else {
      if (entry->type) {
        free(entry->type);
      }
      if (entry->symbol) {
        free(entry->symbol);
      }
    }
  }
  free(table->entries);
  free(table);
}
