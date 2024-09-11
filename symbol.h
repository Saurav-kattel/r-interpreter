#ifndef SYMBOL_H_
#define SYMBOL_H_

#include "common.h"

#define INITIAL_CAPACITY 8
#define TABLE_SIZE 101 // Size of the hash table

typedef enum SymbolKind {
  SYMBOL_KIND_FUNCTION,
  SYMBOL_KIND_VARIABLES,
} SymbolKind;

typedef enum SymbolError {
  SYMBOL_TYPE_ERROR,
  SYMBOL_DUPLICATE_ERROR,
  SYMBOL_NOT_FOUND_ERROR,
  SYMBOL_MEM_ERROR,
  SYMBOL_ERROR_NONE,
} SymbolError;

static const char *errorName[] = {
    "symbol_type_error", "symbol_duplicate_error", "symbol_not_found_error",
    "symbol_mem_error",  "symbol_error_none",
};

SymbolError insertGlobalSymbol(SymbolContext *ctx, char *type, char *name,
                               Result *value, SymbolKind kind);

SymbolError insertFunctionSymbol(SymbolContext *ctx, char *name, char *type,
                                 int paramCount, FuncParams **params,
                                 SymbolKind kind, AstNode *body, int level);
SymbolTableEntry *lookupSymbol(SymbolContext *context, char *name,
                               SymbolKind kind);

SymbolError insertSymbol(SymbolContext *ctx, char *type, char *name,
                         Result *value, SymbolKind kind, int level);
SymbolError insertArray(SymbolContext *ctx, char *name, char *type, int size,
                        void *values, int isFixed);
SymbolError updateSymbolTableValue(SymbolTableEntry *entry, Result *value);
void enterScope(SymbolContext *);
void exitScope(SymbolContext *);
void updateParamWithArgs(SymbolTableEntry *sym, int index, Result *res);

SymbolContext *createSymbolContext(int capacity);
#endif // SYMBOL_H_
