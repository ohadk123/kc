#include "compiler.h"
#include <stdarg.h>

__attribute__((__noreturn__)) void compile_error(String fileName, Location place, const char *fmt, ...) {
    fprintf(stderr, "[%.*s:%zu:%zu] ", (int) fileName.len, fileName.data, place.line, place.col);
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
    abort();
}
