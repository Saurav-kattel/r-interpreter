#if 1

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
  Lexer *lex = InitLexer(file_content, file_name);

  SymbolTable *table = createSymbolTable(100);
  Parser *p = InitParser(lex, table);

  printf("\n\n%s\n\n", file_content);

  while (p->current->type != TOKEN_EOF) {
    AstNode *ast = parseAst(p);
    if (ast) {
      if (ast->type != NODE_STRING_LITERAL && ast->type != NODE_IDENTIFIER) {
        Result *result = EvalAst(ast, p);
        printf("\n %.3lf\n", *(double *)result->result);
      } else if (ast->type == NODE_STRING_LITERAL) {
        printf("\n%s\n", ast->stringLiteral.value);
      } else if (ast->type == NODE_IDENTIFIER) {
        printf("\n<name: %s| value: %s| type:%s>\n\n", ast->identifier.name,
               ast->identifier.value, ast->identifier.type);
      }
    }
    freeAst(ast);
  }

  free(p);

  free(lex);
  fclose(fp);
  free(file_content);
  return 0;
}

#endif
