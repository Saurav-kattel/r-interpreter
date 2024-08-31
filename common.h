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

typedef struct Result {
  int NodeType;
  void *result;
} Result;

typedef struct {
  Token *current;
  Lexer *lex;
  SymbolTable *table;
} Parser;

typedef struct SymbolTableEntry {
  char *symbol;
  char *type;  // Type field for each symbol table entry
  void *value; // Use a void pointer to store values of different types
  int isFn;
  int scope; // Scope field for each symbol table entry

  struct {
    char *name;         // Name of the function
    char *returnType;   // Return type of the function (e.g., "int", "string")
    int parameterCount; // Number of parameters
    struct SymbolTableEntry *
        *parameters; // Array of parameter entries (variables)
    // Symbol table for function's local variables
    int scopeLevel;        // Scope level of the function
    AstNode *functionBody; // AST node representing the function's body
  } function;
} SymbolTableEntry;

struct AstNode {
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
      struct AstNode *value;
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

    struct {
      char *name;
      char *returnType;
      AstNode **params;
      int paramsCount;
      AstNode *body;
    } function;
  };
};

#endif // COMMON_H_
