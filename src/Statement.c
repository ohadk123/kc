#include "Statement.h"
#include <stdio.h>

Stmt *makeDeclStmt(TokenType type, Token identifier, Expr *initializer) {
    Stmt *s = malloc(sizeof(Stmt));
    s->type = STMT_DECLARATION;
    s->as.declaration.type = type;
    s->as.declaration.identifier = identifier;
    s->as.declaration.initializer = initializer;
    return s;
}

static void printIndent(int indent) {
    for (int i = 0; i < indent; i++) printf("  ");
}

static void printStmtImpl(Stmt *root, usize indent) {
    printf("{\n");
    switch (root->type) {
        case STMT_DECLARATION:
            printIndent(indent + 1);
            printf("\"type\": \"%s\"\n", tokenTypesStrings[root->as.declaration.type]);
            printIndent(indent + 1);
            printf("\"identifier\": ");
            printToken(root->as.declaration.identifier);
            printf("\n");
            printIndent(indent + 1);
            if (root->as.declaration.initializer != NULL) {
                printf("\"initializer\": ");
                printExprImpl(root->as.declaration.initializer, indent + 1);
            }
            break;
    }
    printf("}\n");
}

void printStmt(Stmt *root) {
    if (root == NULL) {
        printf("null\n");
        return;
    }

    printStmtImpl(root, 0);
}
