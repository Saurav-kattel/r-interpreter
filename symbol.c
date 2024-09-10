
#include "symbol.h"
#include "common.h"
#include "parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *inferTypeFromResult(Result *res) {
  if (res->NodeType == NODE_STRING_LITERAL) {
    return "string";
  }
  if (res->NodeType == NODE_NUMBER) {
    return "number";
  }
  return "nan";
}

SymbolContext *createSymbolContext(int capacity) {
  SymbolContext *ctx = (SymbolContext *)malloc(sizeof(*ctx));

  // Initialize global table
  ctx->globalTable = (SymbolTable *)malloc(sizeof(SymbolTable));
  ctx->globalTable->capacity = capacity;
  ctx->globalTable->size = 0;
  ctx->globalTable->entries = (SymbolTableEntry **)malloc(
      sizeof(SymbolTableEntry *) * ctx->globalTable->capacity);

  // Initialize the stack
  ctx->stack = (Stack *)malloc(sizeof(Stack));
  ctx->stack->capacity = capacity;
  ctx->stack->frameCount = 0;
  ctx->stack->frames =
      (StackFrame **)malloc(sizeof(StackFrame *) * ctx->stack->capacity);

  return ctx;
}

SymbolTableEntry *lookupLocalScope(SymbolTable *scope, char *name,
                                   SymbolKind kind) {
  for (int j = 0; j < scope->size; j++) {

    SymbolTableEntry *entry = scope->entries[j];
    if (strcmp(entry->symbol, name) == 0) {
      switch (kind) {
      case SYMBOL_KIND_FUNCTION: {
        if (entry->isFn) {
          return entry;
        }
        break;
      case SYMBOL_KIND_VARIABLES:
        if (!entry->isFn) {
          return entry;
        }
        break;
      }
      default:
        printf("unknown lookup type");
        exit(1);
      }
    }
  }
  return NULL;
}

// enters the array and it's value in symbol table
SymbolError insertArray(SymbolContext *ctx, char *name, char *type, int size,
                        void *values, int isFixed) {

  SymbolTable *gblTable = ctx->globalTable;

  if (gblTable->size >= gblTable->capacity) {
    gblTable->capacity *= 2;
    gblTable->entries = realloc(gblTable->entries, sizeof(SymbolTableEntry *) *
                                                       gblTable->capacity);
    if (gblTable->entries == NULL) {
      return SYMBOL_MEM_ERROR; // Handle realloc failure
    }
  }

  // Allocate memory for a new SymbolTableEntry
  gblTable->entries[gblTable->size] =
      (SymbolTableEntry *)malloc(sizeof(SymbolTableEntry));
  if (gblTable->entries[gblTable->size] == NULL) {
    return SYMBOL_MEM_ERROR; // Handle malloc failure
  }

  if (strcmp(type, "string") == 0) {
    ctx->globalTable->entries[ctx->globalTable->size]->value = values;
  } else {
    ctx->globalTable->entries[ctx->globalTable->size]->value = values;
  }

  ctx->globalTable->entries[ctx->globalTable->size]->symbol = strdup(name);
  ctx->globalTable->entries[ctx->globalTable->size]->type = strdup(type);
  ctx->globalTable->entries[ctx->globalTable->size]->arraySize = size;
  ctx->globalTable->entries[ctx->globalTable->size]->isArray = 1;
  ctx->globalTable->entries[ctx->globalTable->size]->isGlobal = 1;
  ctx->globalTable->entries[ctx->globalTable->size]->isFixed = isFixed;
  ctx->globalTable->size++;
  return SYMBOL_ERROR_NONE;
}

// searches the iden or fn name inside global context
SymbolTableEntry *lookupGlobalScope(SymbolTable *table, char *name,
                                    SymbolKind kind) {
  return lookupLocalScope(table, name, kind);
}

// searches for the entry in both scopes
SymbolTableEntry *lookupSymbol(SymbolContext *context, char *name,
                               SymbolKind kind) {

  for (int i = context->stack->frameCount - 1; i >= 0; i--) {
    StackFrame *frame = context->stack->frames[i];
    SymbolTable *localScope = frame->localTable;
    SymbolTableEntry *entry = lookupLocalScope(localScope, name, kind);

    if (entry) {

      return entry;
    }
  }

  return lookupGlobalScope(context->globalTable, name, kind);
}

// entires the entry in global scope
SymbolError insertGlobalSymbol(SymbolContext *ctx, char *type, char *name,
                               Result *value, SymbolKind kind) {

  SymbolTableEntry *entry = lookupSymbol(ctx, name, kind);

  if (entry) {
    return SYMBOL_DUPLICATE_ERROR;
  }

  int size = ctx->globalTable->size;
  if (size >= ctx->globalTable->capacity) {
    ctx->globalTable->capacity *= 2;
    ctx->globalTable->entries =
        realloc(ctx->globalTable->entries,
                sizeof(SymbolTableEntry *) * ctx->globalTable->capacity);
  }

  ctx->globalTable->entries[size] = malloc(sizeof(SymbolTableEntry));
  ctx->globalTable->entries[size]->symbol = strdup(name);
  ctx->globalTable->entries[size]->type = strdup(type);
  ctx->globalTable->entries[size]->isGlobal = 1;

  if (value == NULL) {
    ctx->globalTable->entries[size]->value = NULL;
    ctx->globalTable->size++;
    return SYMBOL_ERROR_NONE;
  }

  char *inferredType = inferTypeFromResult(value);

  if (strcmp(inferredType, type) != 0) {
    return SYMBOL_TYPE_ERROR;
  }
  if (strcmp(type, "string") == 0) {
    ctx->globalTable->entries[size]->value = strdup((char *)value->result);
    free(value->result);
    free(value);
  }
  if (strcmp(type, "number") == 0) {
    ctx->globalTable->entries[size]->value = (double *)value->result;
    free(value);
  }
  ctx->globalTable->size++;
  return SYMBOL_ERROR_NONE;
}

// entirs the function in global scope
SymbolError insertFunction(SymbolTable *gblTable, char *name, char *type,
                           int paramCount, FuncParams **params, AstNode *body,
                           SymbolKind kind) {
  if (gblTable->size >= gblTable->capacity) {
    gblTable->capacity *= 2;
    gblTable->entries = realloc(gblTable->entries, sizeof(SymbolTableEntry *) *
                                                       gblTable->capacity);
    if (gblTable->entries == NULL) {
      return SYMBOL_MEM_ERROR; // Handle realloc failure
    }
  }

  // Allocate memory for a new SymbolTableEntry
  gblTable->entries[gblTable->size] =
      (SymbolTableEntry *)malloc(sizeof(SymbolTableEntry));
  if (gblTable->entries[gblTable->size] == NULL) {
    return SYMBOL_MEM_ERROR; // Handle malloc failure
  }

  // Set up the function entry
  gblTable->entries[gblTable->size]->isFn = 1;
  gblTable->entries[gblTable->size]->symbol = strdup(name);
  gblTable->entries[gblTable->size]->type = strdup(type);
  gblTable->entries[gblTable->size]->function.parameterCount = paramCount;
  gblTable->entries[gblTable->size]->function.body = body;

  // Allocate memory for the parameter array in the function symbol
  gblTable->entries[gblTable->size]->function.params =
      malloc(paramCount * sizeof(FuncParams *));
  if (gblTable->entries[gblTable->size]->function.params == NULL) {
    return SYMBOL_MEM_ERROR; // Handle malloc failure for params
  }

  // Copy the parameter pointers (shallow copy)
  for (int i = 0; i < paramCount; i++) {
    gblTable->entries[gblTable->size]->function.params[i] = params[i];
  }

  // Increment the size after successful insertion
  gblTable->size++;

  return SYMBOL_ERROR_NONE;
}

// entires the function entry in local scope
SymbolError insertFnStack(SymbolContext *ctx, char *name, char *type,
                          int paramCount, FuncParams **params, AstNode *body,
                          SymbolKind kind) {

  if (ctx->stack->capacity <= ctx->stack->frameCount) {
    ctx->stack->capacity *= 2;
    ctx->stack->frames = (StackFrame **)realloc(
        ctx->stack->frames, sizeof(StackFrame *) * ctx->stack->capacity);
    if (ctx->stack->frames == NULL) {
      return SYMBOL_MEM_ERROR;
    }
  }

  // Allocate memory for the new StackFrame if necessary
  ctx->stack->frames[ctx->stack->frameCount] =
      (StackFrame *)malloc(sizeof(StackFrame));
  if (!ctx->stack->frames[ctx->stack->frameCount]) {
    return SYMBOL_MEM_ERROR;
  }

  // Initialize the local symbol table in the new stack frame
  StackFrame *newFrame = ctx->stack->frames[ctx->stack->frameCount];
  newFrame->localTable = (SymbolTable *)malloc(sizeof(SymbolTable));
  if (!newFrame->localTable) {
    return SYMBOL_MEM_ERROR;
  }

  // Initialize local table's capacity and entries
  newFrame->localTable->capacity = 10; // or some initial value
  newFrame->localTable->size = 0;
  newFrame->localTable->entries = (SymbolTableEntry **)malloc(
      sizeof(SymbolTableEntry *) * newFrame->localTable->capacity);
  if (!newFrame->localTable->entries) {
    return SYMBOL_MEM_ERROR;
  }

  SymbolTable *localTable = newFrame->localTable;
  if (localTable->capacity <= localTable->size) {
    localTable->capacity *= 2;
    localTable->entries = (SymbolTableEntry **)realloc(
        localTable->entries, sizeof(SymbolTableEntry *) * localTable->capacity);

    if (localTable->entries == NULL) {
      return SYMBOL_MEM_ERROR;
    }
  }

  // Insert the function into the local table
  insertFunction(localTable, name, type, paramCount, params, body, kind);

  ctx->stack->frameCount++;
  return SYMBOL_ERROR_NONE;
}

// handles the functions symbol entry
SymbolError insertFunctionSymbol(SymbolContext *ctx, char *name, char *type,
                                 int paramCount, FuncParams **params,
                                 SymbolKind kind, AstNode *body, int level) {

  SymbolTableEntry *entry = lookupSymbol(ctx, name, kind);

  if (entry) {
    return SYMBOL_DUPLICATE_ERROR;
  }

  // Insertion into global or local stack based on the level
  if (level > 0) {
    int status = insertFunction(ctx->globalTable, name, type, paramCount,
                                params, body, kind);
    return status;
  } else {
    int status = insertFnStack(ctx, name, type, paramCount, params, body, kind);
    return status;
  }

  return SYMBOL_ERROR_NONE;
}

// TODO: local scope symbol entry not implemented
SymbolError insertSymbol(SymbolContext *ctx, char *type, char *name,
                         Result *value, SymbolKind kind) {

  return insertGlobalSymbol(ctx, type, name, value, kind);
}

// updates the value of particular symbol inside the context
SymbolError updateSymbolTableValue(SymbolTableEntry *entry, Result *value) {

  switch (value->NodeType) {
  case NODE_STRING_LITERAL: {
    if (strcmp(entry->type, "string") != 0) {
      return SYMBOL_TYPE_ERROR;
    }
    entry->value = strdup((char *)value->result);
    break;
  }
  case NODE_NUMBER: {
    if (strcmp(entry->type, "number") != 0) {
      return SYMBOL_TYPE_ERROR;
    }
    entry->value = (double *)value->result;
    break;
  }
  }
  return SYMBOL_ERROR_NONE;
}
