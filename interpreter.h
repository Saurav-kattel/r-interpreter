#ifndef INTERPRETER_H_
#define INTERPRETER_H_

#include "common.h"
#include "parser.h"
#include "symbol.h"
Result *EvalAst(AstNode *, Parser *);
void freeAst(AstNode *);
AstNode *parseAst(Parser *p);
void printSymbolTable(SymbolTable *);
#endif // INTERPRETER_H_
void freeSymbolTable(SymbolTable *table);
void freeResult(Result *res);
