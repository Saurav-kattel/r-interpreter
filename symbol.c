#include "symbol.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Create a new symbol
Symbol *createSymbol(char *name, SymbolType type, char *dataType, int scope) {
  Symbol *sym = (Symbol *)malloc(sizeof(Symbol));
  if (!sym) {
    perror("Unable to allocate memory for symbol");
    exit(EXIT_FAILURE);
  }
  sym->name = strdup(name);
  sym->type = type;
  sym->dataType = strdup(dataType);
  sym->scope = scope;
  sym->next = NULL;
  return sym;
}

// Initialize the symbol table
SymbolTable *initSymbolTable() {
  SymbolTable *table = (SymbolTable *)malloc(sizeof(SymbolTable));
  if (!table) {
    perror("Unable to allocate memory for symbol table");
    exit(EXIT_FAILURE);
  }
  for (int i = 0; i < TABLE_SIZE; ++i) {
    table->table[i] = NULL;
  }
  table->currentScope = 0;
  return table;
}

// Hash function for the symbol table
unsigned int hash(char *name) {
  unsigned int hash = 0;
  while (*name) {
    hash = (hash << 5) + *name++;
  }
  return hash % TABLE_SIZE;
}

// void insertSymbol(SymbolTable *table, Symbol *sym);

//  Add a symbol to the symbol table

// Add a symbol to the symbol table
void insertSymbol(SymbolTable *table, Symbol *sym) {
  // Calculate the index in the hash table
  unsigned int index = hash(sym->name);

  // Insert the symbol at the beginning of the linked list
  sym->next = table->table[index];
  table->table[index] = sym;
}

// Look up a symbol in the symbol table
Symbol *lookupSymbol(SymbolTable *table, char *name) {
  unsigned int index = hash(name);
  Symbol *sym = table->table[index];
  while (sym) {
    if (strcmp(sym->name, name) == 0 && sym->scope <= table->currentScope) {
      return sym;
    }
    sym = sym->next;
  }
  return NULL;
}

// Enter a new scope
void enterScope(SymbolTable *table) { table->currentScope++; }

// Exit the current scope
void exitScope(SymbolTable *table) {

  Symbol **currentTable = table->table;

  for (int i = 0; i < TABLE_SIZE; i++) {
    Symbol *sym = currentTable[i];
    Symbol *prev = NULL;
    while (sym) {
      if (sym->scope == table->currentScope) {
        if (prev) {
          prev->next = sym->next;
        } else {
          currentTable[i] = sym->next;
        }

        free(sym->name);
        free(sym);
      } else {
        prev = sym;
      }
      sym = sym->next;
    }
  }
  table->currentScope--;
}

// Free the symbol table
void freeSymbolTable(SymbolTable *table) {
  Symbol **currentTable = table->table;
  for (int i = 0; i < TABLE_SIZE; i++) {
    Symbol *sym = currentTable[i];
    while (sym) {
      Symbol *temp = sym;
      sym = sym->next;
      free(temp->name);
      free(temp);
    }
  }
  free(table);
}
