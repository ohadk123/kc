#include "Lexer.h"
#include "Parser.h"
#include <stdio.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file>\n", argv[0]);
        return 1;
    }

    TokensList tokens = {0};
    if (!scanFile(&tokens, argv[1])) {
        fprintf(stderr, "Failed to scan file: %s\n", argv[1]);
        return 1;
    }

    StmtList translation_unit = parse(tokens);
    for (usize i = 0; i < translation_unit.len; i++) {
        printStmt(translation_unit.arr[i]);
    }

    freeTokensList(&tokens);

    return 0;
}
