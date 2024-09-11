#ifndef COMMON_H_
#define COMMON_H_

#include "lexer.h"

// Forward declare AstNode for use in SymbolTableEntry
typedef struct AstNode AstNode;

typedef struct FuncParams {
  char *type;
  char *name;
  int isFunc;  // if the parameter is a
  int isArray; // if the parameter is an array
  void *value;
} FuncParams;

typedef struct SymbolTableEntry {
  char *symbol; // name of the symbol
  char *type;   // Type field for each symbol table entry
  int isGlobal; // Flag to determine if it is global
  // arrays
  int isArray;   // determines the the symbol entry is array or just variable
  int isFixed;   // determies if the array is dynamic or fixed size array
  int arraySize; // keeps the recored of the size of array;

  // functions
  int isFn;
  struct {
    int parameterCount; // Number of parameters
    FuncParams **params;
    AstNode *body;
  } function;

  void *value; // Use a void pointer to store values of different types
} SymbolTableEntry;

typedef struct SymbolTable {
  struct SymbolTableEntry **entries;
  int size;
  int capacity;
  int currentScope;
} SymbolTable;

typedef struct StackFrame {
  SymbolTable *localTable; // Local symbol table for this frame
  int stackLevel;          // Scope level of the frame
} StackFrame;

typedef struct Stack {
  StackFrame **frames; // Array of pointers to stack frames
  int frameCount;      // Number of active frames
  int capacity;        // Capacity of the stack
} Stack;

typedef struct SymbolContext {
  SymbolTable *globalTable;
  Stack *stack;
} SymbolContext;

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
  int level;

  SymbolContext *ctx;
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
          FuncParams **params;
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
