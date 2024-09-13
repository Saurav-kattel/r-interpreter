#include "interpreter.h"
#include "common.h"
#include "lexer.h"
#include "parser.h"
#include "symbol.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>

#define MAX_STRING_LENGTH 255

char *trimQuotes(char *str) {
  size_t len = strlen(str);
  if (str[0] == '"' && str[len - 1] == '"') {
    str[len - 1] = '\0'; // Remove ending quote
    return str + 1;      // Skip starting quote
  }
  return str;
}

void printEvalError(Loc loc, const char *s, ...) {

  va_list args;
  va_start(args, s);

  // Print the filename and line number with color
  printf(MAGENTA "%s" RESET, loc.file_name);
  printf(GREEN "::" RESET);
  printf(BLUE "%d" RESET, loc.row); // Use token's row for line number
  printf(GREEN "::" RESET);
  printf(RED "Error-> " RESET);
  fflush(stdout);

  // Use token's row for line number
  fflush(stdout);

  // Print the error message with color
  fprintf(stderr, YELLOW); // Set text color to yellow for the error message
  vfprintf(stderr, s, args);
  fprintf(stderr, RESET); // Reset text color

  va_end(args);

  printf("\n");
}

static double convertStrToDouble(char *s) {
  double val;
  sscanf(s, "%lf", &val);
  return val;
}

void printSymbolError(SymbolError err, Loc loc, char *name, char *type) {
  switch (err) {
  case SYMBOL_MEM_ERROR:
    printEvalError(loc, "failed allocating memory");
    exit(EXIT_FAILURE);
  case SYMBOL_TYPE_ERROR:
    printEvalError(loc, "TypeError: cannot assign type of %s", type);
    exit(EXIT_FAILURE);
  case SYMBOL_DUPLICATE_ERROR:
    printEvalError(loc, "ReferenceError: %s is already declared\n", name);
    exit(EXIT_FAILURE);
  case SYMBOL_NOT_FOUND_ERROR:
    printEvalError(loc, "ReferenceError: %s is not declared", name);
    exit(EXIT_FAILURE);
  case SYMBOL_ERROR_NONE:
    exit(EXIT_FAILURE);
  }
}

Result newResult(void *data, int nodeType) {
  Result res = {0};
  res.NodeType = nodeType;
  res.result = data;
  return res;
}

void freeResult(Result *res) {
  if (res && res->NodeType == NODE_STRING_LITERAL) {
    if (res->result) {

      free(res->result);
    }
  } else if (res && res->isAllocated) {
    free(res->result);
  }
}

void insertFixedStringArray(AstNode *node, Parser *p, int size) {
  char **values = (char **)calloc(size, sizeof(char *));

  for (int i = 0; i < node->array.actualSize; i++) {
    if (node->array.elements[i]) {
      Result res = EvalAst(node->array.elements[i], p);
      values[i] = strdup((char *)res.result);
      freeResult(&res);
    }
  }

  SymbolError err = insertArray(p->ctx, node->array.name, node->array.type,
                                size, values, node->array.isFixed);

  if (err != SYMBOL_ERROR_NONE) {
    printSymbolError(err, node->loc, node->array.name, node->array.type);
    exit(EXIT_FAILURE);
  }
}

void insertFixedNumberArray(AstNode *node, Parser *p, int size) {
  double *values = (double *)calloc(size, sizeof(double));
  for (int i = 0; i < size; i++) {
    if (node->array.elements[i]) {
      Result res = EvalAst(node->array.elements[i], p);
      values[i] = *(double *)res.result;
    }
  }

  SymbolError err = insertArray(p->ctx, node->array.name, node->array.type,
                                size, values, node->array.isFixed);
  if (err != SYMBOL_ERROR_NONE) {
    printSymbolError(err, node->loc, node->array.name, node->array.type);
    exit(EXIT_FAILURE);
  }
}

void handleFixedArrayInsert(AstNode *node, Parser *p) {

  Result res = EvalAst(node->array.arraySize, p);
  double arraySize = *(double *)res.result;

  int size = (int)arraySize;
  if (strcmp(node->array.type, "string") == 0) {
    insertFixedStringArray(node, p, size);
  } else if (strcmp(node->array.type, "number") == 0) {
    insertFixedNumberArray(node, p, size);
  }
}

void freeEntry(SymbolTableEntry *entry) {

  if (entry->isParam) {
    free(entry);
  }
}

void printResult(Result *res) {
  if (!res) {
    return;
  }
  if (res->NodeType == NODE_STRING_LITERAL) {
    printf(YELLOW "%s" RESET, trimQuotes((char *)res->result));
  } else {
    printf(MAGENTA "%.0lf" RESET, *(double *)res->result);
  }
}

void printArray(SymbolTableEntry *entry, Parser *p) {
  printf(GREEN "[ " RESET);
  if (strcmp(entry->type, "string") == 0) {

    char **elements = (char **)entry->value;
    for (int i = 0; i < entry->arraySize; i++) {
      if (elements[i]) {
        printf(YELLOW "%s" RESET, elements[i]);
        if (i < entry->arraySize - 1) {
          printf(", ");
        }
      }
    }
    printf(GREEN "]" RESET);
    return;
  }

  double *elements = (double *)entry->value;
  for (int i = 0; i < entry->arraySize; i++) {
    printf(MAGENTA "%.0lf" RESET, elements[i]);
    if (i < entry->arraySize - 1) {
      printf(", ");
    }
  }
  printf(GREEN "]" RESET);

  return;
}

void insertDynamicStringArray(AstNode *node, Parser *p) {
  int size = 0;
  int capacity = 10;
  char **values = (char **)calloc(capacity, sizeof(char *));

  while (1) {
    if (node->array.elements[size]) {
      Result res = EvalAst(node->array.elements[size], p);
      if (res.NodeType == NODE_NONE) {
        break;
      }

      if (size >= capacity) {
        char **newValues =
            (char **)realloc(values, (sizeof(char *) * (capacity *= 2)));
        if (newValues == NULL) {
          // Handle realloc failure, free allocated memory, and exit
          for (int i = 0; i < size; i++) {
            free(values[i]);
          }
          free(values);
          fprintf(stderr,
                  "Memory allocation failed during dynamic array resizing.\n");
          exit(EXIT_FAILURE);
        }
        values = newValues;
      }

      values[size] = strdup((char *)res.result);
      freeResult(&res);
      (size)++;
    } else {
      break;
    }
  }

  SymbolError err = insertArray(p->ctx, node->array.name, node->array.type,
                                size, values, node->array.isFixed);
  if (err != SYMBOL_ERROR_NONE) {
    printSymbolError(err, node->loc, node->array.name, node->array.type);
    exit(EXIT_FAILURE);
  }
}

void handleBound(AstNode *node, SymbolTableEntry *var, int index) {

  if (var->isFixed) {
    if (var->arraySize <= index) {
      printEvalError(node->loc,
                     "index out of bound canot access %d index. Array `%s` is "
                     "only size of %d\n",
                     var->arraySize, var->symbol, index);
      exit(EXIT_FAILURE);
    }
  } else {
    if (var->arraySize <= index) {
      if (!(index <= var->arraySize)) {
        printEvalError(node->loc, "cannot access  index %d ", index);
        exit(EXIT_FAILURE);
      }
      if (strcmp(var->type, "string") == 0) {
        var->value =
            (char **)realloc(var->value, sizeof(char *) * (var->arraySize + 1));
      } else {
        var->value = (double *)realloc(var->value,
                                       sizeof(double) * (var->arraySize + 1));
      }
    }
  }
}

void insertDynamicNumberArray(AstNode *node, Parser *p) {

  int size = 0;
  int capacity = 10;

  double *values = (double *)malloc(sizeof(double) * capacity);
  while (1) {
    if (node->array.elements[size]) {
      Result res = EvalAst(node->array.elements[size], p);
      if (res.NodeType == NODE_NONE) {
        break;
      }

      if (size >= capacity) {
        values = (double *)realloc(values, (sizeof(double) * (capacity *= 2)));
      }
      values[size] = *(double *)res.result;
      freeResult(&res);
      size++;
    } else {
      break;
    }
  }

  SymbolError err = insertArray(p->ctx, node->array.name, node->array.type,
                                size, values, node->array.isFixed);
  if (err != SYMBOL_ERROR_NONE) {
    printSymbolError(err, node->loc, node->array.name, node->array.type);
    exit(EXIT_FAILURE);
  }
}

void handleDynamicArrayInsert(AstNode *node, Parser *p) {

  if (strcmp(node->array.type, "string") == 0) {
    insertDynamicStringArray(node, p);
  } else {
    insertDynamicNumberArray(node, p);
  }
}

static char *inferDatatype(AstNode *node) {
  if (strcmp(node->identifier.type, "iden_num") == 0) {
    return "number";
  }
  if (strcmp(node->identifier.type, "iden_str") == 0) {
    return "string";
  }
  printEvalError(node->loc, "unknown result type\n");
  exit(EXIT_FAILURE);
}

static char *getDataType(Result res) {
  switch (res.NodeType) {
  case NODE_NUMBER:
    return "number";
  case NODE_STRING_LITERAL:
    return "string";
  }
  printf("unknown result type\n");
  exit(EXIT_FAILURE);
}

// eval ast function
Result EvalAst(AstNode *node, Parser *p) {
  switch (node->type) {
  case NODE_RETURN: {
    Result res = EvalAst(node->expr, p);
    res.isReturn = 1;
    return res;
  }

  case NODE_NUMBER: {
    Result res = newResult(&node->number, NODE_NUMBER);
    return res;
  }

  case NODE_FUNCTION: {
    SymbolTableEntry *sym = lookupSymbol(p->ctx, node->function.defination.name,
                                         SYMBOL_KIND_FUNCTION);

    if (sym) {
      printEvalError(node->loc, "%s is already defined",
                     node->function.defination.name);
      exit(EXIT_FAILURE);
    }

    insertFunctionSymbol(p->ctx, node->function.defination.name,
                         node->function.defination.returnType,
                         node->function.defination.paramsCount,
                         node->function.defination.params, SYMBOL_KIND_FUNCTION,
                         node->function.defination.body, p->level);

    break;
  }

  case NODE_FUNCTION_CALL: {

    SymbolTableEntry *sym =
        lookupSymbol(p->ctx, node->function.call.name, SYMBOL_KIND_FUNCTION);

    if (!sym) {
      printEvalError(node->loc, "undeclared function %s was called\n",
                     node->function.call.name);
      exit(EXIT_FAILURE);
    }
    // update the func symbol table params with the value of args

    for (int i = 0; i < node->function.call.argsCount; i++) {
      Result res = EvalAst(node->function.call.args[i], p);

      char *paramType = sym->function.params[i]->type;

      if (strcmp(paramType, getDataType(res)) != 0) {
        printEvalError(node->loc, "expected argument of type %s  but got %s",
                       paramType, getDataType(res));
        exit(EXIT_FAILURE);
      }
      updateParamWithArgs(sym, i, &res);
      freeResult(&res);
    }

    Result value = EvalAst(sym->function.body, p);

    if (sym->type && (value.NodeType == NODE_NONE)) {
      printEvalError(node->loc, "expected return type to be %s but got void\n",
                     sym->type);
      exit(EXIT_FAILURE);
    }

    if (strcmp(sym->type, getDataType(value)) != 0) {
      printEvalError(
          node->loc,
          " cannot return %s from the function with  the return type of %s",
          getDataType(value), sym->type);
      exit(EXIT_FAILURE);
    }

    return value;
  }

  case NODE_BINARY_OP: {
    Result left = EvalAst(node->binaryOp.left, p);
    Result right = EvalAst(node->binaryOp.right, p);

    if (left.NodeType == NODE_NONE || right.NodeType == NODE_NONE) {
      printEvalError(node->loc, "Error: Null result encountered\n");
      exit(EXIT_FAILURE);
    }

    Result res = {0};

    if (left.NodeType == NODE_NUMBER && right.NodeType == NODE_NUMBER) {
      double leftVal = *(double *)(left.result);
      double rightVal = *(double *)(right.result);

      // Allocate memory dynamically for the result
      double *val = malloc(sizeof(double));
      if (val == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
      }

      switch (node->binaryOp.op) {
      case TOKEN_PLUS: {
        *val = leftVal + rightVal;
        res = newResult(val, NODE_NUMBER); // Pass dynamically allocated memory
        break;
      }
      case TOKEN_MINUS: {
        *val = leftVal - rightVal;
        res = newResult(val, NODE_NUMBER);
        break;
      }
      case TOKEN_MODULO: {
        *val = (int)leftVal % (int)rightVal;
        res = newResult(val, NODE_NUMBER);
        break;
      }
      case TOKEN_MULTIPLY: {
        *val = leftVal * rightVal;
        res = newResult(val, NODE_NUMBER);
        break;
      }
      case TOKEN_DIVIDE: {
        *val = leftVal / rightVal;
        res = newResult(val, NODE_NUMBER);
        break;
      }
      case TOKEN_DB_EQUAL: {
        *val = (double)(leftVal == rightVal);
        res = newResult(val, NODE_NUMBER);
        break;
      }
      case TOKEN_EQ_GREATER: {
        *val = (double)(leftVal >= rightVal);
        res = newResult(val, NODE_NUMBER);
        break;
      }
      case TOKEN_EQ_LESSER: {
        *val = (double)(leftVal <= rightVal);
        res = newResult(val, NODE_NUMBER);
        break;
      }
      case TOKEN_LESSER: {
        *val = (double)(leftVal < rightVal);
        res = newResult(val, NODE_NUMBER);
        break;
      }
      case TOKEN_GREATER: {
        *val = (double)(leftVal > rightVal);
        res = newResult(val, NODE_NUMBER);
        break;
      }
      case TOKEN_EQ_NOT: {
        *val = (double)(leftVal != rightVal);
        res = newResult(val, NODE_NUMBER);
        break;
      }
      case TOKEN_AND: {
        *val = (double)(leftVal && rightVal);
        res = newResult(val, NODE_NUMBER);
        break;
      }
      case TOKEN_OR: {
        *val = (double)(leftVal || rightVal);
        res = newResult(val, NODE_NUMBER);
        break;
      }
      default:
        printEvalError(node->loc, "Error: Unknown binary operator\n");
        exit(EXIT_FAILURE);
      }

      // Free the results used, as needed
      freeResult(&left);
      freeResult(&right);
      res.isAllocated = 1;
      return res;
    } else if (left.NodeType == NODE_STRING_LITERAL &&
               right.NodeType == NODE_STRING_LITERAL) {
      char *leftStr = trimQuotes((char *)left.result);
      char *rightStr = trimQuotes((char *)right.result);

      size_t len1 = strlen(leftStr);
      size_t len2 = strlen(rightStr);

      char *concatenated = (char *)malloc(len1 + len2 + 1);
      if (concatenated == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
      }

      strcpy(concatenated, leftStr);
      strcat(concatenated, rightStr);

      freeResult(&left); // Free left and right strings after use
      freeResult(&right);

      return newResult(
          concatenated,
          NODE_STRING_LITERAL); // Pass dynamically allocated memory
    } else {
      printEvalError(
          node->loc, "Error: cannot do ( %s ) operations between %s and %s\n",
          tokenNames[node->binaryOp.op], getDataType(left), getDataType(right));
      exit(EXIT_FAILURE);
    }
  }

  case NODE_UNARY_OP: {
    Result right = EvalAst(node->unaryOp.right, p);
    if (right.NodeType == NODE_NUMBER) {
      double rightVal = *(double *)(right.result);
      switch (node->unaryOp.op) {
      case TOKEN_NOT: {
        Result node = newResult(&(double){!rightVal}, NODE_NUMBER);
        return node;
      }
      default:
        printEvalError(node->loc, "Error: Unknown unary operator\n");
        exit(EXIT_FAILURE);
      }
    } else {
      printEvalError(node->loc, "Error: Invalid type for unary operation\n");
      exit(EXIT_FAILURE);
    }
  }

  case NODE_IDENTIFIER_VALUE: {

    SymbolTableEntry *var =
        lookupSymbol(p->ctx, node->identifier.name, SYMBOL_KIND_VARIABLES);

    if (!var) {
      printEvalError(node->loc, "%s is not decleared\n", node->identifier.name);
      exit(EXIT_FAILURE);
    }

    if (strcmp(var->type, "string") == 0) {

      Result res = newResult((char *)var->value, NODE_STRING_LITERAL);
      freeEntry(var);
      return res;
    }
    Result res = newResult(var->value, NODE_NUMBER);
    freeEntry(var);
    return res;
  }

  case NODE_IDENTIFIER_MUTATION: {
    SymbolTableEntry *var =
        lookupSymbol(p->ctx, node->identifier.name, SYMBOL_KIND_VARIABLES);
    if (!var) {
      printEvalError(node->loc, "%s is not decleared\n", node->identifier.name);
      exit(EXIT_FAILURE);
    }

    Result res = EvalAst(node->identifier.value, p);

    char *type = getDataType(res);

    if (strcmp(type, var->type) != 0) {
      printEvalError(node->loc, "cannot assign type of %s to type of %s", type,
                     var->type);
      freeResult(&res);
      exit(EXIT_FAILURE);
    }

    SymbolError err = updateSymbolTableValue(var, &res);
    if (err != SYMBOL_ERROR_NONE) {
      printSymbolError(err, node->loc, node->identifier.name,
                       node->identifier.type);
      exit(EXIT_FAILURE);
    }

    break;
  }

  case NODE_IDENTIFIER_ASSIGNMENT: {

    SymbolTableEntry *var =
        lookupSymbol(p->ctx, node->identifier.name, SYMBOL_KIND_VARIABLES);

    if (var && !var->isParam) {
      printEvalError(node->loc, "cannot redeclare %s\n", node->identifier.name);
      exit(EXIT_FAILURE);
    }

    Result res = EvalAst(node->identifier.value, p);

    char *inferedDataType = getDataType(res);

    if (strcmp(node->identifier.type, inferedDataType) != 0) {
      printEvalError(node->loc, "cannot assign typeof %s to %s\n",
                     inferedDataType, node->identifier.type);
      free(res.result);
      exit(EXIT_FAILURE);
    }

    SymbolError err =
        insertSymbol(p->ctx, node->identifier.type, node->identifier.name, &res,
                     SYMBOL_KIND_VARIABLES, p->level);

    if (err != SYMBOL_ERROR_NONE) {
      printSymbolError(err, node->loc, node->identifier.name, inferedDataType);
    }
    break;
  }

  case NODE_IDENTIFIER_DECLERATION: {
    SymbolTableEntry *var =
        lookupSymbol(p->ctx, node->identifier.name, SYMBOL_KIND_VARIABLES);
    if (var) {
      printEvalError(node->loc,
                     "cannot redeclare variable %s is already decleared\n",
                     node->identifier.name);
      exit(EXIT_FAILURE);
    }

    SymbolError err =
        insertSymbol(p->ctx, node->identifier.type, node->identifier.name, NULL,
                     SYMBOL_KIND_VARIABLES, p->level);

    if (err != SYMBOL_ERROR_NONE) {
      printSymbolError(err, node->loc, node->identifier.name,
                       node->identifier.type);
      exit(EXIT_FAILURE);
    }

    break;
  }

  case NODE_STRING_LITERAL: {
    char *str = strdup(node->stringLiteral.value);
    Result res = newResult(str, NODE_STRING_LITERAL);
    return res;
  }

  case NODE_BLOCK: {
    p->level++;
    enterScope(p->ctx);
    for (int i = 0; i < node->block.statementCount; i++) {
      AstNode *ast = node->block.statements[i];

      Result result = EvalAst(ast, p);

      if (result.NodeType != NODE_NONE && (result.isReturn || result.isBreak)) {
        exitScope(p->ctx);
        p->level--;
        return result;
      } else if (result.NodeType != NODE_NONE &&
                 (ast->type == NODE_FUNCTION_READ_IN || result.isContinue)) {
        return result;
      }

      freeResult(&result);
    };
    exitScope(p->ctx);
    p->level--;
    break;
  }

  case NODE_IF_ELSE: {
    Result conditionResult = EvalAst(node->ifElseBlock.condition, p);
    if (conditionResult.NodeType != NODE_NUMBER) {
      printEvalError(
          node->loc,
          "Error: Condition in if-else must be a number (interpreted as "
          "boolean)\n");
      exit(EXIT_FAILURE);
    }

    double conditionValue = *(double *)(conditionResult.result);

    freeResult(&conditionResult);

    if (conditionValue) {
      Result val = EvalAst(node->ifElseBlock.ifBlock, p);
      if (val.NodeType != NODE_NONE) {
        return val;
      }
    } else if (node->ifElseBlock.elseBlock != NULL) {
      Result val = EvalAst(node->ifElseBlock.elseBlock, p);
      if (val.NodeType != NODE_NONE) {
        return val;
      }
    }
    break;
  }

  case NODE_FUNCTION_READ_IN: {
    int initialBufferSize = 100;
    int currentBufferSize = 0;

    char *buffer = (char *)malloc(sizeof(char) * initialBufferSize);
    if (buffer == NULL) {
      perror("Failed to allocate memory for buffer");
      exit(EXIT_FAILURE);
    }

    while (1) {
      if (currentBufferSize >= initialBufferSize - 1) {
        initialBufferSize += 100;
        char *newBuffer =
            (char *)realloc(buffer, sizeof(char) * initialBufferSize);
        if (newBuffer == NULL) {
          perror("Failed to reallocate memory");
          free(buffer); // Free original buffer before exiting
          exit(EXIT_FAILURE);
        }
        buffer = newBuffer;
      }

      char ch = getchar();
      if (ch == '\n') {
        break;
      }
      buffer[currentBufferSize] = ch;
      currentBufferSize++;
    }

    buffer[currentBufferSize] = '\0';

    Result res = {0};

    if (strcmp(node->read.type, "string") == 0) {
      res = newResult(buffer, NODE_STRING_LITERAL); // Include null terminator
    } else if (strcmp(node->read.type, "number") == 0) {
      double numberValue;
      sscanf(buffer, "%lf", &numberValue);

      double *ptr = (double *)malloc(sizeof(double));
      if (ptr == NULL) {
        perror("Failed to allocate memory for number");
        free(buffer); // Free buffer before exiting
        exit(EXIT_FAILURE);
      }
      *ptr = numberValue;
      res = newResult((void *)ptr, NODE_NUMBER);
    }

    return res;
  }

  case NODE_FUNCTION_PRINT: {

    for (int i = 0; i < node->print.statementCount; i++) {
      AstNode *stmt = node->print.statments[i];
      switch (stmt->type) {
      case NODE_IDENTIFIER_VALUE: {

        SymbolTableEntry *entry =
            lookupSymbol(p->ctx, stmt->identifier.name, SYMBOL_KIND_VARIABLES);

        if (!entry) {
          printf("null no entry found");
          break;
        }

        if (entry->isArray) {
          freeEntry(entry);
          printArray(entry, p);
          break;
        }

        Result res = EvalAst(stmt, p);
        printResult(&res);
        freeEntry(entry);
        break;
      }

      case NODE_STRING_LITERAL: {
        Result res = EvalAst(stmt, p);
        printResult(&res);
        freeResult(&res);
        break;
      }

      case NODE_NUMBER: {
        Result res = EvalAst(stmt, p);
        printResult(&res);
        break;
      }

      case NODE_ARRAY_ELEMENT_ACCESS: {
        Result res = EvalAst(stmt, p);
        if (res.NodeType == NODE_STRING_LITERAL) {
          printf(YELLOW "%s" RESET, trimQuotes((char *)res.result));
        } else {
          printf(MAGENTA "%0.lf" RESET, *(double *)res.result);
        }
        break;
      }
      default:
        printf("node type is %s\n", nodeTypeNames[stmt->type]);
      }
    }
    printf("\n");
    break;
  }

  case NODE_ARRAY_ELEMENT_ACCESS: {
    SymbolTableEntry *var =
        lookupSymbol(p->ctx, node->arrayElm.name, SYMBOL_KIND_VARIABLES);

    if (!var || !var->isArray) {
      printEvalError(node->loc, " %s is not decleared\n", node->arrayElm.name);
      exit(EXIT_FAILURE);
    }
    Result res = EvalAst(node->arrayElm.index, p);
    if (res.NodeType == NODE_NONE) {
      printEvalError(node->loc, "invalid index");
      exit(EXIT_FAILURE);
    }
    double idx = *(double *)res.result;
    int index = (int)idx;
    if (index >= var->arraySize) {
      printEvalError(node->loc,
                     "index out of bound. index 0 cannot be accessed", index);
      exit(EXIT_FAILURE);
    }

    if (strcmp(var->type, "string") == 0) {
      char *value = ((char **)var->value)[index];
      return newResult(value, NODE_STRING_LITERAL);
    } else {
      double value = ((double *)var->value)[index];
      return newResult(&value, NODE_NUMBER);
    }
  }

  // needs refactoring
  case NODE_ARRAY_INIT: {
    SymbolTableEntry *var =
        lookupSymbol(p->ctx, node->array.name, SYMBOL_KIND_VARIABLES);

    if (var) {
      printEvalError(node->loc, "cannot redeclare %s is already decleared\n",
                     node->array.name);
      exit(EXIT_FAILURE);
    }

    if (node->array.isFixed) {
      handleFixedArrayInsert(node, p);
    } else {
      handleDynamicArrayInsert(node, p);
    }

    break;
  }

    /*
  case NODE_ARRAY_DECLARATION: {
    SymbolTableEntry *var =
        lookupSymbol(p->table, node->array.name, node->isParam);

    if (var) {
        printEvalError(node->loc, "cannot redeclare  %s is already
    decleared\n", node->identifier.name); exit(EXIT_FAILURE);
    }

    if (node->array.arraySize) {
      Result *result = EvalAst(node->array.arraySize, p);
      double size = *(double *)result.result;
      if (strcmp(node->array.type, "string") == 0) {
        insertStrArraySymbol(p->table, node->array.name, node->array.type,
  size, NULL, node->array.isFixed, node->array.actualSize); } else {
        insertNumArraySymbol(p->table, node->array.name, node->array.type,
  size, NULL, node->array.isFixed);
      }
    } else {
      if (strcmp(node->array.type, "string") == 0) {
        insertStrArraySymbol(p->table, node->array.name, node->array.type,
  0, NULL, node->array.isFixed, node->array.actualSize); } else {
        insertNumArraySymbol(p->table, node->array.name, node->array.type,
  0, NULL, node->array.isFixed);
      }
    }
    break;
  }
  */
  case NODE_ARRAY_ELEMENT_ASSIGN: {

    SymbolTableEntry *var =
        lookupSymbol(p->ctx, node->arrayElm.name, SYMBOL_KIND_VARIABLES);

    if (!var) {
      printEvalError(node->loc, "array %s is not decleared\n",
                     node->arrayElm.name);
      exit(EXIT_FAILURE);
    }

    if (!var->isArray) {
      printEvalError(node->loc, " %s is not an array \n", node->arrayElm.name);
      exit(EXIT_FAILURE);
    }

    Result res = EvalAst(node->arrayElm.index, p);
    double resIndex = *(double *)res.result;
    int index = (int)resIndex;

    // checks and handles bound of array if fixed else increases the arr size
    if (var->isFixed) {
      handleBound(node, var, index);
    }

    res = EvalAst(node->arrayElm.value, p);
    char *type = getDataType(res);

    if (strcmp(type, var->type) != 0) {
      printEvalError(node->loc, "cannot assign type of %s to %s", type,
                     var->type);
      exit(EXIT_FAILURE);
    }

    if (strcmp(var->type, "string") == 0) {
      if (!var->value) {
        var->value = (char **)malloc(sizeof(char *) * var->arraySize);
      }
      ((char **)var->value)[index] = strdup((char *)res.result);

      if (!var->isFixed) {
        var->arraySize++;
      }
      free(res.result);
      break;
    }

    if (!var->value) {
      var->value = (double *)malloc(sizeof(double) * var->arraySize);
    }

    ((double *)var->value)[index] = *(double *)res.result;
    if (!var->isFixed) {
      var->arraySize++;
    }

    break;
  }

  case NODE_BREAK: {
    Result res = newResult(NULL, NODE_BREAK);
    res.isBreak = 1;
    return res;
  }

  case NODE_CONTNUE: {
    Result res = newResult(NULL, NODE_CONTNUE);
    res.isContinue = 1;
    return res;
  }

  case NODE_WHILE_LOOP: {
    p->level++;
    enterScope(p->ctx);
    Result result = EvalAst(node->whileLoop.condition, p);
    double condition = 0;
    if (result.NodeType != NODE_NONE) { // Check if result is non-null
      if (result.result) {
        condition = *(double *)result.result;
      }
      free(result.result); // Free the dynamically allocated memory pointed to
      // by result.result, if applicable
    };

    while (condition) {
      Result blockRes = EvalAst(node->whileLoop.body, p);

      if (blockRes.NodeType != NODE_NONE && blockRes.isReturn) {
        return blockRes;
      }

      if (blockRes.NodeType && blockRes.isContinue) {
        freeResult(&blockRes);
        result = EvalAst(node->whileLoop.condition, p);
        condition = *(double *)result.result;
        freeResult(&result);
        continue;
      }

      // Handle break: exit the loop
      if (blockRes.NodeType != NODE_NONE && blockRes.isBreak) {
        freeResult(&blockRes);
        break;
      }

      freeResult(&blockRes);

      blockRes = EvalAst(node->whileLoop.condition, p);
      condition = *(double *)blockRes.result;
      freeResult(&blockRes);
    }
    exitScope(p->ctx);
    p->level--;
    break;
  }

  case NODE_FOR_LOOP: {
    p->level++;
    enterScope(p->ctx);
    EvalAst(node->loopFor.initializer, p);

    Result result = EvalAst(node->loopFor.condition, p);
    double condition = *(double *)result.result;

    free(result.result); // result.result  is a allocted pointer

    while (condition) {
      // evaluates the body
      Result blockRes = EvalAst(node->loopFor.loopBody, p);

      if (blockRes.NodeType && blockRes.isReturn) {
        return blockRes;
      }

      // free  the result if result is valid and is not a type of return
      if (blockRes.NodeType && blockRes.isContinue) {
        EvalAst(node->loopFor.icrDcr, p);

        Result newResult = EvalAst(node->loopFor.condition, p);
        condition = *(double *)newResult.result;
        free(newResult.result);
        continue; // Skip the rest of the loop body
      }

      // Handle break: exit the loop
      if (blockRes.NodeType && blockRes.isBreak) {
        exitScope(p->ctx);
        break; // Exit the loop
      }

      // handle the icrDcr statement
      Result res = EvalAst(node->loopFor.icrDcr, p);
      Result newResult = EvalAst(node->loopFor.condition, p);
      condition = *(double *)newResult.result;
      freeResult(&newResult);
      freeResult(&res);
    }
    exitScope(p->ctx);
    p->level--;
    break;
  }

  default:
    printEvalError(node->loc, "Error: Unexpected node type %s\n",
                   nodeTypeNames[node->type]);
    exit(EXIT_FAILURE);
  }

  return newResult(NULL, NODE_NONE); // Ensure you return a valid pointer
}

void freeAst(AstNode *node) {
  if (node == NULL) {
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
  case NODE_FOR_LOOP: {

    if (node->loopFor.loopBody) {
      freeAst(node->loopFor.loopBody);
    }

    if (node->loopFor.condition) {
      freeAst(node->loopFor.condition);
    }
    if (node->loopFor.icrDcr) {
      freeAst(node->loopFor.icrDcr);
    }
    if (node->loopFor.initializer) {
      freeAst(node->loopFor.initializer);
    }
    break;
  }
  case NODE_WHILE_LOOP: {
    if (node->whileLoop.body) {
      freeAst(node->whileLoop.body);
    }
    if (node->whileLoop.condition) {
      freeAst(node->whileLoop.condition);
    }
    break;
  }
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

    free(node->function.defination.params);

    break;
  case NODE_NUMBER:
  case NODE_CONTNUE:
  case NODE_BREAK:
    // No dynamic memory to free here
    break;
  case NODE_STRING_LITERAL:
    if (node->stringLiteral.value) {
      free(node->stringLiteral.value);
      node->stringLiteral.value = NULL;
    }
    break;

  case NODE_BLOCK: {
    for (int i = 0; i < node->block.statementCount; i++) {
      if (node->block.statements[i]) {
        freeAst(node->block.statements[i]);
        node->block.statements[i] = NULL;
      }
    }
    free(node->block.statements);
    break;
  }
  case NODE_ARRAY_INIT:
  case NODE_ARRAY_DECLARATION: {
    if (node->array.name) {
      free(node->array.name);
    }

    if (node->array.type) {
      free(node->array.type);
    }
    if (node->array.elements) {
      int i = 0;
      while (node->array.elements[i]) {
        freeAst(node->array.elements[i]);
        i++;
      }
      free(node->array.elements);
    }
    if (node->array.arraySize) {
      free(node->array.arraySize);
    }

    break;
  }
  case NODE_ARRAY_ELEMENT_ASSIGN: {
    if (node->arrayElm.name) {
      free(node->arrayElm.name);
    }

    if (node->arrayElm.value) {
      freeAst(node->arrayElm.value);
    }
    if (node->arrayElm.index) {
      freeAst(node->arrayElm.index);
    }
    break;
  }
  case NODE_ARRAY_ELEMENT_ACCESS: {
    if (node->arrayElm.index) {
      freeAst(node->arrayElm.index);
    }
    if (node->arrayElm.value) {
      free(node->arrayElm.value);
    }
    if (node->arrayElm.name) {
      free(node->arrayElm.name);
    }
    break;
  }

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

  case NODE_FUNCTION_READ_IN:
    if (node->read.type) {
      free(node->read.type);
    }
    break;
  }

  // Free the node itself
  free(node);
  node = NULL;
}

AstNode *parseAst(Parser *p) {
  if (parserIsAtEnd(p)) {

    return NULL; // No more tokens to parse
  }

  // skips if the token is comment

  Token *tkn = p->current;
  switch (tkn->type) {
  case TOKEN_COMMENT: {
    consume(TOKEN_COMMENT, p);
    break;
  }
  case TOKEN_PRINT: {
    AstNode *ast = parsePrint(p);
    consume(TOKEN_SEMI_COLON, p);
    return ast;
  }

  case TOKEN_IDEN: {
    Token *nextToken = p->tokens[p->idx + 1];

    switch (nextToken->type) {
    case TOKEN_LPAREN: {
      AstNode *ast = functionCall(p);
      consume(TOKEN_SEMI_COLON, p);
      return ast;
    }
    case TOKEN_LSQUARE: {
      AstNode *ast = parseArray(p);
      consume(TOKEN_SEMI_COLON, p);
      return ast;
    }
    default:
      break;
    }
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

  case TOKEN_FOR: {
    return parseForLoop(p);
  }

  case TOKEN_CONTINUE: {
    AstNode *node = parseContinueNode(p);
    consume(TOKEN_SEMI_COLON, p);
    return node;
  }

  case TOKEN_BREAK: {
    AstNode *node = parseBreakNode(p);
    consume(TOKEN_SEMI_COLON, p);
    return node;
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
  case TOKEN_WHILE: {
    return parseWhileNode(p);
  }
  case TOKEN_EOF:
    return NULL;

  default:
    printEvalError(*p->current->loc, "Unexpected token ' %s ' \n",
                   tokenNames[tkn->type]);
    exit(EXIT_FAILURE);
  }
  return NULL;
}
