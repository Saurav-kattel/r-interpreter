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
static double convertStrToDouble(char *s) {
  double val;
  sscanf(s, "%lf", &val);
  return val;
}

Result *EvalAst(AstNode *node) {
  switch (node->type) {
  case NODE_NUMBER:
    return newResult(&node->number, NODE_NUMBER);

  case NODE_BINARY_OP: {
    Result *left = EvalAst(node->binaryOp.left);
    Result *right = EvalAst(node->binaryOp.right);

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
    } else {
      printf("Error: Invalid types for binary operation\n");
      exit(EXIT_FAILURE);
    }
  }

  case NODE_UNARY_OP: {
    Result *right = EvalAst(node->unaryOp.right);

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

  case NODE_IDENTIFIER: {

    Symbol *sym = lookupSymbol(node->identifier.table, node->identifier.name);
    if (sym == NULL) {
      printf("Error: Variable '%s' not found\n", node->identifier.name);
      exit(EXIT_FAILURE);
    }

    if (strcmp(sym->dataType, "number") == 0) {
      return newResult(&(double){convertStrToDouble(sym->value)}, NODE_NUMBER);
    } else if (strcmp(sym->dataType, "string") == 0) {
      return newResult(sym->value, NODE_STRING_LITERAL);
    }
  }

  case NODE_STRING_LITERAL: {
    char *str =
        strdup(node->stringLiteral.value); // Make sure to duplicate the string
    return newResult(str, NODE_STRING_LITERAL);
  }

  default:
    printf("Error: Unhandled node type %d\n", node->type);
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
  }
  free(node);
}

AstNode *parseAst(Parser *p) {
  if (p->current->type == TOKEN_IDEN) {
    AstNode *ast = varDecleration(p);
    consume(TOKEN_SEMI_COLON, p);
    return ast;
  } else if (p->current->type == TOKEN_STRING) {
    AstNode *ast = string(p);
    consume(TOKEN_SEMI_COLON, p);
    return ast;
  }

  AstNode *ast = logical(p);
  consume(TOKEN_SEMI_COLON, p);
  return ast;
}
