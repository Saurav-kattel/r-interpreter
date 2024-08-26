#ifndef SYMBOL_H_
#define SYMBOL_H_

#if 0
#define MAX_PARAM_COUNT 10
#define TABLE_SIZE 100
typedef enum { SYMBOL_VARIABLE, SYMBOL_FUNCTION, SYMBOL_TYPE } SymbolType;

typedef struct Symbol {

  SymbolType type;

  char *name;
  // variables
  int dataType;
  int scope;
  int isInitialized;

  // for size of an array
  int arraySize;

  // functions
  int returnType;
  int paramTypes[MAX_PARAM_COUNT];
  int paramCount;

  // debuggig purposes
  char *fileName;
  int lineNumber;

  struct Symbol *next;
} Symbol;

typedef struct SymbolTable {
  Symbol *table[TABLE_SIZE]; // Hash table array
  int currentScope;          // Current scope level
} SymbolTable;
#endif

#define TABLE_SIZE 101 // Size of the hash table

// Enum for different symbol types
typedef enum { SYMBOL_VARIABLE, SYMBOL_FUNCTION, SYMBOL_TYPE } SymbolType;

// Struct for storing symbol information
typedef struct Symbol {
  char *name;      // Identifier name
  SymbolType type; // Type of symbol (variable, function, etc.)
  char *dataType;  // For variables, the type (e.g., int, float)
  int scope;       // Scope level (e.g., global, local)
  // Additional attributes can be added here
  struct Symbol *next; // For chaining in hash table
} Symbol;

typedef struct SymbolTable {
  Symbol *table[TABLE_SIZE]; // Hash table array
  int currentScope;          // Current scope level
} SymbolTable;

void insertSymbol(SymbolTable *table, Symbol *sym);
Symbol *lookupSymbol(SymbolTable *table, char *name);
void deleteSymbol(SymbolTable *table, char *name);
void updateSymbol(SymbolTable *table, char *name, int newType, int newValue);
int checkSymbolExistence(SymbolTable *table, char *name);
void listSymbols(SymbolTable *table, int scope);
Symbol *createSymbol(char *name, SymbolType type, char *dataType, int scope);

#endif // SYMBOL_H_
