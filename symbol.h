#ifndef SYMBOL_H_
#define SYMBOL_H_

#define INITIAL_CAPACITY 8
#if 0
#define MAX_PARAM_COUNT 10
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

#if 0
// Struct for storing symbol information
typedef struct Symbol {
  char *name;      // Identifier name
  SymbolType type; // Type of symbol (variable, function, etc.)
  char *dataType;  // For variables, the type (e.g., int, float)
  int scope;
  char *value; // Scope level (e.g., global, local)
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
Symbol *createSymbol(char *name, SymbolType type, char *dataType, char *value,
                     int scope);
void enterScope(SymbolTable *table);
void printSymbolTable(SymbolTable *table);
void exitScope(SymbolTable *table);
#endif

typedef struct {
  char *symbol;
  char *type;  // Add a type field to each symbol table entry
  void *value; // Use a void pointer to store values of different types
  int scope;   // Add a scope field to each symbol table entry
} SymbolTableEntry;

// Define a structure to represent the symbol table
typedef struct {
  SymbolTableEntry *entries;
  int size;
  int capacity;
  int currentScope; // Keep track of the current scope
} SymbolTable;

#if 0
typedef struct SymbolTableEntry {
  char *name;
  void *value;
  char *type;
} SymbolTableEntry;

typedef struct Scope {
  SymbolTableEntry **symbols;
  int symbolCount;
  int capacity;
} Scope;

typedef struct SymbolTable {
  Scope **scopes;
  int scopeCount;
  int scopeCapacity;
} SymbolTable;
#endif

SymbolTable *createSymbolTable(int initialCapacity);

void insertSymbol(SymbolTable *table, char *symbol, char *type, void *value);

SymbolTableEntry *lookupSymbol(SymbolTable *table, char *symbol);

void enterScope(SymbolTable *table);
void updateSymbolTableValue(SymbolTable *table, char *varName, char *value,
                            char *type);
void exitScope(SymbolTable *table);

#if 0
SymbolTable *createSymbolTable();
SymbolTableEntry *lookupSymbol(SymbolTable *table, const char *name);
void addSymbolToCurrentScope(SymbolTable *table, char *name, void *value,
                             char *type);
void exitScope(SymbolTable *table);
void enterScope(SymbolTable *table);
void updateSymbolTableValue(SymbolTable *table, char *varName, char *value,
                            char *type);
SymbolTableEntry *lookupSymbol(SymbolTable *table, const char *name);
void printSymbolTable(SymbolTable *);
#endif
#endif // SYMBOL_H_
