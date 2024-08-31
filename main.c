#if 1

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

  SymbolTable *table = createSymbolTable(100);
  Parser *p = InitParser(lex, table);
  printf("\n\n%s\n\n", file_content);

  Program *prog = (Program *)malloc(sizeof(Program));
  prog->size = 0;
  prog->capacity = 10;
  prog->program = (AstNode **)malloc(sizeof(AstNode) * prog->capacity);

  while (p->current->type != TOKEN_EOF) {
    AstNode *ast = parseAst(p);
    if (ast) {
      addToProgram(ast, prog);
    }
  }

  for (int i = 0; i < prog->size; i++) {
    EvalAst(prog->program[i], p);
  }

  printSymbolTable(p->table);
  free(p);
  free(lex);
  fclose(fp);
  free(file_content);
  return 0;
}

#endif
