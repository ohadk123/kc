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

    Expr *root = parse(tokens);
    printExpr(root);

    freeTokensList(&tokens);

    return 0;
}
