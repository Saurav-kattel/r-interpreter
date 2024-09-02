#include "interpreter.h"
#include "common.h"
#include "lexer.h"
#include "parser.h"
#include "symbol.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Result *newResult(void *data, int nodeType) {
  Result *res = (Result *)malloc(sizeof(Result));
  res->NodeType = nodeType;
  res->result = data;
  return res;
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
  case NODE_NUMBER:
    return newResult(&node->number, NODE_NUMBER);
  case NODE_FUNCTION_PARAM: {
    for (int i = 0; i < node->function.defination.paramsCount; i++) {
      EvalAst(node->function.defination.params[i], p);
    }
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
      updateSymbolTableValue(
          p->table, sym->function.parameters[i]->identifier.name, res->result,
          sym->function.parameters[i]->identifier.type);
      EvalAst(sym->function.functionBody, p);
    }
    break;
  }

  case NODE_BINARY_OP: {
    Result *left = EvalAst(node->binaryOp.left, p);
    Result *right = EvalAst(node->binaryOp.right, p);

    if (left->NodeType == NODE_NUMBER && right->NodeType == NODE_NUMBER) {
      double leftVal = *(double *)(left->result);
      double rightVal = *(double *)(right->result);
      switch (node->binaryOp.op) {

      case TOKEN_PLUS:
        return newResult(&(double){leftVal + rightVal}, NODE_NUMBER);
      case TOKEN_MINUS:
        return newResult(&(double){leftVal - rightVal}, NODE_NUMBER);
      case TOKEN_MULTIPLY:
        return newResult(&(double){leftVal * rightVal}, NODE_NUMBER);
      case TOKEN_DIVIDE:
        return newResult(&(double){leftVal / rightVal}, NODE_NUMBER);
      case TOKEN_DB_EQUAL:
        return newResult(&(double){leftVal == rightVal}, NODE_NUMBER);
      case TOKEN_EQ_GREATER:
        return newResult(&(double){leftVal >= rightVal}, NODE_NUMBER);
      case TOKEN_EQ_LESSER:
        return newResult(&(double){leftVal <= rightVal}, NODE_NUMBER);
      case TOKEN_LESSER:
        return newResult(&(double){leftVal < rightVal}, NODE_NUMBER);
      case TOKEN_GREATER:
        return newResult(&(double){leftVal > rightVal}, NODE_NUMBER);
      case TOKEN_EQ_NOT:
        return newResult(&(double){leftVal != rightVal}, NODE_NUMBER);
      case TOKEN_AND:
        return newResult(&(double){(leftVal && rightVal)}, NODE_NUMBER);
      case TOKEN_OR:
        return newResult(&(double){(leftVal || rightVal)}, NODE_NUMBER);

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

      char *concatenated =
          (char *)malloc(len1 + len2 + 1); // +1 for null terminator
      if (concatenated == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
      }

      strcpy(concatenated, leftStr);
      strcat(concatenated, rightStr);

      printf("here\n");

      return newResult(concatenated, NODE_STRING_LITERAL);
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
      case TOKEN_NOT:
        return newResult(&(double){!rightVal}, NODE_NUMBER);
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
    SymbolTableEntry *var = lookupSymbol(p->table, node->identifier.name);
    if (!var) {
      printParseError(p, "%s is not decleared\n", node->identifier.name);
      exit(EXIT_FAILURE);
    }
    if (strcmp(var->type, "string") == 0) {
      return newResult(var->value, NODE_STRING_LITERAL);
    }
    return newResult(var->value, NODE_NUMBER);
  }

  case NODE_IDENTIFIER_MUTATION: {
    SymbolTableEntry *var = lookupSymbol(p->table, node->identifier.name);
    if (!var) {
      printParseError(p, "%s is not decleared\n", node->identifier.name);
      exit(EXIT_FAILURE);
    }
    Result *res = EvalAst(node->identifier.value, p);
    char *type = getDataType(res);
    if (strcmp(type, var->type) != 0) {
      printParseError(p, "cannot assign type of %s to type of %s", type,
                      var->type);
      exit(EXIT_FAILURE);
    }
    updateSymbolTableValue(p->table, node->identifier.name, res->result,
                           var->type);

    break;
  }

  case NODE_IDENTIFIER_ASSIGNMENT: {
    SymbolTableEntry *var = lookupSymbol(p->table, node->identifier.name);
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
      exit(EXIT_FAILURE);
    }
    insertSymbol(p->table, node->identifier.name, node->identifier.type,
                 res->result);

    break;
  }

  case NODE_IDENTIFIER_DECLERATION: {
    SymbolTableEntry *var = lookupSymbol(p->table, node->identifier.name);
    if (var) {
      printParseError(p, "cannot redeclare variable %s is already decleared\n",
                      node->identifier.name);
      exit(EXIT_FAILURE);
    }

    insertSymbol(p->table, node->identifier.name, node->identifier.type, NULL);

    break;
  }

  case NODE_STRING_LITERAL: {
    char *str = strdup(node->stringLiteral.value);
    return newResult(str, NODE_STRING_LITERAL);
  }

  case NODE_BLOCK: {
    enterScope(p->table);
    for (int i = 0; i < node->block.statementCount; i++) {
      AstNode *ast = node->block.statements[i];
      EvalAst(ast, p);
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
      if (val) {
        free(val->result);
        free(val);
      }
      return val;
    } else if (node->ifElseBlock.elseBlock != NULL) {
      Result *val = EvalAst(node->ifElseBlock.elseBlock, p);
      if (val) {
        free(val->result);
        free(val);
      }
      return val;
    }
    break;
  }

  default:
    printf("Error: Unexpected node type %d\n", node->type);
    exit(EXIT_FAILURE);
  }

  return NULL; // Ensure you return a valid pointer
}

void freeAst(AstNode *node) {
  if (node->type == NODE_BINARY_OP) {
    freeAst(node->binaryOp.left);
    freeAst(node->binaryOp.right);
  } else if (node->type == NODE_UNARY_OP) {
    freeAst(node->unaryOp.right);
  } else if (node->type == NODE_STRING_LITERAL) {
    free(node->stringLiteral.value);
  } else if (node->type == NODE_IDENTIFIER_ASSIGNMENT) {
    // Do not free identifier.name or symbol.value here if they are managed
    // elsewhere
  }
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
  case TOKEN_EOF:
    return NULL;
  default:
    printParseError(p, "Unexpected token %s falling through the condition\n",
                    tokenNames[tkn->type]);
    exit(EXIT_FAILURE);
  }
}
