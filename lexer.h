#ifndef LEXER_H
#define LEXER_H

enum {

  // literals
  TOKEN_STRING,
  TOKEN_NUMBER,
  TOKEN_IDEN,
  // symbols
  TOKEN_LPAREN,
  TOKEN_RPAREN,
  TOKEN_LCURLY,
  TOKEN_RCURLY,
  TOKEN_SEMI_COLON,
  TOKEN_ASSIGN,
  TOKEN_COMMENT,
  TOKEN_EOF,
  TOKEN_COLON,
  TOKEN_ERR,
  TOKEN_COMMA,
  TOKEN_DOT,

  // arthemetic
  TOKEN_PLUS,
  TOKEN_MINUS,
  TOKEN_DIVIDE,
  TOKEN_MULTIPLY,
  TOKEN_MODULO,

  // relational operators
  TOKEN_GREATER,
  TOKEN_LESSER,
  TOKEN_DB_EQUAL,
  TOKEN_EQ_GREATER,
  TOKEN_EQ_LESSER,
  TOKEN_EQ_NOT,
  // LOGICAL
  TOKEN_AND,
  TOKEN_OR,
  TOKEN_NOT,

  TOKEN_IF,
  TOKEN_ELSE,
  TOKEN_FOR,
  TOKEN_RETURN,
  TOKEN_FN,
  TOKEN_PRINT,
  TOKEN_READ_IN,

};

static const char *tokenNames[] = {
    // literals
    "string",
    "number",
    "identifier",
    // symbols
    "(",
    ")",
    "{",
    "}",
    ";",
    "=",
    "COMMENT",
    "EOF",
    ":",
    "ERR",
    ",",
    ".",

    // arthemetic
    "+",
    "-",
    "/",
    "*",
    "%",

    // relational operators
    " >",
    "<",
    "==",
    ">=",
    "<=",
    "!",

    // LOGICAL
    "&&",
    "||",
    "!",

    "if",
    "else",
    "for",
    "return",
    "fn",
    "println",
    "readIn",
};

typedef int TokenType;

typedef struct {
  char *source;
  int curr;
  int line;
  char *filename;
} Lexer;

typedef struct {
  TokenType type;
  char *value;
} Token;

Lexer *InitLexer(char *, char *);
Token *NewToken(TokenType, char *);
Token *GetNextToken(Lexer *);
char peek(Lexer *);
char peekNext(Lexer *);
char advance(Lexer *);
#endif // LEXER_H
