#include "lexer.h"
#include "parser.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: kc <file>\n");
        exit(1);
    }

    TokensList tokens = {0};
    if (!scan_file(&tokens, argv[1])) {
        printf("LEXER ERROR");
    }

    for (size_t i = 0; i < tokens.len; i++) {
        print_tok(tokens.arr[i]);
    }

    StmtList ast = parse(tokens, str_from_cstr(argv[1]));
    printf("ast := %zu\n", ast.len);

    printf("compiling done!\n");
}
