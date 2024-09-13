
#include "parser.h"
#include "common.h"
#include "interpreter.h"
#include "lexer.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *getNodeType(int nodeType) {
  switch (nodeType) {
  case NODE_STRING_LITERAL: {
    return "string";
  }
  case NODE_NUMBER: {
    return "number";
  }
  default: {
    return "nan";
  }
  }
}

void populateTokens(Parser *p, Lexer *lex, int initialCapacity, int *size) {
  // Allocate initial memory for tokens
  //
  p->tokens = (Token **)malloc(sizeof(Token *) * initialCapacity);

  if (p->tokens == NULL) {
    fprintf(stderr, "Memory allocation failed\n");
    exit(EXIT_FAILURE);
  }

  *size = 0; // Initialize sizepa
  Token *tkn = GetNextToken(lex);

  while (tkn->type != TOKEN_EOF) {
    // Check if more space is needed
    if (*size >= initialCapacity) {
      initialCapacity += 100;
      Token **newTokens =
          (Token **)realloc(p->tokens, sizeof(Token *) * initialCapacity);
      if (newTokens == NULL) {
        fprintf(stderr, "Memory reallocation failed\n");
        exit(EXIT_FAILURE);
      }
      p->tokens = newTokens;
    }
    // Assign token and increment size
    p->tokens[*size] = tkn;
    (*size)++;
    // Get the next token
    tkn = GetNextToken(lex);
  }

  // Add the EOF token
  if (*size >= initialCapacity) {
    initialCapacity += 100;
    Token **newTokens =
        (Token **)realloc(p->tokens, sizeof(Token *) * initialCapacity);
    if (newTokens == NULL) {
      fprintf(stderr, "Memory reallocation failed\n");
      exit(EXIT_FAILURE);
    }
    p->tokens = newTokens;
  }

  p->tokens[*size] = tkn;
  (*size)++;
}
// initializing the parser
Parser *InitParser(Lexer *lex, SymbolContext *ctx) {
  Parser *p = (Parser *)calloc(1, sizeof(Parser));
  if (p == NULL) {
    printf("unable to allocate parser");
    exit(EXIT_FAILURE);
  }
  p->lex = lex;
  int size = 0;
  p->level = 0;
  p->idx = 0;
  int initialCapacity = 100;
  populateTokens(p, lex, initialCapacity, &size);
  p->ctx = ctx;
  p->current = p->tokens[0];
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
    printError(p->current, "unexpected token  %s expected token %s\n",
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

int isKeyword(char *name) {
  int arraySize = 6;
  const char *keywords[] = {"number", "string", "if",      "fn",
                            "else",   "for",    "println", "readIn"};
  for (int i = 0; i < arraySize; i++) {
    if (strcmp(keywords[i], name) == 0) {
      return 1;
    }
  }
  return 0;
}

void printError(Token *tkn, const char *s, ...) {

  va_list args;
  va_start(args, s);

  // Get the location from the token
  Loc *loc = tkn->loc;

  // Print the filename andloc number with color
  printf(MAGENTA "%s" RESET, loc->file_name);
  printf(GREEN "::" RESET);
  printf(BLUE "%d" RESET, loc->row); // Use token's row forloc number
  printf(GREEN "::" RESET);
  printf(RED "Error-> " RESET);
  fflush(stdout);

  // Print the error message with color
  fprintf(stderr, YELLOW); // Set text color to yellow for the error message
  vfprintf(stderr, s, args);
  fprintf(stderr, RESET); // Reset text color

  va_end(args);

  // Print the context based on the token's location
  // printContext(tkn);
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

AstNode *newArrayNode(char *name, char *type, int isFixed, int actualSize,
                      AstNode *size, AstNode **elements, Loc loc) {
  AstNode *node = (AstNode *)calloc(1, sizeof(AstNode));
  if (!node) {
    printf("unable to allocate new ast node\n");
    exit(EXIT_FAILURE);
  }

  node->loc = loc;
  node->type = NODE_ARRAY_INIT;
  node->array.isFixed = isFixed;
  node->array.name = strdup(name);
  node->array.type = strdup(type);
  node->array.actualSize = actualSize;
  node->array.arraySize = size;
  node->array.elements = elements;
  return node;
}

AstNode *newArrayElmAccessNode(AstNode *index, char *name, Loc loc) {
  AstNode *node = (AstNode *)malloc(sizeof(AstNode));
  if (!node) {
    printf("unable to allocate new ast node\n");
    exit(EXIT_FAILURE);
  }
  node->loc = loc;
  node->type = NODE_ARRAY_ELEMENT_ACCESS;
  node->arrayElm.name = strdup(name);
  node->arrayElm.index = index;
  return node;
};

AstNode *newArrayElmAssignNode(char *name, AstNode *index, AstNode *value,
                               Loc loc) {
  AstNode *node = (AstNode *)malloc(sizeof(AstNode));
  if (!node) {
    printf("unable to allocate new ast node\n");
    exit(EXIT_FAILURE);
  }
  node->loc = loc;
  node->type = NODE_ARRAY_ELEMENT_ASSIGN;
  node->arrayElm.value = value;
  node->arrayElm.index = index;
  node->arrayElm.name = strdup(name);
  return node;
}

AstNode *newArrayDeclNode(char *name, char *type, int isFixed, AstNode *size,
                          Loc loc) {
  AstNode *node = (AstNode *)malloc(sizeof(AstNode));
  if (!node) {
    printf("unable to allocate new ast node\n");
    exit(EXIT_FAILURE);
  }

  node->loc = loc;
  node->type = NODE_ARRAY_DECLARATION;
  node->array.isDeclaration = 1;
  node->array.isFixed = isFixed;
  node->array.name = strdup(name);
  node->array.type = strdup(type);
  node->array.arraySize = size;
  node->array.elements = NULL;
  return node;
}

AstNode *newWhileNode(AstNode *condition, AstNode *body, Loc loc) {
  AstNode *node = (AstNode *)malloc(sizeof(AstNode));
  if (!node) {
    printf("unable to allocate new ast node\n");
    exit(EXIT_FAILURE);
  }
  node->loc = loc;
  node->type = NODE_WHILE_LOOP;
  node->whileLoop.body = body;
  node->whileLoop.condition = condition;
  return node;
}

AstNode *newBreakNode(Loc loc) {
  AstNode *node = (AstNode *)malloc(sizeof(AstNode));
  if (!node) {
    printf("unable to allocate new ast node\n");
    exit(EXIT_FAILURE);
  }
  node->loc = loc;
  node->type = NODE_BREAK;
  return node;
}

AstNode *newContinueNode(Loc loc) {
  AstNode *node = (AstNode *)malloc(sizeof(AstNode));
  if (!node) {
    printf("unable to allocate new ast node\n");
    exit(EXIT_FAILURE);
  }
  node->loc = loc;
  node->type = NODE_CONTNUE;
  return node;
}

AstNode *newPrintNode(AstNode **stmts, int currentSize, Loc loc) {
  AstNode *node = (AstNode *)malloc(sizeof(AstNode));
  if (!node) {
    printf("unable to allocate new ast node\n");
    exit(EXIT_FAILURE);
  }
  node->loc = loc;
  node->type = NODE_FUNCTION_PRINT;
  node->print.statments = stmts;
  node->print.statementCount = currentSize;
  return node;
}

AstNode *newForLoopNode(AstNode *initalizer, AstNode *conditon, AstNode *icrDcr,
                        AstNode *loopBody, Loc loc) {
  AstNode *node = (AstNode *)malloc(sizeof(AstNode));
  if (!node) {
    printf("unable to allocate new ast node\n");
    exit(EXIT_FAILURE);
  }
  node->loc = loc;
  node->type = NODE_FOR_LOOP;
  node->loopFor.icrDcr = icrDcr;
  node->loopFor.condition = conditon;
  node->loopFor.initializer = initalizer;
  node->loopFor.loopBody = loopBody;
  return node;
}

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

// creates and returns new ast for block stmt;

AstNode *newReturnNode(AstNode *expression, int nodeType) {
  AstNode *node = (AstNode *)malloc(sizeof(AstNode));
  if (!node) {
    printf("unable to allocate new ast node\n");
    exit(EXIT_FAILURE);
  }
  node->type = nodeType;
  node->expr = expression;
  return node;
}

AstNode *newReadInNode(int nodeType, char *type) {
  AstNode *node = (AstNode *)malloc(sizeof(AstNode));
  if (!node) {
    printf("unable to allocate new ast node\n");
    exit(EXIT_FAILURE);
  }
  node->type = nodeType;
  node->read.type = strdup(type);
  free(type);
  return node;
}

// returns the identifier ast from the provided argumentes

AstNode *newIdentifierNode(char *type, char *name, AstNode *value, Parser *p,
                           int nodeType, int isParam, Loc locNo) {
  AstNode *node = (AstNode *)malloc(sizeof(AstNode));
  if (!node) {
    printf("Unable to allocate new AST node\n");
    exit(EXIT_FAILURE);
  }

  int isDeceleration = (nodeType == NODE_IDENTIFIER_DECLERATION) ? 1 : 0;
  node->loc = *p->current->loc;
  node->type = nodeType;

  node->identifier.value = value;

  node->identifier.type = type ? strdup(type) : NULL;
  if (type && !node->identifier.type) {
    printf("Memory allocation failed\n");
    free(node);
    exit(EXIT_FAILURE);
  }
  node->identifier.name = strdup(name);
  if (!node->identifier.name) {
    printf("Memory allocation failed\n");
    free(node->identifier.type);
    free(node);
    exit(EXIT_FAILURE);
  }

  node->isParam = isParam;
  node->identifier.isDeceleration = isDeceleration;

  return node;
}

// returns the binary ast from the provided argumentes
AstNode *newBinaryNode(TokenType op, AstNode *left, AstNode *right, Loc loc) {

  AstNode *node = (AstNode *)malloc(sizeof(AstNode));
  if (!node) {
    printf("unable to allocate new ast node\n");
    exit(EXIT_FAILURE);
  }
  node->type = NODE_BINARY_OP;
  node->loc = loc;
  node->binaryOp.op = op;
  node->binaryOp.left = left;
  node->binaryOp.right = right;
  return node;
}
// returns the number ast from the provided argumentes
AstNode *newNumberNode(double value, Loc loc) {
  AstNode *node = (AstNode *)malloc(sizeof(AstNode));
  if (!node) {
    printf("unable to allocate new ast node\n");
    exit(EXIT_FAILURE);
  }
  node->type = NODE_NUMBER;
  node->loc = loc;
  node->number = value;
  return node;
}
// returns the unary ast from the provided argumentes

AstNode *newUnaryNode(TokenType type, AstNode *right, Loc loc) {
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

AstNode *newStringNode(char *value, Loc loc) {
  AstNode *node = (AstNode *)malloc(sizeof(AstNode));
  if (!node) {
    printf("Unable to allocate new AST node\n");
    exit(EXIT_FAILURE);
  }
  node->loc = loc;
  node->type = NODE_STRING_LITERAL;
  node->stringLiteral.value = strdup(value); // Duplicate the string value
  if (!node->stringLiteral.value) {
    printf("Memory allocation failed\n");
    free(node);
    exit(EXIT_FAILURE);
  }

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
    return newStringNode(tkn->value, *tkn->loc);
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
          p->current->type == TOKEN_MODULO ||
          p->current->type == TOKEN_DIVIDE || p->current->type == TOKEN_DOT)) {
    Token *tkn = p->current;
    consume(tkn->type, p);
    AstNode *right = factor(p);
    node = newBinaryNode(tkn->type, node, right,
                         *tkn->loc); // Pass tkn->type instead of NODE_BINARY_OP
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
    node = newBinaryNode(tkn->type, node, right, *tkn->loc);
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

  switch (tkn->type) {
  case TOKEN_NUMBER: {
    double value = convertStrToDouble(tkn->value);
    consume(TOKEN_NUMBER, p);
    return newNumberNode(value, *tkn->loc);
  }
  case TOKEN_LPAREN: {
    consume(TOKEN_LPAREN, p);
    AstNode *node = logical(p);
    consume(TOKEN_RPAREN, p);
    return node;
  }

  case TOKEN_IDEN: {

    if (parserPeek(p)->type == TOKEN_LPAREN) {
      return functionCall(p);
    }

    if (parserPeek(p)->type == TOKEN_LSQUARE) {
      return parseArray(p);
    }

    // to parse the variable that was assigned as a value
    AstNode *node = newIdentifierNode("", tkn->value, NULL, p,
                                      NODE_IDENTIFIER_VALUE, 0, *tkn->loc);
    consume(TOKEN_IDEN, p);
    return node;
  }

  case TOKEN_STRING: {
    AstNode *node = newStringNode(p->current->value, *tkn->loc);
    consume(TOKEN_STRING, p);
    return node;
  }

  case TOKEN_READ_IN: {
    AstNode *node = parseReadIn(p);
    return node;
  }

  case TOKEN_FOR: {
    return parseForLoop(p);
  }
  default:
    printError(p->current,
               "Unexpected token %s with value '%s'. Expected number, "
               "parenthesis, "
               "or identifier.\n",
               tokenNames[tkn->type], tkn->value);
    exit(EXIT_FAILURE);
  }
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
    node = newBinaryNode(tkn->type, node, right, *tkn->loc);
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
    node = newBinaryNode(tkn->type, node, right, *tkn->loc);
  }
  return node;
}

AstNode *unary(Parser *p) {
  if (p->current && (p->current->type == TOKEN_NOT)) {
    Token *tkn = p->current;
    consume(tkn->type, p);
    AstNode *right = unary(p);
    return newUnaryNode(tkn->type, right, *tkn->loc);
  }
  return relational(p);
}

AstNode *handleNumberIdentifiers(Parser *p, Token *typeToken, char *varName,
                                 int nodeType) {
  AstNode *valueNode = logical(p);
  if (nodeType == NODE_IDENTIFIER_MUTATION) {
    return newIdentifierNode("number", varName, valueNode, p, nodeType, 0,
                             *typeToken->loc);
  }
  return newIdentifierNode(typeToken->value, varName, valueNode, p, nodeType, 0,
                           *typeToken->loc);
}

AstNode *handleStringIdentifiers(Parser *p, Token *typeToken, char *varName,
                                 int nodeType) {
  AstNode *valueNode = logical(p);

  if (nodeType == NODE_IDENTIFIER_MUTATION) {
    return newIdentifierNode("string", varName, valueNode, p, nodeType, 0,
                             *typeToken->loc);
  }
  return newIdentifierNode(typeToken->value, varName, valueNode, p, nodeType, 0,
                           *typeToken->loc);
}

AstNode *handleIdenIdentifiers(Parser *p, Token *typeToken, char *varName,
                               int nodeType) {
  AstNode *valueNode = logical(p);
  return newIdentifierNode(typeToken->value, varName, valueNode, p, nodeType, 0,
                           *typeToken->loc);
}

AstNode *handleIdenReadIn(Parser *p, Token *typeToken, char *varName,
                          int nodeType) {
  AstNode *valueNode = logical(p);
  return newIdentifierNode(typeToken->value, varName, valueNode, p, nodeType, 0,
                           *typeToken->loc);
}
AstNode *handleReadInIdentiers(Parser *p, Token *typeToken, char *varname,
                               int nodeType) {
  AstNode *valueNode = logical(p);
  return newIdentifierNode(typeToken->value, varname, valueNode, p, nodeType, 0,
                           *typeToken->loc);
}
// handles  variables

AstNode *varDecleration(Parser *p) {
  Token *tkn = p->current;

  char *varName = strdup(tkn->value);
  if (!varName) {
    fprintf(stderr, "Memory allocation failed\n");
    exit(EXIT_FAILURE);
  }

  if (isKeyword(varName)) {
    printError(p->current,
               "cannot use keyword as variable \"%s\" is a keyword\n", varName);
    free(varName);
    exit(EXIT_FAILURE);
  }

  consume(TOKEN_IDEN, p);

  if (p->current->type == TOKEN_ASSIGN) {
    consume(TOKEN_ASSIGN, p);
    Token newToken;
    newToken.value = strdup(p->current->value);

    if (!newToken.value) {
      fprintf(stderr, "Memory allocation failed\n");
      free(varName);
      exit(EXIT_FAILURE);
    }

    newToken.type = p->current->type;
    newToken.loc = p->current->loc;

    AstNode *node;

    switch (p->current->type) {
    case TOKEN_NUMBER:
      node = handleNumberIdentifiers(p, &newToken, varName,
                                     NODE_IDENTIFIER_MUTATION);
      break;
    case TOKEN_STRING:
      node = handleStringIdentifiers(p, &newToken, varName,
                                     NODE_IDENTIFIER_MUTATION);
      break;
    case TOKEN_IDEN:
      node = handleIdenIdentifiers(p, &newToken, varName,
                                   NODE_IDENTIFIER_MUTATION);
      break;
    case TOKEN_READ_IN: {
      node = handleReadInIdentiers(p, &newToken, varName,
                                   NODE_IDENTIFIER_MUTATION);
      break;
    }
    default:
      printError(p->current, "unknown token \"%s\" \n",
                 tokenNames[p->current->type]);
      free(newToken.value);
      free(varName);
      exit(EXIT_FAILURE);
    }
    free(varName);
    free(newToken.value);
    return node;
  }

  consume(TOKEN_COLON, p);
  Token *typeToken = p->current;
  consume(TOKEN_IDEN, p);

  if (p->current->type == TOKEN_SEMI_COLON) {
    AstNode *node =
        newIdentifierNode(typeToken->value, varName, NULL, p,
                          NODE_IDENTIFIER_DECLERATION, 0, *typeToken->loc);
    free(varName);
    return node;
  }

  if (!checkValidType(typeToken)) {
    printError(p->current, "\"%s\" is not a valid type\n", typeToken->value);
    free(varName);
    exit(EXIT_FAILURE);
  }
  consume(TOKEN_ASSIGN, p);

  AstNode *node;
  switch (tkn->type) {
  case TOKEN_NUMBER:
    node = handleNumberIdentifiers(p, typeToken, varName,
                                   NODE_IDENTIFIER_ASSIGNMENT);
    break;
  case TOKEN_STRING:
    node = handleStringIdentifiers(p, typeToken, varName,
                                   NODE_IDENTIFIER_ASSIGNMENT);
    break;
  case TOKEN_IDEN:
    node = handleIdenIdentifiers(p, typeToken, varName,
                                 NODE_IDENTIFIER_ASSIGNMENT);
    break;
  case TOKEN_READ_IN: {

    node = parseReadIn(p);
    break;
  }
  default:
    printError(p->current, "unexpected token %s\n",
               tokenNames[p->current->type]);
    free(varName);
    exit(EXIT_FAILURE);
  }
  free(varName);
  return node;
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
    printError(p->current, "expected ->{<-but got %s\n",
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
  while (p->current->type != TOKEN_RCURLY && !parserIsAtEnd(p)) {
    AstNode *stmt = parseAst(p);
    addStatementToBlock(blockNode, stmt);
  }
  consume(TOKEN_RCURLY, p);

  return blockNode;
}

AstNode *ifElseParser(Parser *p) {

  if (p->current->type != TOKEN_IF) {
    printError(p->current, "expected \"if\" but got %s\n", p->current->value);
    exit(EXIT_FAILURE);
  }
  consume(TOKEN_IF, p);
  consume(TOKEN_LPAREN, p);

  AstNode *ast = logical(p);
  consume(TOKEN_RPAREN, p);

  if (p->current->type != TOKEN_LCURLY) {
    printError(p->current, "expected { but got %s %s\n",
               tokenNames[p->current->type], p->current->value);
    exit(EXIT_FAILURE);
  }

  AstNode *ifBlock = parseBlockStmt(p);

  AstNode *elseBlock = NULL;
  if (p->current->type == TOKEN_ELSE) {
    consume(TOKEN_ELSE, p);
    if (p->current->type != TOKEN_LCURLY) {
      printError(p->current, "expected { but got%s\n",
                 tokenNames[p->current->type]);
      exit(EXIT_FAILURE);
    }

    elseBlock = parseBlockStmt(p);
  }
  return newIfElseNode(ast, ifBlock, elseBlock);
}

// ------------------------parsing functions-------------------------------

AstNode *newFunctionDefination(Parser *p, char *fnName, char *returnType,
                               int paramsCount, FuncParams **params,
                               AstNode *fnBody) {
  AstNode *node = (AstNode *)malloc(sizeof(AstNode));
  if (!node) {
    printf("failed allocating memory for the ast\n");
    exit(EXIT_FAILURE);
  }
  node->type = NODE_FUNCTION;
  node->loc = *p->current->loc;
  node->isParam = 1;
  node->function.defination.params = params;
  node->function.defination.returnType = strdup(returnType);
  node->function.defination.name = strdup(fnName);
  node->function.defination.paramsCount = paramsCount;
  node->function.defination.body = fnBody;
  return node;
}

FuncParams *parseFnParams(Parser *p) {
  Loc loc = *p->current->loc;
  if (p->current->type != TOKEN_IDEN) {
    printError(p->current, "expcted %s but got %s in function paramteres",
               tokenNames[TOKEN_IDEN], tokenNames[p->current->type]);
    exit(EXIT_FAILURE);
  }
  if (isKeyword(p->current->value)) {
    printError(p->current, "cannot use  keyword %s as function parameters",
               p->current->value);
    exit(EXIT_FAILURE);
  }
  Token *paramName = p->current;
  consume(TOKEN_IDEN, p);

  consume(TOKEN_COLON, p);

  Token *paramType = p->current;

  if (!checkValidType(p->current)) {
    printError(p->current, "unknown type parameter %s", p->current->value);
    exit(EXIT_FAILURE);
  }

  consume(TOKEN_IDEN, p);

  if (p->current->type != TOKEN_RPAREN) {
    consume(TOKEN_COMMA, p);
  }

  FuncParams *param = calloc(1, sizeof(FuncParams));
  param->name = strdup(paramName->value);
  param->type = strdup(paramType->value);
  return param;
}

AstNode *parseFunction(Parser *p) {
  if (p->current->type != TOKEN_FN || strcmp(p->current->value, "fn") != 0) {
    printError(p->current, "error occured  expected fn but got %s\n",
               p->current->value);
    exit(EXIT_FAILURE);
  }
  consume(TOKEN_FN, p);
  char *fnName = p->current->value;
  consume(TOKEN_IDEN, p);

  consume(TOKEN_LPAREN, p);

  int paramsSize = 2;
  FuncParams **params = (FuncParams **)malloc(sizeof(FuncParams *) * 2);
  int paramsCount = 0;

  while (p->current->type != TOKEN_RPAREN) {
    if (paramsCount >= paramsSize) {
      params = (FuncParams **)realloc(params, paramsSize *= 2);
    }
    FuncParams *param = parseFnParams(p);
    params[paramsCount++] = param;
  }
  consume(TOKEN_RPAREN, p);

  consume(TOKEN_ARROW, p);
  char *returnType = p->current->value;
  consume(TOKEN_IDEN, p);
  AstNode *fnBody = parseBlockStmt(p);
  return newFunctionDefination(p, fnName, returnType, paramsCount, params,
                               fnBody);
}

AstNode *parseFnArguments(Parser *p) {
  Loc loc = *p->current->loc;
  switch (p->current->type) {
  case TOKEN_STRING: {
    AstNode *ast = newStringNode(p->current->value, loc);
    consume(TOKEN_STRING, p);
    return ast;
  }
  case TOKEN_NUMBER: {
    AstNode *ast = newNumberNode(convertStrToDouble(p->current->value), loc);
    consume(TOKEN_NUMBER, p);
    return ast;
  }

  case TOKEN_IDEN: {
    AstNode *ast = newIdentifierNode(NULL, p->current->value, NULL, p,
                                     NODE_IDENTIFIER_VALUE, 0, loc);
    consume(TOKEN_IDEN, p);
    return ast;
  }
  }
  printError(p->current, "unknown argument type");
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

  AstNode **callArgs = (AstNode **)malloc(sizeof(AstNode *) * capacity);
  while (p->current->type != TOKEN_RPAREN) {
    if (argsCount >= capacity) {
      capacity *= 2;
      callArgs = (AstNode **)realloc(callArgs, sizeof(AstNode *) * capacity);
    }
    AstNode *argAst = parseFnArguments(p);
    if (p->current->type != TOKEN_RPAREN) {
      consume(TOKEN_COMMA, p);
    }
    callArgs[argsCount] = argAst;
    argsCount++;
  }
  consume(TOKEN_RPAREN, p);
  AstNode *node = newFnCallNode(fnName, argsCount, callArgs);
  free(fnName);
  return node;
}

AstNode *parseReturn(Parser *p) {
  Token *tkn = p->current;
  if (tkn->type != TOKEN_RETURN) {
    printError(p->current, "excpted %s but got %s\n", tokenNames[TOKEN_RETURN],
               tokenNames[p->current->type]);
    exit(EXIT_FAILURE);
  }
  consume(TOKEN_RETURN, p);
  AstNode *expression = logical(p);
  return newReturnNode(expression, NODE_RETURN);
}

AstNode *parsePrint(Parser *p) {
  Token *tkn = p->current;

  if (tkn->type != TOKEN_PRINT) {
    printError(p->current, "excpted function println but got %s", tkn->value);
    free(tkn->value);
    free(tkn);
    exit(EXIT_FAILURE);
  }
  consume(TOKEN_PRINT, p);
  consume(TOKEN_LPAREN, p);

  int initalCapacity = 5;
  int currentSize = 0;
  AstNode **stmts = (AstNode **)malloc(sizeof(AstNode *) * initalCapacity);

  while (p->current->type != TOKEN_RPAREN) {
    if (currentSize >= initalCapacity) {
      initalCapacity += 5;
      stmts = (AstNode **)realloc(stmts, sizeof(AstNode *) * (initalCapacity));
    }

    AstNode *stmt = logical(p);
    if (p->current->type == TOKEN_COMMA &&
        parserPeek(p)->type != TOKEN_LPAREN) {
      consume(TOKEN_COMMA, p);
    }
    stmts[currentSize] = stmt;
    currentSize++;
  }

  consume(TOKEN_RPAREN, p);
  return newPrintNode(stmts, currentSize, *tkn->loc);
}

AstNode *parseReadIn(Parser *p) {

  if (p->current->type != TOKEN_READ_IN) {
    printError(p->current, "expected token %s but got %s\n",
               tokenNames[TOKEN_READ_IN], tokenNames[p->current->type]);
    exit(EXIT_FAILURE);
  }
  consume(TOKEN_READ_IN, p);

  consume(TOKEN_LPAREN, p);
  char *type = strdup(p->current->value);
  if (!checkValidType(p->current)) {
    printError(p->current, "unknown type parameter %s\n", type);
    free(type);
    exit(EXIT_FAILURE);
  }
  consume(TOKEN_IDEN, p);
  consume(TOKEN_RPAREN, p);
  return newReadInNode(NODE_FUNCTION_READ_IN, type);
}

AstNode *parseForLoop(Parser *p) {
  Loc loc = *p->current->loc;
  if (p->current->type != TOKEN_FOR) {
    printError(p->current, "expected for but got %s",
               tokenNames[p->current->type]);
    exit(EXIT_FAILURE);
  }
  consume(TOKEN_FOR, p);
  consume(TOKEN_LPAREN, p);

  AstNode *initializer = varDecleration(p);
  consume(TOKEN_SEMI_COLON, p);

  AstNode *condition = logical(p);
  consume(TOKEN_SEMI_COLON, p);

  AstNode *icrDcr = varDecleration(p);
  consume(TOKEN_RPAREN, p);

  if (p->current->type != TOKEN_LCURLY) {
    printError(p->current, "expected { but got %s",
               tokenNames[p->current->type]);
    exit(EXIT_FAILURE);
  }
  AstNode *loopBody = parseBlockStmt(p);
  return newForLoopNode(initializer, condition, icrDcr, loopBody, loc);
}

AstNode *parseBreakNode(Parser *p) {
  Loc loc = *p->current->loc;
  if (p->current->type != TOKEN_BREAK) {
    printError(p->current, "expected break but got %s",
               tokenNames[p->current->type]);
    exit(EXIT_FAILURE);
  }
  consume(TOKEN_BREAK, p);
  return newBreakNode(loc);
}

AstNode *parseContinueNode(Parser *p) {
  Loc loc = *p->current->loc;
  if (p->current->type != TOKEN_CONTINUE) {
    printError(p->current, "expected break but got %s",
               tokenNames[p->current->type]);
    exit(EXIT_FAILURE);
  }
  consume(TOKEN_CONTINUE, p);
  return newContinueNode(loc);
}

AstNode *parseWhileNode(Parser *p) {
  Loc loc = *p->current->loc;
  if (p->current->type != TOKEN_WHILE) {
    printError(p->current, "expected for but got %s",
               tokenNames[p->current->type]);
    exit(EXIT_FAILURE);
  }
  consume(TOKEN_WHILE, p);
  consume(TOKEN_LPAREN, p);
  AstNode *condition = logical(p);
  consume(TOKEN_RPAREN, p);
  AstNode *body = parseBlockStmt(p);
  return newWhileNode(condition, body, loc);
}

// parses fixed size arrays
void parseFixedArray(Parser *p, AstNode ***elements, Token *type,
                     int *actualSize) {

  int currentSize = 0;
  int capacity = 10;

  *elements = (AstNode **)calloc(capacity, sizeof(AstNode *));
  capacity *= 2;
  *elements = (AstNode **)realloc(*elements, sizeof(AstNode *) * capacity);
  if (elements == NULL) {
    printf("cannot allocated enough memory for array elements of size");
    exit(EXIT_FAILURE);
  }

  while (p->current->type != TOKEN_RCURLY) {
    if (currentSize >= capacity) {
      capacity *= 2;
      *elements = (AstNode **)realloc(*elements, sizeof(AstNode *) * capacity);
      if (elements == NULL) {
        printf("cannot allocated enough memory for array elements of size");
        exit(EXIT_FAILURE);
      }
    }

    AstNode *ast = logical(p);
    char *astType = getNodeType(ast->type);

    if (strcmp(astType, type->value) != 0) {
      printError(type, "cannot insert type of %s in array of type %s", astType,
                 type->value);
      exit(EXIT_FAILURE);
    }

    (*elements)[currentSize] = ast;

    if (parserPeek(p)->type != TOKEN_RCURLY &&
        p->current->type == TOKEN_COMMA) {
      consume(TOKEN_COMMA, p);
    }
    currentSize++;
  }
  (*actualSize) = currentSize;
}

void parseDynamicArray(Parser *p, AstNode ***elements, Token *type,
                       int *actualSize) {

  int currentSize = 0;
  int capacity = 10;

  *elements = (AstNode **)calloc(capacity, sizeof(AstNode *));
  if (*elements == NULL) {
    printf("cannot allocated enough memory for array *elements ");
    exit(EXIT_FAILURE);
  }

  while (p->current->type != TOKEN_RCURLY) {

    // reallocating the new mem size if the array is full

    if (currentSize >= capacity) {
      capacity *= 2;
      *elements = (AstNode **)realloc(elements, sizeof(AstNode *) * capacity);
      if (*elements == NULL) {
        printf("cannot allocated enough memory for array *elements ");
        exit(EXIT_FAILURE);
      }
    }

    // consume the comma if the current token is comma but it is not in  the
    // end

    AstNode *ast = logical(p);
    char *astType = getNodeType(ast->type);
    if (strcmp(astType, type->value) != 0) {
      printError(type, "cannot insert type of %s in array of type %s", astType,
                 type->value);
      exit(EXIT_FAILURE);
    }

    if (p->current->type == TOKEN_COMMA &&
        parserPeek(p)->type != TOKEN_RCURLY) {
      consume(TOKEN_COMMA, p);
    }

    (*elements)[currentSize] = ast;
    currentSize++;
  }
  (*actualSize) = currentSize;
}

AstNode *parseArray(Parser *p) {

  AstNode **elements = NULL;
  Token *name = p->current;
  Token *type = NULL;
  AstNode *arraySize = NULL;
  int isFixed = 0;
  int actualSize = 0;

  // array name
  consume(TOKEN_IDEN, p);
  //[
  consume(TOKEN_LSQUARE, p);

  if (p->current->type == TOKEN_RSQUARE) {
    isFixed = 0;
  } else {
    arraySize = logical(p);
    isFixed = 1;
  }

  //]
  consume(TOKEN_RSQUARE, p);

  if (p->current->type == TOKEN_ASSIGN) {
    consume(TOKEN_ASSIGN, p);
    AstNode *value = logical(p);
    char *nodeType = getNodeType(value->type);
    return newArrayElmAssignNode(name->value, arraySize, value, *name->loc);
  } else if (p->current->type == TOKEN_SEMI_COLON ||
             p->current->type == TOKEN_RPAREN ||
             p->current->type == TOKEN_COMMA) {
    AstNode *node =
        newArrayElmAccessNode(arraySize, name->value, *p->current->loc);
    return node;
  }

  //:
  consume(TOKEN_COLON, p);

  // type of array
  type = p->current;
  if (!checkValidType(type)) {
    printError(type, "unknown type param %s\n", type->value);
    exit(EXIT_FAILURE);
  }
  consume(TOKEN_IDEN, p);

  if (p->current->type == TOKEN_SEMI_COLON) {
    return newArrayDeclNode(name->value, type->value, isFixed, arraySize,
                            *name->loc);
  }
  // =
  consume(TOKEN_ASSIGN, p);

  //{

  consume(TOKEN_LCURLY, p);

  if (isFixed) {
    parseFixedArray(p, &elements, type, &actualSize);
  } else {
    parseDynamicArray(p, &elements, type, &actualSize);
  }

  consume(TOKEN_RCURLY, p);

  return newArrayNode(name->value, type->value, isFixed, actualSize, arraySize,
                      elements, *name->loc);
}
