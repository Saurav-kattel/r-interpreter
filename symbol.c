
#include "symbol.h"
#include "common.h"
#include "interpreter.h"
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

void freeSymbolArray(SymbolTableEntry *entry) {

  if (strcmp(entry->type, "string") == 0) {
    for (int j = 0; j < entry->arraySize; j++) {
      free(((char **)entry->value)[j]);
    }
  }
  free(entry->value);
}

void freeFnSymbol(SymbolTableEntry *entry) {

  if (entry->function.body) {
    freeAst(entry->function.body);
  }

  for (int i = 0; i < entry->function.parameterCount; i++) {
    free(entry->function.params[i]->name);
    if (strcmp(entry->function.params[i]->type, "string") == 0) {
      free(entry->function.params[i]->value);
    }
    free(entry->function.params[i]->type);
  }

  free(entry->function.params);
}

void exitScope(SymbolContext *ctx) {
  StackFrame *frame = ctx->stack->frames[ctx->stack->frameCount - 1];

  if (!frame) {
    return;
  }

  SymbolTable *table = frame->localTable;
  if (!table) {
    return;
  }

  for (int i = 0; i < table->size; i++) {
    SymbolTableEntry *entry = table->entries[i];

    if (!entry) {
      continue;
    }
    free(entry->symbol);

    if (strcmp(entry->type, "string") == 0) {
      free(entry->value);
    }

    if (entry->isArray) {
      freeSymbolArray(entry);
    }

    if (entry->isFn) {
      freeSymbolArray(entry);
    }

    free(entry->type);
  }
  free(table);
  free(frame);
  ctx->stack->frameCount--;
}

void enterScope(SymbolContext *ctx) {
  int frameCount = ctx->stack->frameCount;
  int capacity = ctx->stack->capacity;

  // Check if we need to increase the capacity of the stack
  if (capacity <= frameCount) {
    ctx->stack->capacity *= 2;
    ctx->stack->frames = realloc(ctx->stack->frames,
                                 sizeof(StackFrame *) * ctx->stack->capacity);
    if (!ctx->stack->frames) {
      // handle memory error
    }
  }

  // Create a new stack frame for the new scope
  ctx->stack->frames[frameCount] = calloc(1, sizeof(StackFrame));
  ctx->stack->frames[frameCount]->localTable = calloc(1, sizeof(SymbolTable));

  // Increment frame count since we've entered a new scope
  ctx->stack->frameCount++;
}
SymbolTableEntry *lookupLocalScope(SymbolTable *scope, char *name,
                                   SymbolKind kind) {
  for (int j = 0; j < scope->size; j++) {

    SymbolTableEntry *entry = scope->entries[j];
    if (strcmp(entry->symbol, name) == 0) {
      switch (kind) {
      case SYMBOL_KIND_FUNCTION:
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
    }

    // If we are looking for a variable, it could be a function parameter.
    if (entry->isFn && entry->function.parameterCount > 0) {
      // Check for parameters in the function's parameter list
      for (int k = 0; k < entry->function.parameterCount; k++) {
        FuncParams *param = entry->function.params[k];
        if (strcmp(param->name, name) == 0) {
          // Create a temporary SymbolTableEntry for the parameter
          SymbolTableEntry *paramEntry = malloc(sizeof(SymbolTableEntry));
          paramEntry->symbol = param->name;
          paramEntry->type = param->type;
          paramEntry->value = param->value;
          paramEntry->isArray = param->isArray;
          paramEntry->isFn = param->isFunc;
          return paramEntry; // Return the entry for the parameter
        }
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

SymbolError insertLocalSymbol(SymbolContext *ctx, char *type, char *name,
                              Result *value, SymbolKind kind, int level) {

  // Check if the symbol already exists
  SymbolTableEntry *entry = lookupSymbol(ctx, name, kind);
  if (entry) {
    return SYMBOL_DUPLICATE_ERROR;
  }

  int currentFrame = ctx->stack->frameCount - 1;

  // Ensure the current frame is valid
  if (currentFrame < 0) {
    return SYMBOL_NOT_FOUND_ERROR;
  }

  StackFrame *frame = ctx->stack->frames[currentFrame];

  // Ensure the local table is initialized for the current frame
  if (frame->localTable == NULL) {
    frame->localTable = calloc(1, sizeof(SymbolTable));
    if (!frame->localTable) {
      return SYMBOL_MEM_ERROR;
    }
    // Initialize capacity
    frame->localTable->capacity = 4; // Starting with a small capacity
    frame->localTable->entries =
        malloc(sizeof(SymbolTableEntry *) * frame->localTable->capacity);
    if (!frame->localTable->entries) {
      return SYMBOL_MEM_ERROR;
    }
  }

  if (frame->localTable->capacity == 0) {

    frame->localTable->capacity = 4;
    frame->localTable->entries =
        malloc(sizeof(SymbolTableEntry *) * frame->localTable->capacity);
  }

  SymbolTable *locTable = frame->localTable;

  // If the frame's level is less than the current level, update the level
  // (i.e., entering a deeper scope)
  if (frame->stackLevel < level) {
    frame->stackLevel = level;
  }

  // Handle reallocation of the local table's entries if necessary

  // Handle reallocation of the local table's entries if necessary
  if (locTable->size >= locTable->capacity) {
    // Attempt to realloc with a temporary pointer to avoid overwriting in case
    // of failure
    size_t newCapacity = locTable->capacity * 2;
    SymbolTableEntry **temp =
        realloc(locTable->entries, sizeof(SymbolTableEntry *) * newCapacity);

    // Check for memory allocation failure
    if (!temp) {
      // Handle the memory allocation error
      fprintf(stderr, "Memory allocation error during realloc\n");
      return SYMBOL_MEM_ERROR;
    }

    // If successful, update the entries and capacity
    locTable->entries = temp;
    locTable->capacity = newCapacity;
  }

  // Allocate and initialize the new symbol table entry
  locTable->entries[locTable->size] = malloc(sizeof(SymbolTableEntry));

  if (!locTable->entries[locTable->size]) {
    return SYMBOL_MEM_ERROR;
  }

  // Copy the symbol and type
  locTable->entries[locTable->size]->type = strdup(type);
  locTable->entries[locTable->size]->symbol = strdup(name);
  locTable->entries[locTable->size]->isGlobal = 0;

  // Handle the case where the value is NULL (e.g., uninitialized variables)
  if (value == NULL) {
    locTable->entries[locTable->size]->value = NULL;
    locTable->size++;
    return SYMBOL_ERROR_NONE;
  }

  // Infer the type and check for type mismatch
  char *inferredType = inferTypeFromResult(value);
  if (strcmp(inferredType, type) != 0) {
    return SYMBOL_TYPE_ERROR;
  }

  // Handle value assignment based on type
  if (strcmp(type, "string") == 0) {
    locTable->entries[locTable->size]->value = strdup((char *)value->result);
    free(value->result); // Free the result as the value is now copied
    free(value);         // Free the Result structure
  } else if (strcmp(type, "number") == 0) {
    locTable->entries[locTable->size]->value = (double *)value->result;
    free(value); // Free the Result structure
  }

  // Increment the size of the local table
  locTable->size++;

  return SYMBOL_ERROR_NONE;
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

void updateParamWithArgs(SymbolTableEntry *sym, int index, Result *res) {
  if (res->NodeType == NODE_STRING_LITERAL) {
    sym->function.params[index]->value = strdup((char *)res->result);
  } else {
    sym->function.params[index]->value = (double *)res->result;
  }
};

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

SymbolError insertSymbol(SymbolContext *ctx, char *type, char *name,
                         Result *value, SymbolKind kind, int level) {
  if (level > 0) {
    return insertLocalSymbol(ctx, type, name, value, kind, level);
  }
  return insertGlobalSymbol(ctx, type, name, value, kind);
}

// updates the value of particular symbol inside the context
SymbolError updateSymbolTableValue(SymbolTableEntry *entry, Result *value) {

  switch (value->NodeType) {
  case NODE_STRING_LITERAL: {
    if (strcmp(entry->type, "string") != 0) {
      return SYMBOL_TYPE_ERROR;
    }
    free(entry->value);
    entry->value = strdup((char *)value->result);
    break;
  }
  case NODE_NUMBER:
    if (strcmp(entry->type, "number") != 0) {
      return SYMBOL_TYPE_ERROR;
    }
    entry->value = (double *)value->result;
    break;
  }
  freeResult(value);
  return SYMBOL_ERROR_NONE;
}
