#include "lexer.h"
#include "parser.h"
#include "symbol.h"

#include <stdio.h>
#include <stdlib.h>
double EvalAst(AstNode *node) {
  switch (node->type) {
  case NODE_NUMBER:
    return node->number;

  case NODE_BINARY_OP: {
    double left = EvalAst(node->binaryOp.left);
    double right = EvalAst(node->binaryOp.right);

    switch (node->binaryOp.op) {
    case TOKEN_PLUS:
      return left + right;
    case TOKEN_MULTIPLY:
      return left * right;
    case TOKEN_MINUS:
      return left - right;
    case TOKEN_DIVIDE:
      return left / right;
    case TOKEN_DB_EQUAL:
      return left == right;
    case TOKEN_EQ_GREATER:
      return left >= right;
    case TOKEN_EQ_LESSER:
      return left <= right;
    case TOKEN_LESSER:
      return left < right;
    case TOKEN_GREATER:
      return left > right;
    case TOKEN_EQ_NOT:
      return left != right;
    case TOKEN_AND:
      return left && right;
    case TOKEN_OR:
      return left || right;
    default:
      printf("unknown case\n");
      break;
    }
  }
  case NODE_UNARY_OP: {

    double right = EvalAst(node->unaryOp.right);
    switch (node->unaryOp.op) {
    case TOKEN_NOT:
      return !right;
    }
  }
  case NODE_IDENTIFIER:
    break;
  case NODE_STRING_LITERAL:
    break;
  default:
    printf("unhandled token \n");
    exit(1);
  }

  return 0.0;
}

void freeAst(AstNode *node) {
  if (node->type == NODE_BINARY_OP) {
    freeAst(node->binaryOp.left);
    freeAst(node->binaryOp.right);
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
