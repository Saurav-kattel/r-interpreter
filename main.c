

#include "interpreter.h"
#include "lexer.h"
#include "parser.h"
#include "symbol.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void shiftArgs(int *argc, char **argv) { (*argc) = (*argc) - 1; }

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
  Lexer *lex = InitLexer(file_content);
  Parser *p = InitParser(lex);
  SymbolTable *table = (SymbolTable *)malloc(sizeof(SymbolTable));

  while (p->current->type != TOKEN_EOF) {
    AstNode *ast = parseAst(p, table);
    if (ast->type != NODE_IDENTIFIER) {
      double result = EvalAst(ast);
      printf("result is %.3lf\n", result);
    }
    freeAst(ast);
  }
  free(p);

  free(lex);
  fclose(fp);
  free(file_content);
  return 0;
}
