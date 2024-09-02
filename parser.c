
#include "parser.h"
#include "common.h"
#include "interpreter.h"
#include "lexer.h"
#include "symbol.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void populateTokens(Parser *p, Lexer *lex, int initialCapacity, int *size) {
  Token *tkn = GetNextToken(lex);
  p->tokens = (Token **)malloc(sizeof(Token *) * initialCapacity);

  while (tkn->type != TOKEN_EOF) {
    if (*size >= initialCapacity) {
      initialCapacity += 100;
      p->tokens =
          (Token **)realloc(p->tokens, sizeof(Token *) * initialCapacity);
    }
    p->tokens[*size] = tkn;
    (*size)++;
    tkn = GetNextToken(lex);
  }
  p->tokens[*size] = tkn;
  (*size)++;
}

// initializing the parser
Parser *InitParser(Lexer *lex, SymbolTable *table) {
  Parser *p = (Parser *)malloc(sizeof(Parser));

  if (p == NULL) {
    printf("unable to allocate parser");
    exit(EXIT_FAILURE);
  }

  p->lex = lex;
  int size = 0;
  p->idx = 0;
  int initialCapacity = 100;
  populateTokens(p, lex, initialCapacity, &size);

  p->current = p->tokens[0];
  p->table = table;
  p->size = size;
  return p;
}

int parserIsAtEnd(Parser *p) { return p->idx >= p->size; }

Token *advanceParser(Parser *p) {
  do {
    if (parserIsAtEnd(p)) {
      return p->current;
    }
    p->idx++;
    p->current = p->tokens[p->idx];
  } while (p->current->type == TOKEN_COMMENT);

  return p->current;
}

// to look ate the next occuring token
Token *parserPeek(Parser *p) {
  if (!parserIsAtEnd(p)) {
    return p->tokens[p->idx + 1];
  }
  return NULL;
}

// to look at the the 2nd positon from the current parser positon
Token *parserPeekNext(Parser *p) {
  if (!parserIsAtEnd(p)) {
    return p->tokens[p->idx + 2];
  }
  return NULL;
}

// checks the type and advances the parser
void consume(TokenType type, Parser *p) {
  if (p->current->type != type) {
    printParseError(p, "unexpected token  %s expected token %s\n",
                    tokenNames[p->current->type], tokenNames[type]);
    exit(EXIT_FAILURE);
  }

  advanceParser(p);
}

// return type based on the token type
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

// to print the error location
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

// parses the if else block into ast

AstNode *newIfElseNode(AstNode *condition, AstNode *ifBlock,
                       AstNode *elseBlock) {
  AstNode *node = (AstNode *)malloc(sizeof(AstNode));
  if (!node) {
    printf("unable to allocate new ast node\n");
    exit(EXIT_FAILURE);
  }
  node->type = NODE_IF_ELSE;
  node->ifElseBlock.condition = condition;
  node->ifElseBlock.ifBlock = ifBlock;
  node->ifElseBlock.elseBlock = elseBlock;
  return node;
}

// returns the identifier ast from the provided argumentes
AstNode *newIdentifierNode(char *type, char *name, AstNode *value, Parser *p,
                           int nodeType) {
  AstNode *node = (AstNode *)malloc(sizeof(AstNode));
  if (!node) {
    printf("unable to allocate new ast node\n");
    exit(EXIT_FAILURE);
  }

  int isDeceleration = 0;
  if (nodeType == NODE_IDENTIFIER_DECLERATION) {
    isDeceleration = 1;
  }

  node->type = nodeType;

  if (value != NULL) {
    node->identifier.value = value;
  }
  if (type != NULL) {
    node->identifier.type = strdup(type);
  }
  node->identifier.name = strdup(name);
  node->identifier.table = p->table;
  node->identifier.isDeceleration = isDeceleration;
  return node;
}

// returns the binary ast from the provided argumentes
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
// returns the number ast from the provided argumentes
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
// returns the unary ast from the provided argumentes

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

// returns the string ast from the provided argumentes

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

// returns the string's ast after check and validating syntax and tokens

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

// builds the ast for arthemetic, relational and logical operations
AstNode *term(Parser *p) {
  if (p->current == NULL) {
    printf("token is NULL\n");
    exit(EXIT_FAILURE);
  }
  AstNode *node = factor(p);

  while (p->current &&
         (p->current->type == TOKEN_MULTIPLY ||
          p->current->type == TOKEN_DIVIDE || p->current->type == TOKEN_DOT)) {
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
    // to parse the variable that was assigned as a value
    AstNode *node =
        newIdentifierNode("", tkn->value, NULL, p, NODE_IDENTIFIER_VALUE);
    consume(TOKEN_IDEN, p);
    return node;
  } else if (tkn->type == TOKEN_STRING) {
    AstNode *node = newStringNode(p->current->value);
    consume(TOKEN_STRING, p);
    return node;
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

AstNode *handleNumberIdentifiers(Parser *p, Token *typeToken, char *varName,
                                 int nodeType) {
  AstNode *valueNode = logical(p);
  if (nodeType == NODE_IDENTIFIER_MUTATION) {
    return newIdentifierNode("number", varName, valueNode, p, nodeType);
  }
  return newIdentifierNode(typeToken->value, varName, valueNode, p, nodeType);
}

AstNode *handleStringIdentifiers(Parser *p, Token *typeToken, char *varName,
                                 int nodeType) {
  AstNode *valueNode = logical(p);

  if (nodeType == NODE_IDENTIFIER_MUTATION) {
    return newIdentifierNode("string", varName, valueNode, p, nodeType);
  }
  return newIdentifierNode(typeToken->value, varName, valueNode, p, nodeType);
}

AstNode *handleIdenIdentifiers(Parser *p, Token *typeToken, char *varName,
                               int nodeType) {
  AstNode *valueNode = logical(p);

  return newIdentifierNode(typeToken->value, varName, valueNode, p, nodeType);
}

// handles  variables

AstNode *varDecleration(Parser *p) {
  Token *tkn = p->current;

  if (p->current->type == TOKEN_IDEN && parserPeek(p)->type == TOKEN_LPAREN) {
    return functionCall(p);
  }

  char *varName = strdup(tkn->value);

  if (isKeyword(varName)) {
    printParseError(p, "cannot use keyword as variable \"%s\" is a keyword\n",
                    varName);
    free(varName);
    exit(EXIT_FAILURE);
  }
  consume(TOKEN_IDEN, p);
  if (p->current->type == TOKEN_ASSIGN) {
    consume(TOKEN_ASSIGN, p);
    switch (p->current->type) {
    case TOKEN_NUMBER: {

      Token newToken = {.type = TOKEN_NUMBER,
                        .value = *strdup(p->current->value)};
      return handleNumberIdentifiers(p, &newToken, varName,
                                     NODE_IDENTIFIER_MUTATION);
    }
    case TOKEN_STRING: {
      Token newToken = {.type = TOKEN_STRING,
                        .value = *strdup(p->current->value)};

      return handleStringIdentifiers(p, p->current, varName,
                                     NODE_IDENTIFIER_MUTATION);
    }
    case TOKEN_IDEN: {
      return handleIdenIdentifiers(p, p->current, varName,
                                   NODE_IDENTIFIER_MUTATION);
    }
    default:
      printParseError(p, "unknown token \"%s\"", tokenNames[p->current->type]);
      exit(EXIT_FAILURE);
      break;
    }
  }

  consume(TOKEN_COLON, p);
  Token *typeToken = p->current;
  consume(TOKEN_IDEN, p);

  if (p->current->type == TOKEN_SEMI_COLON) {
    return newIdentifierNode(typeToken->value, varName, NULL, p,
                             NODE_IDENTIFIER_DECLERATION);
  }

  if (!checkValidType(typeToken)) {
    printParseError(p, "\"%s\" is not a valid type\n", typeToken->value);
    exit(EXIT_FAILURE);
  }
  consume(TOKEN_ASSIGN, p);

  switch (tkn->type) {
  case NODE_NUMBER:
    return handleNumberIdentifiers(p, typeToken, varName,
                                   NODE_IDENTIFIER_ASSIGNMENT);
  case TOKEN_STRING:
    return handleStringIdentifiers(p, typeToken, varName,
                                   NODE_IDENTIFIER_ASSIGNMENT);
  case TOKEN_IDEN:
    return handleIdenIdentifiers(p, typeToken, varName,
                                 NODE_IDENTIFIER_ASSIGNMENT);
  default:
    printParseError(p, "unexpected token %s\n", tokenNames[p->current->type]);
    exit(EXIT_FAILURE);
  }
}

void addStatementToBlock(AstNode *blockNode, AstNode *statement) {
  if (blockNode->type != NODE_BLOCK) {
    printf("Error: Attempting to add a statement to a non-block node.\n");
    exit(EXIT_FAILURE);
  }

  // Allocate or reallocate memory for the new statement
  blockNode->block.statements =
      realloc(blockNode->block.statements,
              (blockNode->block.statementCount + 1) * sizeof(AstNode *));

  // Add the statement to the block
  blockNode->block.statements[blockNode->block.statementCount++] = statement;
}

AstNode *parseBlockStmt(Parser *p) {

  if (p->current->type != TOKEN_LCURLY) {
    printParseError(p, "expected ->{<-but got %s\n",
                    tokenNames[p->current->type]);
    exit(EXIT_FAILURE);
  }

  consume(TOKEN_LCURLY, p);

  AstNode *blockNode = (AstNode *)malloc(sizeof(AstNode));

  if (!blockNode) {
    printf("unable to allocate memory for ast node\n");
    exit(EXIT_FAILURE);
  }

  blockNode->type = NODE_BLOCK;
  blockNode->block.statements = NULL;
  blockNode->block.statementCount = 0;

  enterScope(p->table);
  while (p->current->type != TOKEN_RCURLY && !parserIsAtEnd(p)) {
    AstNode *stmt = parseAst(p);
    addStatementToBlock(blockNode, stmt);
  }
  consume(TOKEN_RCURLY, p);
  exitScope(p->table);
  return blockNode;
}

AstNode *ifElseParser(Parser *p) {

  if (p->current->type != TOKEN_IF) {
    printParseError(p, "expected \"if\" but got %s\n", p->current->value);
    exit(EXIT_FAILURE);
  }
  consume(TOKEN_IF, p);
  consume(TOKEN_LPAREN, p);

  AstNode *ast = logical(p);
  consume(TOKEN_RPAREN, p);

  if (p->current->type != TOKEN_LCURLY) {
    printParseError(p, "expected { but got %s %s\n",
                    tokenNames[p->current->type], p->current->value);
    exit(EXIT_FAILURE);
  }

  AstNode *ifBlock = parseBlockStmt(p);

  AstNode *elseBlock = NULL;
  if (p->current->type == TOKEN_ELSE) {
    consume(TOKEN_ELSE, p);
    if (p->current->type != TOKEN_LCURLY) {
      printParseError(p, "expected { but got%s\n",
                      tokenNames[p->current->type]);
      exit(EXIT_FAILURE);
    }

    elseBlock = parseBlockStmt(p);
  }
  return newIfElseNode(ast, ifBlock, elseBlock);
}

// ------------------------parsing functions-------------------------------

AstNode *newFnParams(char *fnName, char *returnType, int paramsCount,
                     AstNode **params, AstNode *fnBody) {
  AstNode *node = (AstNode *)malloc(sizeof(AstNode));
  if (!node) {
    printf("failed allocating memory for the ast\n");
    exit(EXIT_FAILURE);
  }
  node->type = NODE_FUNCTION;
  node->function.defination.params = params;
  node->function.defination.returnType = strdup(returnType);
  node->function.defination.name = strdup(fnName);
  node->function.defination.body = fnBody;
  return node;
}

AstNode *parseFnParams(Parser *p) {
  if (p->current->type != TOKEN_IDEN) {
    printParseError(p, "expcted %s but got %s in function paramteres",
                    tokenNames[TOKEN_IDEN], tokenNames[p->current->type]);
    exit(EXIT_FAILURE);
  }
  if (isKeyword(p->current->value)) {
    printParseError(p, "cannot use  keyword %s as function parameters",
                    p->current->value);
    exit(EXIT_FAILURE);
  }
  char *paramName = strdup(p->current->value);
  consume(TOKEN_IDEN, p);

  consume(TOKEN_COLON, p);

  char *paramType = strdup(p->current->value);
  if (!checkValidType(p->current)) {
    printParseError(p, "unknown type parameter %s", p->current->value);
    exit(EXIT_FAILURE);
  }
  consume(TOKEN_IDEN, p);

  if (p->current->type != TOKEN_RPAREN) {
    consume(TOKEN_COMMA, p);

    printf("%s\n", p->current->value);
  }
  return newIdentifierNode(paramType, paramName, NULL, p, NODE_FUNCTION_PARAM);
}

AstNode *parseFunction(Parser *p) {
  if (p->current->type != TOKEN_FN || strcmp(p->current->value, "fn") != 0) {
    printParseError(p, "error occured  expected fn but got %s\n",
                    p->current->value);
    exit(EXIT_FAILURE);
  }
  consume(TOKEN_FN, p);
  char *fnName = p->current->value;
  consume(TOKEN_IDEN, p);
  consume(TOKEN_COLON, p);
  char *returnType = p->current->value;
  consume(TOKEN_IDEN, p);
  consume(TOKEN_LPAREN, p);

  int paramsSize = 2;
  AstNode **params = (AstNode **)malloc(sizeof(AstNode *) * 2);
  int paramsCount = 0;

  while (p->current->type != TOKEN_RPAREN) {
    if (paramsCount >= paramsSize) {
      params = (AstNode **)realloc(params, paramsSize *= 2);
    }
    AstNode *ast = parseFnParams(p);
    params[paramsCount++] = ast;
  }

  consume(TOKEN_RPAREN, p);
  AstNode *fnBody = parseBlockStmt(p);
  return newFnParams(fnName, returnType, paramsCount, params, fnBody);
}

AstNode *parseFnArguments(Parser *p) {
  switch (p->current->type) {
  case TOKEN_STRING: {
    AstNode *ast = newStringNode(p->current->value);
    consume(TOKEN_STRING, p);
    return ast;
  }
  case TOKEN_NUMBER: {
    AstNode *ast = newNumberNode(convertStrToDouble(p->current->value));
    consume(TOKEN_NUMBER, p);
    return ast;
  }

  case TOKEN_IDEN: {
    AstNode *ast = newIdentifierNode(NULL, p->current->value, NULL, p,
                                     NODE_IDENTIFIER_VALUE);
    consume(TOKEN_IDEN, p);
    return ast;
  }
  }
  printParseError(p, "unknown argument type");
  exit(EXIT_FAILURE);
}

AstNode *newFnCallNode(char *fnName, int argsCount, AstNode **callArgs) {
  AstNode *node = (AstNode *)malloc(sizeof(AstNode));
  if (!node) {
    printf("failed allocating memory for the ast\n");
    exit(EXIT_FAILURE);
  }

  node->type = NODE_FUNCTION_CALL;
  node->isCall = 1;
  node->function.call.argsCount = argsCount;
  node->function.call.name = strdup(fnName);
  node->function.call.args = callArgs;
  return node;
}

AstNode *functionCall(Parser *p) {
  char *fnName = strdup(p->current->value);
  consume(TOKEN_IDEN, p);
  consume(TOKEN_LPAREN, p);
  int capacity = 2;
  int argsCount = 0;
  AstNode **callArgs = (AstNode **)malloc(sizeof(AstNode) * capacity);
  while (p->current->type != TOKEN_RPAREN) {
    if (argsCount >= capacity) {
      capacity *= 2;
      callArgs = (AstNode **)realloc(callArgs, sizeof(AstNode) * capacity);
    }
    AstNode *argAst = parseFnArguments(p);
    if (p->current->type != TOKEN_RPAREN) {
      consume(TOKEN_COMMA, p);
    }
    callArgs[argsCount] = argAst;
    argsCount++;
  }
  consume(TOKEN_RPAREN, p);
  return newFnCallNode(fnName, argsCount, callArgs);
}
