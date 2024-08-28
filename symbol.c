#include "symbol.h"

#include <cstddef>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
// Create a new symbol
Symbol *createSymbol(char *name, SymbolType type, char *dataType, char *value,
                     int scope) {
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
  sym->value = strdup(value);
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

void insertSymbol(SymbolTable *table, Symbol *sym) {
  // Calculate the index in the hash table
  unsigned int index = hash(sym->name);
  // printSymbolTable(table);
  // Insert the symbol at the beginning of the linked list
  sym->next = table->table[index];
  table->table[index] = sym;
}

// Look up a symbol in the symbol table

Symbol *lookupSymbol(SymbolTable *table, char *name) {
  unsigned int index = hash(name);
  Symbol *sym = table->table[index];
  Symbol *result = NULL;

  // Iterate through the linked list at the hash table index
  while (sym) {

    // Check if the symbol's name matches and its scope is valid
    if (strcmp(sym->name, name) == 0 && sym->scope <= table->currentScope) {
      // Keep track of the symbol with the highest scope that is still valid
      if (!result || sym->scope > result->scope) {
        result = sym;
      }
    }
    printf("<name:%s value:%s scope:%d >\n", sym->name, sym->value, sym->scope);
    sym = sym->next;
  }

  // Return the found symbol or NULL if not found
  return result;
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
      free(temp->value);
      free(temp);
    }
  }
  free(table);
}

void printSymbolTable(SymbolTable *table) {
  printf("Symbol Table:\n");
  printf("Name\t\tType\t\tDataType\tScope\t\tValue\n");
  printf("----------------------------------------------------------\n");

  for (int i = 0; i < TABLE_SIZE; i++) {
    Symbol *sym = table->table[i];
    while (sym) {
      printf("%s\t\t%d\t\t%s\t\t%d\t\t%s\n", sym->name, sym->type,
             sym->dataType, sym->scope, sym->value ? sym->value : "NULL");
      sym = sym->next;
    }
  }
}

*/

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
  table->entries[table->size].value = value;
  table->entries[table->size].scope =
      table->currentScope; // Set the scope of the new symbol
  table->size++;
}

// Function to lookup a symbol in the symbol table
SymbolTableEntry *lookupSymbol(SymbolTable *table, char *symbol) {
  for (int i = table->size - 1; i >= 0; i--) {
    if (strcmp(table->entries[i].symbol, symbol) == 0 &&
        table->entries[i].scope <= table->currentScope) {
      return &table->entries[i];
    }
  }
  return NULL; // Symbol not found
}

// Function to enter a new scope
void enterScope(SymbolTable *table) { table->currentScope++; }

// Function to exit the current scope and remove all symbols declared in this
// scope
void exitScope(SymbolTable *table) {
  int i = table->size - 1;
  while (i >= 0) {

    if (table->entries[i].scope == table->currentScope) {
      for (int j = i; j < table->size - 1; j++) {
        table->entries[j] = table->entries[j + 1];
      }
      table->size--;
    } else {
      i--;
    }
  }
  table->currentScope--;
}

// Function to check the type of a symbol
char *getType(SymbolTable *table, char *symbol) {
  for (int i = table->size - 1; i >= 0; i--) {
    if (strcmp(table->entries[i].symbol, symbol) == 0 &&
        table->entries[i].scope <= table->currentScope) {
      return table->entries[i].type;
    }
  }
  return NULL; // Symbol not found
}
