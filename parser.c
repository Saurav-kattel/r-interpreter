
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
    printParseError(p, "unexpected token  %s expected %s\n",
                    tokenNames[p->current->type], tokenNames[type]);
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
  default:
    return "none";
  }
}

void printContext(Parser *p) {
  // Find the start of the current line
  int start = p->lex->curr;
  while (start > 0 && p->lex->source[start - 1] != '\n') {
    start--;
  }

  // Find the end of the current line
  int end = p->lex->curr;
  while (p->lex->source[end] != '\0' && p->lex->source[end] != '\n') {
    end++;
  }

  // Calculate the length of the line
  int length = end - start;

  // Allocate memory for the line and copy it
  char *line = (char *)malloc(length + 1);
  if (line == NULL) {
    printf("Failed allocating memory for line\n");
    exit(EXIT_FAILURE);
  }

  strncpy(line, &p->lex->source[start], length);
  line[length] = '\0'; // Null-terminate the string
  // Print the entire line
  printf(BLUE "at line" RESET);
  printf(GREEN "::" RESET);
  printf(CYAN "%d" RESET, p->lex->line);
  printf(GREEN "::" RESET);
  printf(RED "-> " RESET);
  printf(RED "%s\n" RESET, line);
  // Free the allocated memory
  free(line);
}

int isKeyword(char *name) {
  int arraySize = 6;
  const char *keywords[] = {"number", "string", "if", "fn", "else", "for"};
  for (int i = 0; i < arraySize; i++) {
    if (strcmp(keywords[i], name) == 0) {
      return 1;
    }
  }
  return 0;
}

void printParseError(Parser *p, const char *s, ...) {
  va_list args;
  va_start(args, s);

  // Print the filename and line number with color
  printf(MAGENTA "%s" RESET, p->lex->filename);
  printf(GREEN "::" RESET);
  printf(BLUE "%d" RESET, p->lex->line);
  printf(GREEN "::" RESET);
  printf(RED "Error-> " RESET);
  fflush(stdout);

  // Print the error message with color
  fprintf(stderr, YELLOW); // Set text color to yellow for the error message
  vfprintf(stderr, s, args);
  fprintf(stderr, RESET); // Reset text color

  va_end(args);

  // Print the context (with no color change, assuming it should be default)
  printContext(p);
}

static double convertStrToDouble(char *s) {
  double val;
  sscanf(s, "%lf", &val);
  return val;
}

int checkValidType(Token *typeToken) {

  int arraySize = 2;
  const char *types[] = {"number", "string"};

  for (int i = 0; i < arraySize; i++) {
    if (strcmp(types[i], typeToken->value) == 0) {
      return 1;
    }
  }
  return 0;
}

// -------------------------for parsing ast -----------------------
//

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

AstNode *newStringNode(char *value) {
  AstNode *node = (AstNode *)malloc(sizeof(AstNode));
  if (!node) {
    printf("Unable to allocate new AST node\n");
    exit(EXIT_FAILURE);
  }

  node->type = NODE_STRING_LITERAL; // Assuming NODE_STRING_LITERAL is the
                                    // type for string literals
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

  printParseError(p,
                  "Unexpected token %s with value '%s'. Expected number, "
                  "parenthesis, "
                  "or identifier.\n",
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

AstNode *handleNumberIdentifiers(Parser *p, Token *typeToken, char *varName) {
  if (strcmp(typeToken->value, getType(p)) != 0) {
    printParseError(p, "expected type %s but got number\n", typeToken->value);
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

AstNode *handleStringIdentifiers(Parser *p, Token *typeToken, char *varName) {
  if (strcmp(typeToken->value, getType(p)) != 0) {
    printParseError(p, "expected type %s but got string\n", typeToken->value);
    free(typeToken);
    exit(EXIT_FAILURE);
  }

  AstNode *node =
      newIdentifierNode("string", varName, p->current->value, p->table);
  advanceParser(p);
  return node;
}

AstNode *handleIdenIdentifiers(Parser *p, Token *typeToken, char *varName) {
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

AstNode *varDecleration(Parser *p) {
  Token *tkn = p->current;

  if (tkn->type != TOKEN_IDEN) {
    printParseError(p, "expected identifier, got %s \n", tokenNames[tkn->type]);
    exit(EXIT_FAILURE);
  }

  // consumes the name of the variable
  char *varName = strdup(tkn->value);
  if (isKeyword(varName)) {
    printParseError(p, "cannot use keyword as variable \"%s\" is a keyword\n",
                    varName);
    free(varName);
    exit(EXIT_FAILURE);
  }
  consume(TOKEN_IDEN, p);

  // consumes :
  consume(TOKEN_COLON, p);

  // consumes the variable type
  Token *typeToken = p->current;
  if (!checkValidType(typeToken)) {
    printParseError(p, "unknown type parameter %s\n", typeToken->value);
    exit(EXIT_FAILURE);
  };
  consume(TOKEN_IDEN, p);

  // consumes the =
  consume(TOKEN_ASSIGN, p);

  // handles var decleration and assignments
  switch (p->current->type) {
  case TOKEN_NUMBER:
    return handleNumberIdentifiers(p, typeToken, varName);
  case TOKEN_STRING:
    return handleStringIdentifiers(p, typeToken, varName);
  case TOKEN_IDEN:
    return handleIdenIdentifiers(p, typeToken, varName);
  default:
    printParseError(p, "unexpected token %s\n", tokenNames[p->current->type]);
    exit(EXIT_FAILURE);
  }
}
