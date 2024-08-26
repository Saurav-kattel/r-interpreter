
#include "parser.h"
#include "lexer.h"
#include "symbol.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

Parser *InitParser(Lexer *lex) {
  Parser *p = (Parser *)malloc(sizeof(Parser));
  if (p == NULL) {
    printf("unable to allocate parser");
    exit(EXIT_FAILURE);
  }
  p->lex = lex;
  p->current = GetNextToken(lex);
  return p;
}

int parserIsAtEnd(Parser *p) {
  return p->current == NULL || p->current->type == TOKEN_EOF;
}

Token *advanceParser(Parser *p) {

  if (parserIsAtEnd(p)) {
    return p->current;
  }

  Token *tmp = p->current;
  p->current = GetNextToken(p->lex);
  return tmp;
}

Token *parserPeek(Parser *p) {
  if (!parserIsAtEnd(p)) {
    return GetNextToken(p->lex);
  }
  return NULL;
}

Token *parserPeekNext(Parser *p) {
  if (!parserIsAtEnd(p)) {
    Token *tmp = GetNextToken(p->lex);
    tmp = GetNextToken(p->lex);
    return tmp;
  }
  return NULL;
}

void consume(TokenType type, Parser *p) {
  if (p->current->type != type) {
    printf("unexpected token  %s expected %s\n", tokenNames[p->current->type],
           tokenNames[type]);
    exit(EXIT_FAILURE);
  }

  advanceParser(p);
}

AstNode *newIdentifierNode(char *type, char *name, AstNode *value,
                           SymbolTable *table) {
  AstNode *node = (AstNode *)malloc(sizeof(AstNode));
  if (!node) {
    printf("unable to allocate new ast node\n");
    exit(EXIT_FAILURE);
  }
  node->type = NODE_IDENTIFIER;

  node->identifier.value = value;

  Symbol *symbol =
      createSymbol(name, SYMBOL_VARIABLE, type, table->currentScope);
  insertSymbol(table, symbol);

  node->identifier.type = strdup(type);
  node->identifier.name = strdup(name);
  return node;
}

AstNode *newBinaryNode(TokenType op, AstNode *left, AstNode *right) {

  AstNode *node = (AstNode *)malloc(sizeof(AstNode));
  if (!node) {
    printf("unable to allocate new ast node\n");
    exit(EXIT_FAILURE);
  }
  node->type = NODE_BINARY_OP;

  node->binaryOp.op = op;
  node->binaryOp.left = left;
  node->binaryOp.right = right;
  return node;
}

AstNode *newNumberNode(double value) {
  AstNode *node = (AstNode *)malloc(sizeof(AstNode));
  if (!node) {
    printf("unable to allocate new ast node\n");
    exit(EXIT_FAILURE);
  }
  node->type = NODE_NUMBER;
  node->number = value;
  return node;
}

AstNode *newUnaryNode(TokenType type, AstNode *right) {
  AstNode *node = (AstNode *)malloc(sizeof(AstNode));
  if (!node) {
    printf("unable to allocate new ast node\n");
    exit(EXIT_FAILURE);
  }
  node->type = NODE_UNARY_OP;

  node->unaryOp.op = type;
  node->unaryOp.right = right;
  return node;
}

static double convertStrToDouble(char *s) {
  double val;
  sscanf(s, "%lf", &val);
  return val;
}

AstNode *term(Parser *p) {

  if (p->current == NULL) {
    printf("token is NULL\n");
    exit(EXIT_FAILURE);
  }
  AstNode *node = factor(p);

  while (p->current && (p->current->type == TOKEN_MULTIPLY ||
                        p->current->type == TOKEN_DIVIDE)) {
    Token *tkn = p->current;
    consume(tkn->type, p);
    AstNode *right = factor(p);
    node = newBinaryNode(tkn->type, node,
                         right); // Pass tkn->type instead of NODE_BINARY_OP
  }
  return node;
}

AstNode *expr(Parser *p) {
  // Start by parsing the term
  AstNode *node = term(p);

  // Continue as long as the current token is '+' or '-'
  while (p->current &&
         (p->current->type == TOKEN_PLUS || p->current->type == TOKEN_MINUS)) {
    Token *tkn = p->current;

    // Consume the '+' or '-' token
    consume(tkn->type, p);

    // Parse the next term
    AstNode *right = term(p);

    // Create a new binary operation node
    node = newBinaryNode(tkn->type, node, right);
  }

  // Return the parsed expression
  return node;
}

AstNode *factor(Parser *p) {
  Token *tkn = p->current;

  if (tkn == NULL) {
    printf("token is NULL\n");
    exit(EXIT_FAILURE);
  }
  if (tkn->type == TOKEN_NUMBER) {
    consume(TOKEN_NUMBER, p);
    double value = convertStrToDouble(tkn->value);

    return newNumberNode(value);
  } else if (tkn->type == TOKEN_LPAREN) {
    consume(TOKEN_LPAREN, p);
    AstNode *node = logical(p);
    consume(TOKEN_RPAREN, p);
    return node;
  }

  printf(
      "unknown token %s with value of %s but expected number or parenthesis\n",
      tokenNames[tkn->type], tkn->value);
  exit(EXIT_FAILURE);
}

AstNode *relational(Parser *p) {
  AstNode *node = expr(p);

  while (p->current && (p->current->type == TOKEN_DB_EQUAL ||
                        p->current->type == TOKEN_EQ_LESSER ||
                        p->current->type == TOKEN_LESSER ||
                        p->current->type == TOKEN_GREATER ||
                        p->current->type == TOKEN_EQ_GREATER ||
                        p->current->type == TOKEN_EQ_NOT)) {

    Token *tkn = p->current;
    consume(tkn->type, p);
    AstNode *right = expr(p);
    node = newBinaryNode(tkn->type, node, right);
  }

  return node;
}

AstNode *logical(Parser *p) {
  AstNode *node = unary(p);
  while (p->current &&
         (p->current->type == TOKEN_OR || p->current->type == TOKEN_AND)) {
    Token *tkn = p->current;
    consume(tkn->type, p);
    AstNode *right = unary(p);
    node = newBinaryNode(tkn->type, node, right);
  }
  return node;
}

AstNode *unary(Parser *p) {
  if (p->current && (p->current->type == TOKEN_NOT)) {
    Token *tkn = p->current;
    consume(tkn->type, p);
    AstNode *right = unary(p);
    return newUnaryNode(tkn->type, right);
  }
  return relational(p);
}

// for variable decleration
AstNode *varDecleration(Parser *p, SymbolTable *table) {
  Token *tkn = p->current;

  if (tkn->type != TOKEN_IDEN) {
    printf("Expected identifier, got %s\n", tokenNames[tkn->type]);
    exit(EXIT_FAILURE);
  }

  char *varName = strdup(tkn->value);
  advanceParser(p);

  if (p->current->type != TOKEN_COLON) {
    printf("Expected :, got %s\n", tokenNames[tkn->type]);
    exit(EXIT_FAILURE);
  }

  advanceParser(p);
  if (p->current->type != TOKEN_IDEN) {
    printf("Expected variable type, got %s\n", tokenNames[tkn->type]);
    exit(EXIT_FAILURE);
  }
  char *type = strdup(p->current->value);
  advanceParser(p);
  if (p->current->type != TOKEN_ASSIGN) {
    printf("Expected =, got %s\n", tokenNames[tkn->type]);
    exit(EXIT_FAILURE);
  }
  advanceParser(p);
  printf("%s\n", tokenNames[p->current->type]);
  AstNode *valueNode = logical(p);

  AstNode *identifierNode = newIdentifierNode(type, varName, valueNode, table);

  free(varName);
  free(type);

  return identifierNode;
}
