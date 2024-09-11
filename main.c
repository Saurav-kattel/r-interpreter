
#include "common.h"
#include "interpreter.h"
#include "lexer.h"
#include "parser.h"
#include "symbol.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Program {
  AstNode **program;
  int size;
  int capacity;
} Program;

void shiftArgs(int *argc, char **argv) { (*argc) = (*argc) - 1; }

void freeTokens(Token *tkn) {
  if (tkn) {
    if (tkn->loc) {
      free(tkn->loc->file_name);
      free(tkn->loc);
    }

    if (tkn->value) {
      free(tkn->value);
    }
    free(tkn);
  }
}

void addToProgram(AstNode *ast, Program *pg) {
  if (!pg) {
    printf("program is null\n");
    exit(EXIT_FAILURE);
  }
  if (pg->size >= pg->capacity) {
    pg->capacity *= 2;
    AstNode **newProgram =
        (AstNode **)realloc(pg->program, pg->capacity * sizeof(AstNode *));
    if (!newProgram) {
      printf("Memory allocation failed\n");
      exit(EXIT_FAILURE);
    }
    pg->program = newProgram;
  }
  pg->program[pg->size] = ast;
  pg->size++;
}

void freeTable(SymbolTable *table) {
  if (!table) {
    return;
  }

  for (int i = 0; i < table->size; i++) {
    SymbolTableEntry *entry = table->entries[i];
    if (!entry) {
      continue;
    }

    // Free the symbol
    free(entry->symbol);

    // Free the array (if applicable)
    if (entry->isArray) {
      if (strcmp(entry->type, "string") == 0) {
        for (int j = 0; j < entry->arraySize; j++) {
          free(((char **)entry->value)[j]);
        }
      }
      free(entry->value);
    }

    // Free the value (if it's a string and not an array)
    if (!entry->isArray && strcmp(entry->type, "string") == 0) {
      free(entry->value);
    }

    // Free the type
    free(entry->type);

    // Free function-related memory
    if (entry->isFn) {
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

    // Now, free the SymbolTableEntry itself
    free(entry);
  }

  // Free the array of entries
  free(table->entries);
  free(table);
}

void freeSymbolContext(SymbolContext *ctx) {

  for (int i = 0; i < ctx->stack->frameCount; i++) {
    freeTable(ctx->stack->frames[i]->localTable);
  }

  free(ctx->stack->frames);
  free(ctx->stack);
  freeTable(ctx->globalTable);
  free(ctx);
}

int main(int argc, char **argv) {
  FILE *fp;

  if (argc <= 1) {
    printf("Please enter file name....\n Usage: ./main <filename>\n");
    exit(EXIT_FAILURE);
  }

  // shifiting the cli arguments
  shiftArgs(&argc, argv);
  char *file_name = argc[argv];

  fp = fopen(file_name, "r");

  if (fp == NULL) {
    printf("unable to open file");
    exit(EXIT_FAILURE);
  }

  fseek(fp, 0, SEEK_END);
  long file_size = ftell(fp);
  rewind(fp);

  // allocating the memory for file
  char *file_content = (char *)malloc(file_size + 1);
  if (!file_content) {
    fclose(fp);
    printf("failed allocating file");
    exit(EXIT_FAILURE);
  }

  // reading the file content into string
  fread(file_content, sizeof(char), file_size, fp);
  file_content[file_size] = '\0';

  Lexer *lex = InitLexer(file_content, file_name);

  SymbolContext *ctx = createSymbolContext(100);

  Parser *p = InitParser(lex, ctx);

  Program *prog = (Program *)calloc(1, sizeof(Program));

  if (prog == NULL) {
    printf("buy more ram\n");
    exit(EXIT_FAILURE);
  }

  prog->size = 0;
  prog->capacity = 10000;
  prog->program = (AstNode **)calloc(prog->capacity, sizeof(AstNode));

  while (p->current->type != TOKEN_EOF) {

    AstNode *ast = parseAst(p);
    if (ast) {
      addToProgram(ast, prog);
      prog->size++;
    }
  }

  for (int i = 0; i < prog->size; i++) {
    if (prog->program[i]) {
      Result *res = EvalAst(prog->program[i], p);
      freeResult(res);
    }
  }

  for (int i = 0; i < prog->size; i++) {
    freeAst(prog->program[i]);
  }

  free(prog->program);
  free(prog);

  for (int i = 0; i < p->size; i++) {
    freeTokens(p->tokens[i]);
  }

  free(p->tokens);
  free(p->lex->source);
  free(p->lex->filename);
  free(p->lex);
  freeSymbolContext(p->ctx);
  free(p);
  fclose(fp);
  free(file_content);
  return 0;
}
