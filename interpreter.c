#include "interpreter.h"
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

Result *EvalAst(AstNode *node, Parser *p) {
  switch (node->type) {
  case NODE_NUMBER:
    return newResult(&node->number, NODE_NUMBER);

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
    } else {
      printf("Error: Invalid types for binary operation\n");
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

  case NODE_IDENTIFIER_ACCESS: {
    if (strcmp(node->identifier.type, "number") == 0) {
      return newResult(&(double){convertStrToDouble(node->identifier.value)},
                       NODE_NUMBER);
    } else if (strcmp(node->identifier.type, "string") == 0) {
      return newResult(strdup(node->identifier.value),
                       NODE_STRING_LITERAL); // Duplicate string value
    }
  }
  case NODE_IDENTIFIER: {
    //  insertSymbol(p->table, node->identifier.name, node->identifier.type,
    //        node->identifier.value);
    return newResult(NULL, NODE_IDENTIFIER);
  }
  case NODE_STRING_LITERAL: {
    char *str = strdup(node->stringLiteral.value); // Duplicate string literal
    return newResult(str, NODE_STRING_LITERAL);
  }

  case NODE_BLOCK: {
    for (int i = 0; i < node->block.statementCount; i++) {
      AstNode *ast = node->block.statements[i];
      EvalAst(ast, p);
    };
    return NULL;
  }

  case NODE_IF_ELSE: {
    Result *conditionResult = EvalAst(node->ifElseBlock.condition, p);
    if (conditionResult->NodeType != NODE_NUMBER) {
      printf("Error: Condition in if-else must be a number (interpreted as "
             "boolean)\n");
      exit(EXIT_FAILURE);
    }

    double conditionValue = *(double *)(conditionResult->result);

    if (conditionValue) {
      // If condition is true (non-zero), execute ifBlock
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
    printf("Error: Unexpected node type\n");
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
  } else if (node->type == NODE_IDENTIFIER) {
    // Do not free identifier.name or symbol.value here if they are managed
    // elsewhere
  }
  free(node);
}

AstNode *parseAst(Parser *p) {
  if (parserIsAtEnd(p)) {

    return NULL; // No more tokens to parse
  }

  Token *tkn = p->current;
  switch (tkn->type) {
  case TOKEN_IDEN: {
    // Handle variable declaration or assignment
    AstNode *ast = varDecleration(p);
    consume(TOKEN_SEMI_COLON, p);
    return ast;
  }
  case TOKEN_STRING: {
    AstNode *ast = string(p);
    consume(TOKEN_SEMI_COLON, p); // Assuming statements end with a semicolon
    return ast;
  } // Handle string literals (if needed)
  case TOKEN_NUMBER: {
    // Handle number literals (if needed)
    AstNode *numberNode = factor(p);
    consume(TOKEN_SEMI_COLON, p); // Assuming statements end with a semicolon
    return numberNode;
  }
  case TOKEN_IF:
    // Handle if-else statements
    return ifElseParser(p);

  case TOKEN_LCURLY: {
    // Handle block statements
    AstNode *st = parseBlockStmt(p);
    return st;
  }
  default:
    printParseError(p, "Unexpected token %s falling through the condition\n",
                    tokenNames[tkn->type]);
    exit(EXIT_FAILURE);
  }
}
