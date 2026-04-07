#ifndef CODEGEN_H_
#define CODEGEN_H_

#include "sema.h"
#include "compiler.h"

typedef struct {
    TranslationUnit unit;
    Scope currScope;
    FILE *output;
} Generator;

#endif // CODEGEN_H_
