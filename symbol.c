#include "symbol.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void printSymbolTable(SymbolTable *table) {
  printf("\n\nSymbol Table:\n");
  printf("Name\t\tType\ttScope\tValue\tcurrentscope\n");
  printf("----------------------------------------------------------\n");

  for (int i = 0; i < table->size; i++) {
    SymbolTableEntry sym = table->entries[i];

    printf("%s\t\t%s\t%d\t%s\t\t%d\n", sym.symbol, sym.type, sym.scope,
           sym.value ? (char *)sym.value : "NULL", table->currentScope);
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
  return table;
}

// Function to insert a new symbol into the symbol table
void insertSymbol(SymbolTable *table, char *symbol, char *type, void *value) {

  // Check if the symbol is already declared in the current scope
  for (int i = table->size - 1; i >= 0; i--) {
    if (strcmp(table->entries[i].symbol, symbol) == 0 &&
        table->entries[i].scope == table->currentScope) {
      printf("Error: Symbol '%s' is already declared in the current scope.\n",
             symbol);
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

  table->entries[table->size].symbol = strdup(symbol);
  table->entries[table->size].type = strdup(type);
  table->entries[table->size].value = strdup(value);

  table->entries[table->size].scope =
      table->currentScope; // Set the scope of the new symbol
  table->size++;
}

// Function to lookup a symbol in the symbol table

// Function to lookup a symbol in the symbol table, considering scope
SymbolTableEntry *lookupSymbol(SymbolTable *table, char *symbol) {
  for (int i = table->size - 1; i >= 0; i--) {
    int symbolMatches = strcmp(table->entries[i].symbol, symbol) == 0;
    int inScope = table->entries[i].scope <= table->currentScope;
    if (symbolMatches && inScope) {
      return &table->entries[i];
    }
  }
  return NULL; // Symbol not found
}

// Function to enter a new scope
void enterScope(SymbolTable *table) { table->currentScope++; }

void exitScope(SymbolTable *table) {

  int i = table->size - 1;
  while (i >= 0) {
    if (table->entries[i].scope == table->currentScope) {
      for (int j = i; j < table->size - 1; j++) {
        table->entries[j] = table->entries[j + 1];
      }
      table->size--;
    }
    i--;
  }

  table->currentScope--;
}

void updateSymbolTableValue(SymbolTable *table, char *varName, char *value,
                            char *type) {

  SymbolTableEntry *sym = lookupSymbol(table, varName);
  if (strcmp(sym->type, type) != 0) {
    printf("cannot assing type of %s to type of %s\n", type, sym->type);
    exit(EXIT_FAILURE);
  }

  if (!sym) {
    printf("variable %s not found \n", varName);
    exit(EXIT_FAILURE);
  }
  sym->value = strdup(value);
}
