#ifndef COMMON_H_
#define COMMON_H_

#include "lexer.h"

// Forward declare AstNode for use in SymbolTableEntry
typedef struct AstNode AstNode;

typedef struct SymbolTable {
  struct SymbolTableEntry *entries;
  int size;
  int capacity;
  int currentScope;

} SymbolTable;

typedef struct SymbolTableEntry {

  char *symbol;  // name of the symbol
  char *type;    // Type field for each symbol table entry
  void *value;   // Use a void pointer to store values of different types
  int isFn;      // determines the the symbol entry is function or just variable
  int isArray;   // determines the the symbol entry is array or just variable
  int isFixed;   // determies if the array is dynamic or fixed size array
  int arraySize; // keeps the recored of the size of array;
  int isParam; // determines the the symbol entry is function parameter or just
               // variable

  int scope; // Scope field for each symbol table entry

  struct {
    char *name;           // Name of the function
    char *returnType;     // Return type of the function (e.g., "int", "string")
    int parameterCount;   // Number of parameters
    AstNode **parameters; // Array of parameter entries (variables)
    // Symbol table for function's local variables
    int scopeLevel;
    AstNode *functionBody; // AST node representing the function's body
  } function;
} SymbolTableEntry;

typedef struct Result {
  int NodeType;
  void *result;
  int isBreak;
  int isContinue;
  int isReturn;
} Result;

typedef struct {
  Token *current;
  Lexer *lex;
  Token **tokens;
  int idx;
  int size;

  SymbolTable *table;
} Parser;
struct AstNode {
  int type;
  Loc loc;
  int isParam;
  double number;

  union {
    struct AstNode *expr;
    struct {
      AstNode **statments;
      int statementCount;
    } print;

    struct {
      AstNode *body;
      AstNode *condition;
    } whileLoop;

    struct {
      struct AstNode *left;
      struct AstNode *right;
      TokenType op;
    } binaryOp;

    struct {
      char *type;
    } read;
    struct {
      struct AstNode *right;
      TokenType op;
    } unaryOp;

    struct {
      char *name;
      char *type;
      struct AstNode *value;
      int isDeceleration;
    } identifier;

    struct {
      char *name;
      char *type;
      AstNode *arraySize;
      int isFixed;
      int isDeclaration;
      AstNode **elements;
      int actualSize;
    } array;

    struct {
      struct AstNode **statements;
      int statementCount;
    } block;

    struct {
      AstNode *initializer;
      AstNode *condition;
      AstNode *icrDcr;
      AstNode *loopBody;
    } loopFor;

    struct {
      struct AstNode *condition;
      struct AstNode *ifBlock;
      struct AstNode *elseBlock;
    } ifElseBlock;

    struct {
      char *value;
    } stringLiteral;

    struct {
      char *name;
      AstNode *index;
      AstNode *value;

    } arrayElm;

    struct {
      int isCall;
      union {
        struct {
          char *name;
          char *returnType;
          AstNode **params;
          int paramsCount;
          AstNode *body;
        } defination;

        struct {
          char *name;
          AstNode **args;
          int argsCount;
        } call;

      } function;
    };
  };
};

#endif // COMMON_H_
