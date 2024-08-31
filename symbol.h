#ifndef SYMBOL_H_
#define SYMBOL_H_

#include "common.h"

#define INITIAL_CAPACITY 8
#define TABLE_SIZE 101 // Size of the hash table

// Enum for different symbol types
typedef enum { SYMBOL_VARIABLE, SYMBOL_FUNCTION, SYMBOL_TYPE } SymbolType;

SymbolTable *createSymbolTable(int initialCapacity);
SymbolTableEntry *lookupFnSymbol(SymbolTable *table, char *symbol);

void insertSymbol(SymbolTable *table, char *symbol, char *type, void *value);

SymbolTableEntry *lookupSymbol(SymbolTable *table, char *symbol);

void enterScope(SymbolTable *table);
void updateSymbolTableValue(SymbolTable *table, char *varName, char *value,
                            char *type);
void exitScope(SymbolTable *table);

#endif // SYMBOL_H_
