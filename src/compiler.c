#include "compiler.h"
#include <stdarg.h>

__attribute__((__noreturn__)) void compile_error(String fileName, Location place, const char *fmt, ...) {
    fprintf(stderr, "[%.*s:%zu:%zu] Error: ", (int) fileName.len, fileName.data, place.line, place.col);
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
    abort();
}

bool compile_err_no_abort(String fileName, Location place, const char *fmt, ...) {
    fprintf(stderr, "[%.*s:%zu:%zu] \033[31;1;4mError:\033[0m ", (int) fileName.len, fileName.data, place.line, place.col);
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
    return true;
}
