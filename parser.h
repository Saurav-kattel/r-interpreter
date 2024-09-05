#ifndef PARSER_H
#define PARSER_H

#include "common.h"
#include "lexer.h"

#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN "\033[36m"
#define RESET "\033[0m"

static const char *nodeTypeNames[] = {
    "node_binary_op",
    "node_number",
    "node_string_literal",
    "node_function",
    "node_function_param",
    "node_identifier_value",
    "node_identifier_declaration", // correcting the spelling of "decleration"
    "node_identifier_assignment",
    "node_identifier_mutation",
    "node_unary_op",
    "node_block",
    "node_function_call",
    "node_if_else",
    "node_return",
    "node_function_print",
    "node_function_read_in",
    "node_for_loop",
};
enum {
  NODE_BINARY_OP,
  NODE_NUMBER,
  NODE_STRING_LITERAL,
  NODE_FUNCTION,
  NODE_FUNCTION_PARAM,
  NODE_IDENTIFIER_VALUE,
  NODE_IDENTIFIER_DECLERATION,
  NODE_IDENTIFIER_ASSIGNMENT,
  NODE_IDENTIFIER_MUTATION,
  NODE_UNARY_OP,
  NODE_BLOCK,
  NODE_FUNCTION_CALL,
  NODE_IF_ELSE,
  NODE_RETURN,
  NODE_FUNCTION_PRINT,
  NODE_FUNCTION_READ_IN,
  NODE_FOR_LOOP,
};

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
AstNode *parseReturn(Parser *p);
AstNode *parsePrint(Parser *p);
AstNode *parseForLoop(Parser *p);
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
AstNode *parseFunction(Parser *p);
AstNode *functionCall(Parser *p);
AstNode *parseReadIn(Parser *p);
int isKeyword(char *);
#endif // PRASER_H
