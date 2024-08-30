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
  NODE_IDENTIFIER_DECLERATION,
  NODE_IDENTIFIER_ASSIGNMENT,
  NODE_IDENTIFIER_MUTATION,
  NODE_UNARY_OP,
  NODE_BLOCK,
  NODE_IF_ELSE,
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
      int isDeceleration;
    } identifier;

    struct {
      struct AstNode **statements;
      int statementCount;
    } block;

    struct {
      struct AstNode *condition;
      struct AstNode *ifBlock;
      struct AstNode *elseBlock;
    } ifElseBlock;

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
AstNode *parseBlockStmt(Parser *p);
AstNode *ifElseParser(Parser *p);
AstNode *parseStatement(Parser *p);
// utils
Parser *InitParser(Lexer *, SymbolTable *);
void freeAst(AstNode *);
void consume(TokenType, Parser *);
void printParseError(Parser *p, const char *s, ...);
void printContext(Parser *p);
int checkValidType(Token *);
int parserIsAtEnd(Parser *p);
// MIGHT BE NEEDED
AstNode *parseProgram(Parser *p, SymbolTable *table);
int isKeyword(char *);
#endif // PRASER_H
