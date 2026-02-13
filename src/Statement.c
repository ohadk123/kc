#include "Statement.h"
#include <stdio.h>

Stmt *makeVarStmt(Type *type, StorageClass storageClass, Token identifier, Expr *initializer) {
    Stmt *s = malloc(sizeof(Stmt));
    s->type = STMT_DECLARATION;
    s->as.declaration.type = type;
    s->as.declaration.storageClass = storageClass;
    s->as.declaration.identifier = identifier;
    s->as.declaration.initializer = initializer;
    return s;
}

Stmt *makeEnumStmt(Token name, TokensList entries) {
    Stmt *s = malloc(sizeof(Stmt));
    s->type = STMT_ENUM;
    s->as.enumStmt.name = name;
    s->as.enumStmt.entries = entries;
    return s;
}

static void printIndent(int indent) {
    for (int i = 0; i < indent; i++) printf("  ");
}

static cstr storageClassStrings[] = {
    [STORAGE_NONE] = "none",
    [STORAGE_EXTERN] = "extern",
    [STORAGE_STATIC] = "static",
};

static void printTypeImpl(Type *type, usize indent) {
    printf("{\n");
    switch (type->kind) {
        case TYPE_SIMPLE:
            printIndent(indent + 1);
            printf("\"kind\": \"simple\",\n");
            printIndent(indent + 1);
            printf("\"const\": %s,\n", type->isConst ? "true" : "false");
            printIndent(indent + 1);
            printf("\"token\": ");
            printToken(type->as.simple);
            printf("\n");
            break;
        case TYPE_POINTER:
            printIndent(indent + 1);
            printf("\"kind\": \"pointer\",\n");
            printIndent(indent + 1);
            printf("\"const\": %s,\n", type->isConst ? "true" : "false");
            printIndent(indent + 1);
            printf("\"inner\": ");
            printTypeImpl(type->as.pointer, indent + 1);
            break;
        case TYPE_ARRAY:
            printIndent(indent + 1);
            printf("\"kind\": \"array\",\n");
            printIndent(indent + 1);
            printf("\"const\": %s,\n", type->isConst ? "true" : "false");
            printIndent(indent + 1);
            printf("\"inner\": ");
            printTypeImpl(type->as.array.inner, indent + 1);
            printIndent(indent + 1);
            if (type->as.array.size != NULL) {
                printf("\"size\": ");
                printExprImpl(type->as.array.size, indent + 1);
            } else {
                printf("\"size\": null\n");
            }
            break;
    }
    printIndent(indent);
    printf("}\n");
}

static void printStmtImpl(Stmt *root, usize indent) {
    printf("{\n");
    switch (root->type) {
        case STMT_DECLARATION:
            printIndent(indent + 1);
            printf("\"stmt\": \"declaration\",\n");
            printIndent(indent + 1);
            printf("\"storage\": \"%s\",\n", storageClassStrings[root->as.declaration.storageClass]);
            printIndent(indent + 1);
            printf("\"type\": ");
            printTypeImpl(root->as.declaration.type, indent + 1);
            printIndent(indent + 1);
            printf("\"identifier\": ");
            printToken(root->as.declaration.identifier);
            printf(",\n");
            printIndent(indent + 1);
            if (root->as.declaration.initializer != NULL) {
                printf("\"initializer\": ");
                printExprImpl(root->as.declaration.initializer, indent + 1);
            } else {
                printf("\"initializer\": null\n");
            }
            break;
        case STMT_ENUM:
            printIndent(indent + 1);
            printf("\"stmt\": \"enum\",\n");
            printIndent(indent + 1);
            printf("\"name\": ");
            printToken(root->as.enumStmt.name);
            printf(",\n");
            printIndent(indent + 1);
            printf("\"entries\": [\n");
            for (usize i = 0; i < root->as.enumStmt.entries.len; i++) {
                printIndent(indent + 2);
                printToken(root->as.enumStmt.entries.arr[i]);
                if (i + 1 < root->as.enumStmt.entries.len) printf(",");
                printf("\n");
            }
            printIndent(indent + 1);
            printf("]\n");
            break;
    }
    printIndent(indent);
    printf("}");
}

void printStmtList(StmtList list) {
    printf("[\n");
    for (usize i = 0; i < list.len-1; i++) {
        printStmtImpl(list.arr[i], 1);
        printf(",\n");
    }
    printStmtImpl(list.arr[list.len-1], 1);
    printf("\n]\n");
}
