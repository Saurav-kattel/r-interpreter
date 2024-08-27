#ifndef INTERPRETER_H_
#define INTERPRETER_H_

#include "parser.h"
#include "symbol.h"
double EvalAst(AstNode *);
void freeAst(AstNode *);
AstNode *parseAst(Parser *p);
#endif // INTERPRETER_H_
