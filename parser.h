#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "symbol.h"

#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN "\033[36m"
#define RESET "\033[0m"

enum {
  NODE_BINARY_OP,
  NODE_NUMBER,
  NODE_STRING_LITERAL,
  NODE_IDENTIFIER,
  NODE_UNARY_OP,
};

typedef struct Result {
  int NodeType;
  void *result;
} Result;

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
      SymbolTable *table;
    } identifier;

    struct {
      char *value;
    } stringLiteral;
  };
} AstNode;

// NECESSARY
AstNode *expr(Parser *);
AstNode *term(Parser *);
AstNode *factor(Parser *);
AstNode *unary(Parser *);
AstNode *relational(Parser *);
AstNode *logical(Parser *p);
AstNode *string(Parser *p);
AstNode *varDecleration(Parser *p);

// utils
Parser *InitParser(Lexer *, SymbolTable *);
void freeAst(AstNode *);
void consume(TokenType, Parser *);
void printParseError(Parser *p, const char *s, ...);
void printContext(Parser *p);
int checkValidType(Token *);
// MIGHT BE NEEDED
AstNode *parseProgram(Parser *p, SymbolTable *table);
AstNode *parseStatement(Parser *p, SymbolTable *table);
#endif // PRASER_H
