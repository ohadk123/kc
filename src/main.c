#include "lexer.h"
#include "parser.h"
#include "sema.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: kc <file>\n");
        exit(1);
    }

    TranslationUnit unit = (TranslationUnit){
        .fileName = str_from_cstr(argv[1]),
        .input = str_from_file(argv[1]),
        .tokens = {0},
        .ast = {0},
    };

    if (!scan_file(&unit)) {
        printf("LEXER ERROR");
    }

    for (size_t i = 0; i < unit.tokens.len; i++) {
        print_tok(unit.tokens.arr[i]);
    }

    parse(&unit);
    for (size_t i = 0; i < unit.ast.len; i++) {
        print_stmt(unit.ast.arr[i], 1);
        printf("\n");
    }

    fill_global_symbol_table(&unit);

    printf("compiling done!\n");
}
