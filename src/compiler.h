#ifndef COMPILER_H
#define COMPILER_H

#include "hashmap.h"
#include "statement.h"
#include "token.h"

typedef struct _Scope Scope;
struct _Scope {
    HashMap symbols;
    Scope *above;

    Type *retType;
    bool inLoop;
};

typedef struct {
    String fileName;
    String input;
    TokensList tokens;
    StmtList ast;
    Scope globalSymbols;
} TranslationUnit;

__attribute__((__noreturn__)) void compile_error(String fileName, Location place, const char *fmt, ...);

#endif // COMPILER_H
