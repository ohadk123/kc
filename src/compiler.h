#ifndef COMPILER_H
#define COMPILER_H

#include "sema.h"
#include "statement.h"
#include "token.h"

struct _TranslationUnit {
    String fileName;
    String input;
    TokensList tokens;
    StmtList ast;
    Scope globalSymbolTable;
};

__attribute__((__noreturn__)) void compile_error(String fileName, Location place, const char *fmt, ...);

#endif // COMPILER_H
