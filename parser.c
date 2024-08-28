
#include "parser.h"
#include "interpreter.h"
#include "lexer.h"
#include "symbol.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

Parser *InitParser(Lexer *lex, SymbolTable *table) {
  Parser *p = (Parser *)malloc(sizeof(Parser));
  if (p == NULL) {
    printf("unable to allocate parser");
    exit(EXIT_FAILURE);
  }
  p->lex = lex;
  p->current = GetNextToken(lex);
  p->table = table;
  return p;
}

int parserIsAtEnd(Parser *p) {
  return p->current == NULL || p->current->type == TOKEN_EOF;
}

Token *advanceParser(Parser *p) {
  do {
    if (parserIsAtEnd(p)) {
      return p->current;
    }
    p->current = GetNextToken(p->lex);
  } while (p->current->type == TOKEN_COMMENT);

  return p->current;
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
    printf("%s:%d:Error: unexpected token  %s expected %s\n", p->lex->filename,
           p->lex->line, tokenNames[p->current->type], tokenNames[type]);
    exit(EXIT_FAILURE);
  }

  advanceParser(p);
}

static char *getType(Parser *p) {
  switch (p->current->type) {
  case TOKEN_NUMBER:
    return "number";
  case TOKEN_STRING:
    return "string";
  }

  printf(
      "%s:%d:Error invalid type found expected string or number but got %s\n",
      p->lex->filename, p->lex->line, tokenNames[p->current->type]);
  exit(EXIT_FAILURE);
}

void printParseError(Parser *p, const char *s, ...) {
  printf("%s:%d:Error:", p->lex->filename, p->lex->line);
  va_list args;
  va_start(args, s);
  while (*s != '\0') {
    if (*s == '%' && *(s + 1) == 'd') {
      int i = va_arg(args, int);
      printf("%d", i);
      s++;
    } else if (*s == '%' && *(s + 1) == 'c') {
      int c = va_arg(args,
                     int); // char is promoted to int when passed through '...'
      putchar(c);
      s++;
    } else if (*s == '%' && *(s + 1) == 's') {
      char *str = va_arg(args, char *);
      printf("%s", str);
      s++;
    } else if (*s == '\n') {
      printf("\n");
      s++;
    } else {
      putchar(*s);
    }
    s++;
  }
  va_end(args);
}

AstNode *newIdentifierNode(char *type, char *name, char *value,
                           SymbolTable *table) {
  AstNode *node = (AstNode *)malloc(sizeof(AstNode));
  if (!node) {
    printf("unable to allocate new ast node\n");
    exit(EXIT_FAILURE);
  }

  if (lookupSymbol(table, name) == NULL) {
    Symbol *symbol =
        createSymbol(name, SYMBOL_VARIABLE, type, value, table->currentScope);
    insertSymbol(table, symbol);
  } else {
    printf("cannot redeclare %s is already decleared \n", name);
    exit(EXIT_FAILURE);
  }

  node->type = NODE_IDENTIFIER;
  node->identifier.value = strdup(value);
  node->identifier.type = strdup(type);
  node->identifier.name = strdup(name);
  node->identifier.table = table;

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

AstNode *newStringNode(char *value) {
  AstNode *node = (AstNode *)malloc(sizeof(AstNode));
  if (!node) {
    printf("Unable to allocate new AST node\n");
    exit(EXIT_FAILURE);
  }

  node->type = NODE_STRING_LITERAL; // Assuming NODE_STRING_LITERAL is the type
                                    // for string literals
  node->stringLiteral.value = strdup(value); // Duplicate the string value
  return node;
}
AstNode *string(Parser *p) {

  if (p->current == NULL) {
    printf("token is NULL\n");
    exit(EXIT_FAILURE);
  }

  if (p->current->type == TOKEN_STRING) {
    Token *tkn = p->current;
    consume(TOKEN_STRING, p);
    return newStringNode(tkn->value);
  }
  return NULL;
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
  AstNode *node = term(p);

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
    printf("Error: Token is NULL\n");
    exit(EXIT_FAILURE);
  }

  if (tkn->type == TOKEN_NUMBER) {
    double value = convertStrToDouble(tkn->value);
    consume(TOKEN_NUMBER, p);
    return newNumberNode(value);
  } else if (tkn->type == TOKEN_LPAREN) {
    consume(TOKEN_LPAREN, p);
    AstNode *node = logical(p); // Assuming logical handles expressions
    consume(TOKEN_RPAREN, p);
    return node;
  } else if (tkn->type == TOKEN_IDEN) {
    Symbol *sym = lookupSymbol(p->table, tkn->value);
    if (sym == NULL) {
      printf("Error: Variable '%s' used before declaration\n", tkn->value);
      exit(EXIT_FAILURE);
    }
    consume(TOKEN_IDEN, p);
    // Handle different types stored in the symbol table
    if (strcmp(sym->dataType, "number") == 0) {
      return newNumberNode(convertStrToDouble(sym->value));
    } else if (strcmp(sym->dataType, "string") == 0) {
      return newStringNode(sym->value);
    }
  }

  printf("%s:%d:Error: Unexpected token %s with value '%s'. Expected number, "
         "parenthesis, "
         "or identifier.\n",
         p->lex->filename, p->lex->line, tokenNames[tkn->type], tkn->value);
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

AstNode *varDecleration(Parser *p) {
  Token *tkn = p->current;

  if (tkn->type != TOKEN_IDEN) {
    printf("%s:%d:Error: Expected identifier, got %s \n", p->lex->filename,
           p->lex->line, tokenNames[tkn->type]);
    exit(EXIT_FAILURE);
  }

  char *varName = strdup(tkn->value);
  consume(TOKEN_IDEN, p);

  consume(TOKEN_COLON, p);

  // type of the variable
  if (p->current->type != TOKEN_IDEN) {
    printf("%s:%d:Error: Expected type, got %s \n", p->lex->filename,
           p->lex->line, tokenNames[p->current->type]);
    exit(EXIT_FAILURE);
  }

  Token *typeToken = p->current;
  consume(TOKEN_IDEN, p);

  consume(TOKEN_ASSIGN, p);
  if (p->current->type == TOKEN_NUMBER) {
    if (strcmp(typeToken->value, getType(p)) != 0) {
      printParseError(p, "expected type number but got %s\n", typeToken->value);
      free(typeToken);
      exit(EXIT_FAILURE);
    }
    AstNode *valueNode = logical(p);
    Result *res = EvalAst(valueNode);
    char buffer[1024];
    sprintf(buffer, "%lf", *(double *)res->result);

    AstNode *identifierNode =
        newIdentifierNode("number", varName, buffer, p->table);
    return identifierNode;
  }

  if (p->current->type == TOKEN_STRING) {

    if (strcmp(typeToken->value, getType(p)) != 0) {
      printParseError(p, "expected type string but got %s\n", typeToken->value);
      free(typeToken);
      exit(EXIT_FAILURE);
    }

    AstNode *node =
        newIdentifierNode("string", varName, p->current->value, p->table);
    advanceParser(p);
    return node;
  }

  if (p->current->type == TOKEN_IDEN) {
    Symbol *sym = lookupSymbol(p->table, p->current->value);
    if (strcmp(sym->dataType, typeToken->value) != 0) {
      printParseError(p, "cannot assign type %s to type %s\n", sym->dataType,
                      typeToken->value);
      exit(EXIT_FAILURE);
    }
    if (sym == NULL) {
      printParseError(p, "Variable %s used before declaration\n",
                      p->current->value);
      exit(EXIT_FAILURE);
    }

    // Parse the expression involving the identifier
    AstNode *valueNode = logical(p);

    // Evaluate the expression and store the result
    char buffer[1024];

    Result *res = EvalAst(valueNode);
    if (res->NodeType == NODE_NUMBER) {
      sprintf(buffer, "%lf", *(double *)res->result);
    } else if (res->NodeType == NODE_STRING_LITERAL) {
      strcpy(buffer, (char *)res->result);
    }

    AstNode *node = newIdentifierNode(sym->dataType, varName, buffer, p->table);
    return node;
  }
  printParseError(p, "unexpected token %s\n", tokenNames[p->current->type]);
  exit(EXIT_FAILURE);
}
