#ifndef SYMBOL_H_
#define SYMBOL_H_

#include "common.h"

#define INITIAL_CAPACITY 8
#define TABLE_SIZE 101 // Size of the hash table

// Enum for different symbol types
typedef enum { SYMBOL_VARIABLE, SYMBOL_FUNCTION, SYMBOL_TYPE } SymbolType;

SymbolTable *createSymbolTable(int initialCapacity);
SymbolTableEntry *lookupFnSymbol(SymbolTable *table, char *symbol);

void insertSymbol(SymbolTable *table, char *symbol, char *type, Result *value,
                  int isParam);

void insertFnSymbol(SymbolTable *table, char *fnName, char *returnType,
                    AstNode **params, AstNode *fnBody, int capacity);

SymbolTableEntry *lookupSymbol(SymbolTable *table, char *symbol, int Param);

void enterScope(SymbolTable *table);
void updateSymbolTableValue(SymbolTable *table, char *varName, Result *value,
                            char *type);
void exitScope(SymbolTable *table);
void printFnSymbolTable(SymbolTable *);
void insertArraySymbol(SymbolTable *table, char *name, char *type, int size,
                       void *value);

#endif // SYMBOL_H_
