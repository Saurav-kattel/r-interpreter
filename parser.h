#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "symbol.h"

enum {
  NODE_BINARY_OP,
  NODE_NUMBER,
  NODE_STRING_LITERAL,
  NODE_IDENTIFIER,
  NODE_UNARY_OP,
};

typedef struct {
  Token *current;
  Lexer *lex;
} Parser;

typedef struct AstNode {
  int type;
  double number;
  union {
    struct {
      struct AstNode *left;
      struct AstNode *right;
      TokenType op;
    } binaryOp;

    struct {
      struct AstNode *right;
      TokenType op;
    } unaryOp;

    struct {
      char *name;
      char *type;
      struct AstNode *value;
    } identifier;
  };
} AstNode;

AstNode *expr(Parser *);
AstNode *term(Parser *);
AstNode *factor(Parser *);
AstNode *unary(Parser *);
AstNode *relational(Parser *);
AstNode *logical(Parser *p);

double EvalAst(AstNode *);

Parser *InitParser(Lexer *);
void freeAst(AstNode *);
void consume(TokenType, Parser *);

AstNode *varDecleration(Parser *p, SymbolTable *table);
#endif // PRASER_H
