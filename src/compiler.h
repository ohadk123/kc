#ifndef COMPILER_H
#define COMPILER_H

#include "hashmap.h"
#include "kir.h"
#include "statement.h"
#include "token.h"

typedef struct _Scope Scope;
struct _Scope {
    HashMap symbols;
    Scope *above;

    bool inLoop;

    String contLabel;
    String breakLabel;
};

typedef struct {
    String fileName;
    String input;
    TokensList tokens;
    TldList ast;
    ProgramKir *kir;
    Scope globalSymbols;
} TranslationUnit;

__attribute__((__noreturn__)) void compile_error(String fileName, Location place, const char *fmt, ...);
bool compile_err_no_abort(String fileName, Location place, const char *fmt, ...);

#endif // COMPILER_H
