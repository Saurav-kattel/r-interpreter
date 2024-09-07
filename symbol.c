#include "symbol.h"
#include "common.h"
#include "parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void printSymbolTable(SymbolTable *table) {
  printf("\n\nSymbol Table:\n");
  printf("Name\t\tType\ttScope\tValue\tcurrentscope\n");
  printf("-------------------------------------------------\n");

  for (int i = 0; i < table->size; i++) {
    SymbolTableEntry sym = table->entries[i];
    if (!sym.isFn) {
      if (strcmp(sym.type, "string") == 0) {
        printf("%s\t\t%s\t%d\t%s\t\t%d\n", sym.symbol, sym.type, sym.scope,
               sym.value ? (char *)sym.value : "NULL", table->currentScope);
      } else if (strcmp(sym.type, "number") == 0) {
        printf("%s\t\t%s\t%d\t%lf\t\t%d\n", sym.symbol, sym.type, sym.scope,
               sym.value ? *(double *)sym.value : 0, table->currentScope);
      }
    }
  }
}

void printFnSymbolTable(SymbolTable *table) {
  printf("\n\nFN Symbol Table:\n");
  printf("Name\t\tType\tParamCount\tScope\tCurrentScope\n");
  printf("--------------------------------------------------------------\n");

  for (int i = 0; i < table->size; i++) {
    SymbolTableEntry sym = table->entries[i];
    if (sym.isFn) {
      printf("%s\t\t%s\t\t%d\t%d\t\t%d\n", sym.function.name,
             sym.function.returnType, sym.function.parameterCount,
             sym.function.scopeLevel, table->currentScope);
    }
  }
}

// Function to create a new symbol table

SymbolTable *createSymbolTable(int initialCapacity) {
  SymbolTable *table = (SymbolTable *)malloc(sizeof(SymbolTable));
  table->entries =
      (SymbolTableEntry *)malloc(initialCapacity * sizeof(SymbolTableEntry));
  table->size = 0;
  table->capacity = initialCapacity;
  table->currentScope = 0; // Initialize the current scope to 0

  // Explicitly initialize each entry in the table
  for (int i = 0; i < initialCapacity; i++) {
    table->entries[i].symbol = NULL;
    table->entries[i].arraySize = 0;
    table->entries[i].isArray = 0;
    table->entries[i].type = NULL;
    table->entries[i].value = NULL;
    table->entries[i].isFn = 0;
    table->entries[i].isParam = 0;
    table->entries[i].scope = 0;
    table->entries[i].function.name = NULL;
    table->entries[i].function.returnType = NULL;
    table->entries[i].function.parameterCount = 0;
    table->entries[i].function.parameters = NULL;
    table->entries[i].function.scopeLevel = 0;
    table->entries[i].function.functionBody = NULL;
  }

  return table;
}

// Function to insert a new symbol into the symbol table
void insertFnSymbol(SymbolTable *table, char *fnName, char *returnType,
                    AstNode **params, AstNode *fnBody, int paramCount) {

  // Check if the symbol is already declared in the current scope
  for (int i = table->size - 1; i >= 0; i--) {
    if (!table->entries[i].isFn) {
      continue;
    }
    if (strcmp(table->entries[i].function.name, fnName) == 0 &&
        table->entries[i].scope == table->currentScope) {
      printf("Error: Symbol '%s' is already declared in the current scope.\n",
             fnName);
      return;
    }
  }

  // Insert the new symbol
  if (table->size == table->capacity) {
    // Resize the table if it's full
    table->capacity *= 2;
    table->entries = (SymbolTableEntry *)realloc(
        table->entries, table->capacity * sizeof(SymbolTableEntry));
  }

  table->entries[table->size].function.name = strdup(fnName);
  table->entries[table->size].function.parameters = params;
  table->entries[table->size].function.returnType = strdup(returnType);
  table->entries[table->size].function.scopeLevel = table->currentScope;
  table->entries[table->size].function.parameterCount = paramCount;
  table->entries[table->size].function.functionBody = fnBody;
  table->entries[table->size].isFn = 1;
  table->entries[table->size].scope =
      table->currentScope; // Set the scope of the new symbol
  table->size++;
}

// Function to insert a new symbol into the symbol table

void insertSymbol(SymbolTable *table, char *symbol, char *type, Result *value,
                  int isParam) {
  // Check if the symbol is already declared in the current scope
  if (!isParam) {
    for (int i = table->size - 1; i >= 0; i--) {
      if (table->entries[i].isFn) {
        continue;
      }
      if (strcmp(table->entries[i].symbol, symbol) == 0 &&
          table->entries[i].scope == table->currentScope) {
        printf("Error: Symbol '%s' is already declared in the current scope.\n",
               symbol);
        return;
      }
    }
  }

  if (table->size == table->capacity) {
    table->capacity *= 2;
    SymbolTableEntry *new_entries = (SymbolTableEntry *)realloc(
        table->entries, table->capacity * sizeof(SymbolTableEntry));
    if (!new_entries) {
      // Handle reallocation failure
      fprintf(stderr, "Memory reallocation failed\n");
      exit(EXIT_FAILURE);
    }
    table->entries = new_entries;
  }

  table->entries[table->size].symbol = strdup(symbol);
  table->entries[table->size].type = strdup(type);

  if (value) {
    if (value->NodeType == NODE_STRING_LITERAL) {
      table->entries[table->size].value = strdup((char *)value->result);
    } else {
      table->entries[table->size].value = (double *)value->result;
    }
  } else {
    table->entries[table->size].value = NULL;
  }

  if (table->size == table->capacity) {
    table->capacity *= 2;
    table->entries = (SymbolTableEntry *)realloc(
        table->entries, table->capacity * sizeof(SymbolTableEntry));
    if (!table->entries) {
      fprintf(stderr, "Memory reallocation failed\n");
      exit(EXIT_FAILURE);
    }
  }

  table->entries[table->size].isParam = isParam;
  table->entries[table->size].scope =
      table->currentScope; // Set the scope of the new symbol
  table->size++;
}

SymbolTableEntry *lookupFnSymbol(SymbolTable *table, char *symbol) {

  for (int i = table->size - 1; i >= 0; i--) {
    if (table->entries[i].isFn) {
      int symbolMatches = strcmp(table->entries[i].function.name, symbol) == 0;
      int inScope =
          table->entries[i].function.scopeLevel <= table->currentScope;
      if (symbolMatches && inScope) {
        return &table->entries[i];
      }
    }
  }
  return NULL; // Symbol not found
}

SymbolTableEntry *lookupSymbol(SymbolTable *table, char *symbol, int param) {
  // Check for NULL poinnters4
  //

  if (table == NULL || symbol == NULL || table->entries == NULL) {
    return NULL; // Invalid input or empty table, return NULL
  }

  // Verify table size is within expected limits
  if (table->size <= 0) {
    return NULL; // Empty table or invalid size, return NULL
  }

  for (int i = table->size - 1; i >= 0; i--) {
    // Check if symbol is initialized
    if (table->entries[i].symbol == NULL) {
      continue; // Skip uninitialized symbols
    }

    int symbolMatches = strcmp(table->entries[i].symbol, symbol) == 0;

    int inScope = table->entries[i].scope <= table->currentScope;
    int isParam = table->entries[i].isParam;

    if (param) {
      if (symbolMatches && inScope && isParam) {
        return &table->entries[i];
      }
    } else {
      if (symbolMatches && inScope) {
        return &table->entries[i];
      }
    }
  }

  return NULL; // Symbol not found
}

// Function to enter a new scope
void enterScope(SymbolTable *table) { table->currentScope++; }

void exitScope(SymbolTable *table) {
  int i = 0;
  while (i < table->size) {
    if (table->entries[i].scope >= table->currentScope) {
      // Free dynamically allocated memory
      free(table->entries[i].symbol);
      free(table->entries[i].type);
      free(table->entries[i]
               .value); // Assuming the value is dynamically allocated

      if (table->entries[i].isFn) {
        // Free function-related fields
        free(table->entries[i].function.name);
        free(table->entries[i].function.returnType);

        // Free parameters if they exist
        for (int j = 0; j < table->entries[i].function.parameterCount; j++) {
          freeAst(table->entries[i]
                      .function.parameters[j]); // Assuming freeAst is the
                                                // function to free AST nodes
        }
        free(table->entries[i].function.parameters);
        freeAst(
            table->entries[i].function.functionBody); // Free the function body
      }

      // Shift the remaining entries
      for (int j = i; j < table->size - 1; j++) {
        table->entries[j] = table->entries[j + 1];
      }
      table->size--; // Reduce the table size after removing the entry

    } else {
      i++; // Move to the next entry only if no deletion happened
    }
  }

  table->currentScope--; // Exit the scope by decrementing the scope level
}
void updateSymbolTableValue(SymbolTable *table, char *varName, Result *value,
                            char *type) {

  SymbolTableEntry *sym = lookupSymbol(table, varName, 0);
  if (strcmp(sym->type, type) != 0) {
    printf("cannot assing type of %s to type of %s\n", type, sym->type);
    exit(EXIT_FAILURE);
  }

  if (!sym) {
    printf("variable %s not found \n", varName);
    exit(EXIT_FAILURE);
  }
  if (value != NULL) {
    if (strcmp(type, "string") == 0) {
      if (sym->value) {
        free(sym->value);
      }
      sym->value = strdup((char *)value->result);
    } else {
      sym->value = (double *)value->result;
    }
  } else {
    sym->value = NULL;
  }
  free(value);
}

void insertNumArraySymbol(SymbolTable *table, char *name, char *type, int size,
                          double *value, int isFixed) {
  // Check if the symbol is already declared in the current scope
  for (int i = table->size - 1; i >= 0; i--) {
    if (!table->entries[i].isArray) {
      continue;
    }
    if (strcmp(table->entries[i].symbol, name) == 0 &&
        table->entries[i].scope == table->currentScope) {
      printf("Error: Array '%s' is already declared in the current scope.\n",
             name);
      return;
    }
  }

  if (table->size == table->capacity) {
    table->capacity *= 2;
    SymbolTableEntry *new_entries = (SymbolTableEntry *)realloc(
        table->entries, table->capacity * sizeof(SymbolTableEntry));
    if (!new_entries) {
      // Handle reallocation failure
      fprintf(stderr, "Memory reallocation failed\n");
      exit(EXIT_FAILURE);
    }
    table->entries = new_entries;
  }

  if (size > 0 && value) {
    table->entries[table->size].value = (double *)malloc(sizeof(double) * size);
    for (int i = 0; i < size; i++) {

      if (value[i]) {
        ((double *)table->entries[table->size].value)[i] = value[i];
      }
    }
  }

  table->entries[table->size].symbol = strdup(name);
  table->entries[table->size].type = strdup(type);
  table->entries[table->size].isArray = 1;
  table->entries[table->size].isFixed = isFixed;
  table->entries[table->size].arraySize = size;
  table->entries[table->size].scope =
      table->currentScope; // Set the scope of the new symbol
  table->size++;
}

void insertStrArraySymbol(SymbolTable *table, char *name, char *type, int size,
                          char **value, int isFixed, int actualSize) {

  // Check if the symbol is already declared in the current scope
  for (int i = table->size - 1; i >= 0; i--) {
    if (!table->entries[i].isArray) {
      continue;
    }
    if (strcmp(table->entries[i].symbol, name) == 0 &&
        table->entries[i].scope == table->currentScope) {
      printf("Error: Array '%s' is already declared in the current scope.\n",
             name);
      return;
    }
  }

  if (table->size == table->capacity) {
    table->capacity *= 2;
    SymbolTableEntry *new_entries = (SymbolTableEntry *)realloc(
        table->entries, table->capacity * sizeof(SymbolTableEntry));
    if (!new_entries) {
      // Handle reallocation failure
      fprintf(stderr, "Memory reallocation failed\n");
      exit(EXIT_FAILURE);
    }
    table->entries = new_entries;
  }

  if (size > 0 && value) {
    table->entries[table->size].value = (char **)malloc(sizeof(char *) * size);
    for (int i = 0; i < actualSize; i++) {
      if (value[i]) {
        ((char **)table->entries[table->size].value)[i] = strdup(value[i]);
      }
    }
  }

  table->entries[table->size].symbol = strdup(name);
  table->entries[table->size].type = strdup(type);
  table->entries[table->size].isArray = 1;
  table->entries[table->size].arraySize = size;
  table->entries[table->size].isFixed = isFixed;
  table->entries[table->size].scope =
      table->currentScope; // Set the scope of the new symbol
  table->size++;
}
