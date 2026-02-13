#ifndef INCLUDE_KC_STATEMENT_H_
#define INCLUDE_KC_STATEMENT_H_

#include "Type.h"

typedef enum {
    STMT_DECLARATION,
} StmtType;

typedef enum {
    STORAGE_NONE,
    STORAGE_EXTERN,
    STORAGE_STATIC,
} StorageClass;

typedef struct {
    Type *type;
    StorageClass storageClass;
    Token identifier;
    Expr *initializer;
} VarDeclStmt;

typedef struct {
    StmtType type;
    union {
        VarDeclStmt declaration;
    } as;
} Stmt;

typedef struct {
    LIST_FIELDS(Stmt *);
} StmtList;

Stmt *makeDeclStmt(Type *type, StorageClass storageClass, Token identifier, Expr *initializer);

Stmt *cloneStmt(Stmt *src);
void printStmt(Stmt *root);
void freeStmt(Stmt *e);

#endif // INCLUDE_KC_STATEMENT_H_
