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
  SymbolTable *table;
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
      char *value;
    } identifier;

    struct {
      char *value;
    } stringLiteral;
  };
} AstNode;

AstNode *expr(Parser *);
AstNode *term(Parser *);
AstNode *factor(Parser *);
AstNode *unary(Parser *);
AstNode *relational(Parser *);
AstNode *logical(Parser *p);
AstNode *string(Parser *p);
double EvalAst(AstNode *);
AstNode *parseProgram(Parser *p, SymbolTable *table);
AstNode *parseStatement(Parser *p, SymbolTable *table);
Parser *InitParser(Lexer *, SymbolTable *);
void freeAst(AstNode *);
void consume(TokenType, Parser *);

AstNode *varDecleration(Parser *p);
#endif // PRASER_H
