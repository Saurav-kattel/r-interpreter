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
  lex->line = 0;
  lex->filename = strdup(filename);
  lex->source = (char *)malloc(sizeof(char) * strlen(source));
  strcpy(lex->source, source);
  return lex;
}

// Returns  a new token with the supplied value and type
Token *NewToken(TokenType type, char *value) {
  Token *tkn = (Token *)malloc(sizeof(Token));
  tkn->type = type;
  strcpy(tkn->value, value);
  return tkn;
}

int isAtEnd(Lexer *l) { return l->curr >= strlen(l->source); }

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
      // strcpy(buffer[i], l->source[start + i]);
    }
    // strncpy(buffer, l->source + start, length);
    buffer[length] = '\0';

    Token *tkn = NewToken(TOKEN_STRING, buffer);
    free(buffer);
    return tkn;
  }

  if (c == ' ') {
    advance(l);
  }

  if (isalpha(c)) {
    int start = l->curr - 1;

    while (isalpha(peek(l))) {
      advance(l);
    }

    int length = l->curr - start;
    char *buffer = (char *)malloc(length + 1);
    strncpy(buffer, l->source + start, length);
    buffer[length] = '\0';
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
      return NewToken(TOKEN_LPAREN, "{");
    }
    printf("unexpected token {\n");
  }

  if (c == '}') {
    if (!isAtEnd(l)) {
      return NewToken(TOKEN_RPAREN, "}");
    }
    printf("unexpecteRRRRRRRRRRn");
  }

  if (c == ';') {
    return NewToken(TOKEN_SEMI_COLON, ";");
  }

  if (c == '#') {
    advance(l);
    while (peek(l) != '#' && (!isAtEnd(l))) {
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
