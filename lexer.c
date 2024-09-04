#include "lexer.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// A constructor for Lexer reutrns Lexer

Lexer *InitLexer(char *source, char *filename) {
  Lexer *lex = (Lexer *)malloc(sizeof(Lexer));
  if (lex == NULL) {
    printf("Failed allocating memory for lexer\n");
    exit(EXIT_FAILURE);
  }

  lex->curr = 0;
  lex->line = 1;

  // Allocate memory for filename and copy it
  lex->filename = strdup(filename);
  if (lex->filename == NULL) {
    printf("Failed allocating memory for filename\n");
    free(lex); // Clean up previously allocated memory
    exit(EXIT_FAILURE);
  }

  // Allocate memory for source and copy it
  size_t source_len = strlen(source);

  lex->source = (char *)malloc(sizeof(char) * (source_len + 1));
  // Ensure +1 for null terminator
  if (lex->source == NULL) {
    printf("Failed allocating memory for source\n");
    free(filename);
    free(lex->filename); // Clean up previously allocated memory
    free(lex);           // Clean up previously allocated memory
    exit(EXIT_FAILURE);
  }
  strcpy(lex->source, source);
  lex->source[source_len] = '\0';
  return lex;
}
// Returns  a new token with the supplied value and type
Token *NewToken(TokenType type, char *value) {
  Token *tkn = (Token *)malloc(sizeof(Token));
  tkn->type = type;
  tkn->value = strdup(value);
  return tkn;
}

int isAtEnd(Lexer *l) { return l->source[l->curr] == '\0'; }
char advance(Lexer *l) {
  if (!isAtEnd(l)) {
    char ch = l->source[l->curr];
    l->curr++;
    if (ch == '\n') {
      l->line++;
    }
    return ch;
  }
  return 0;
}

char peekNext(Lexer *l) {
  if (!isAtEnd(l)) {
    return l->source[l->curr + 1];
  }
  return 0;
}

char peek(Lexer *l) {
  if (!isAtEnd(l)) {
    return l->source[l->curr];
  }
  return 0;
}

void skipWhiteSpace(Lexer *l) {
  while (isspace(peek(l)) && !isAtEnd(l)) {
    advance(l);
  }
}

TokenType getKeywordTokenType(char *buff) {
  if (strcmp(buff, "if") == 0) {
    return TOKEN_IF;
  }
  if (strcmp(buff, "else") == 0) {
    return TOKEN_ELSE;
  }

  if (strcmp(buff, "for") == 0) {
    return TOKEN_FOR;
  }

  if (strcmp(buff, "return") == 0) {
    return TOKEN_RETURN;
  }

  if (strcmp(buff, "fn") == 0) {
    return TOKEN_FN;
  }

  if (strcmp(buff, "println") == 0) {
    return TOKEN_PRINT;
  }
  if (strcmp(buff, "readIn") == 0) {
    return TOKEN_READ_IN;
  }
  return -1;
}

int isNotTypeKeyword(char *name) {
  int arraySize = 7;
  const char *keywords[] = {"if",     "fn",      "else",  "for",
                            "return", "println", "readIn"};

  for (int i = 0; i < arraySize; i++) {
    if (strcmp(keywords[i], name) == 0) {
      return 1;
    }
  }
  return 0;
}
Token *GetNextToken(Lexer *l) {
  skipWhiteSpace(l);
  char c = advance(l);

  //
  //___________________________-
  //                          Literals
  //                              -_________________________________

  if (c == '"') {
    int start = l->curr - 1;
    advance(l);
    while (peek(l) != '"') {
      if (isAtEnd(l)) {
        printf("unterminated string\n");
        exit(EXIT_FAILURE);
      }
      advance(l);
    }

    advance(l); // eating the "
    int length = l->curr - start;
    char *buffer = (char *)malloc(length + 1);
    for (int i = 0; i < length; i++) {
      buffer[i] = l->source[start + i];
    }
    buffer[length] = '\0';

    Token *tkn = NewToken(TOKEN_STRING, buffer);
    free(buffer);
    return tkn;
  }

  if (c == ' ') {
    advance(l);
  }

  // identifiers and keywords
  if (isalpha(c)) {
    int start = l->curr - 1;

    while (isalpha(peek(l))) {
      advance(l);
    }

    int length = l->curr - start;
    char *buffer = (char *)malloc(length + 1);

    strncpy(buffer, l->source + start, length);
    buffer[length] = '\0';

    if (isNotTypeKeyword(buffer)) {

      TokenType type = getKeywordTokenType(buffer);
      if (type == -1) {
        printf("unkwon type\n");
        exit(EXIT_FAILURE);
      }
      Token *tkn = NewToken(type, buffer);
      free(buffer);
      return tkn;
    }

    Token *tkn = NewToken(TOKEN_IDEN, buffer);
    free(buffer);
    return tkn;
  }

  if (isdigit(c)) {
    int start = l->curr - 1;
    while (isdigit(peek(l))) {
      advance(l);
    }
    if (peek(l) == '.' && isdigit(peekNext(l))) {
      advance(l); // Skip the '.'
      while (isdigit(peek(l))) {
        advance(l);
      }
    }

    int length = l->curr - start;
    char *buffer = (char *)malloc(length + 1);
    strncpy(buffer, l->source + start, length);
    buffer[length] = '\0';
    Token *tkn = NewToken(TOKEN_NUMBER, buffer);
    free(buffer);
    return tkn;
  }
  //
  //___________________________-
  //                            symobols
  //                              -_________________________________

  if (c == '(') {
    if (!isAtEnd(l)) {
      return NewToken(TOKEN_LPAREN, "(");
    }
    printf("unexpected token (\n");
  }

  if (c == ')') {
    if (!isAtEnd(l)) {
      return NewToken(TOKEN_RPAREN, ")");
    }
    printf("unexpected token )\n");
  }

  if (c == '{') {
    if (!isAtEnd(l)) {
      return NewToken(TOKEN_LCURLY, "{");
    }
    printf("unexpected token {\n");
  }

  if (c == ',') {
    return NewToken(TOKEN_COMMA, ",");
  }

  if (c == '}') {
    if (!isAtEnd(l)) {
      return NewToken(TOKEN_RCURLY, "}");
    }
    printf("should not happen\n");
  }

  if (c == ';') {
    return NewToken(TOKEN_SEMI_COLON, ";");
  }
  if (c == '.') {
    return NewToken(TOKEN_DOT, ".");
  }

  if (c == '#') {
    advance(l);
    while ((peek(l) != '#') && (!isAtEnd(l))) {
      advance(l);
    }
    advance(l);
    return NewToken(TOKEN_COMMENT, "");
  }

  if (c == ':') {
    return NewToken(TOKEN_COLON, ":");
  }
  //
  //___________________________-Arthemetic
  //                            Operations-_________________________________

  if (c == '-') {
    if (!isAtEnd(l)) {
      return NewToken(TOKEN_MINUS, "-");
    }
    printf("invalid expression \n");
    exit(EXIT_FAILURE);
  }

  if (c == '%') {
    if (!isAtEnd(l)) {
      return NewToken(TOKEN_MODULO, "%");
    }
    printf("invalid expression \n");
    exit(EXIT_FAILURE);
  }

  if (c == '+') {
    if (!isAtEnd(l)) {
      return NewToken(TOKEN_PLUS, "+");
    }
    printf("invalid expression \n");
    exit(EXIT_FAILURE);
  }

  if (c == '/') {
    if (!isAtEnd(l)) {
      return NewToken(TOKEN_DIVIDE, "/");
    }
    printf("invalid expression \n");
    exit(EXIT_FAILURE);
  }

  if (c == '*') {
    if (!isAtEnd(l)) {
      return NewToken(TOKEN_MULTIPLY, "*");
    }
    printf("invalid expression \n");
    exit(EXIT_FAILURE);
  }

  if (isAtEnd(l)) {
    return NewToken(TOKEN_EOF, "EOF");
  }

  //
  //_______________________-Realational
  //                        Operations-______________________________________
  //
  if (c == '>') {

    if (isAtEnd(l)) {
      printf("invalid expression \n");
      exit(EXIT_FAILURE);
    }

    if (peek(l) == '=') {
      advance(l);
      return NewToken(TOKEN_EQ_GREATER, ">=");
    }
    return NewToken(TOKEN_GREATER, ">");
  }

  if (c == '<') {
    if (isAtEnd(l)) {
      printf("invalid expression \n");
      exit(EXIT_FAILURE);
    }

    if (peek(l) == '=') {
      advance(l);
      return NewToken(TOKEN_EQ_LESSER, "<=");
    }
    return NewToken(TOKEN_LESSER, "<");
  }

  if (c == '=') {
    if (isAtEnd(l)) {
      printf("invalid expression \n");
      exit(EXIT_FAILURE);
    }

    if (peek(l) == '=') {
      advance(l);
      return NewToken(TOKEN_DB_EQUAL, "==");
    }
    return NewToken(TOKEN_ASSIGN, "=");
  }

  //
  //_______________________- Logical
  //                        Operations-______________________________________
  //

  if (c == '&') {
    if (isAtEnd(l)) {
      printf("invalid expression \n");
      exit(EXIT_FAILURE);
    }

    if (peek(l) == '&') {
      advance(l);
      return NewToken(TOKEN_AND, "&&");
    }
    printf("unknown token & were you trying to ues &&\n");
    exit(EXIT_FAILURE);
  }

  if (c == '|') {
    if (isAtEnd(l)) {
      printf("invalid expression \n");
      exit(EXIT_FAILURE);
    }

    if (peek(l) == '|') {
      advance(l);
      return NewToken(TOKEN_OR, "||");
    }
    printf("unknown token | were you trying to ues ||\n");
    exit(EXIT_FAILURE);
  }

  if (c == '!') {
    if (isAtEnd(l)) {
      printf("invalid expression \n");
      exit(EXIT_FAILURE);
    }

    if (peek(l) == '=') {
      advance(l);
      return NewToken(TOKEN_EQ_NOT, "!=");
    }
    return NewToken(TOKEN_NOT, "!");
  }

  printf("unknown token %c\n", c);
  exit(EXIT_FAILURE);
}
