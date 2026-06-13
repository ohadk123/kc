#ifndef CODEGEN_H
#define CODEGEN_H

#include "compiler.h"

void lower_ast(TranslationUnit *unit, FILE *outf);

#endif  // CODEGEN_H
