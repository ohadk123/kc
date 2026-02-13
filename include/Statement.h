#ifndef INCLUDE_KC_STATEMENT_H_
#define INCLUDE_KC_STATEMENT_H_

#include "Type.h"

typedef enum {
    STORAGE_NONE,
    STORAGE_EXTERN,
    STORAGE_STATIC,
} StorageClass;

typedef enum {
    STMT_DECLARATION,
    STMT_ENUM,
} StmtType;

typedef struct {
    Type *type;
    StorageClass storageClass;
    Token identifier;
    Expr *initializer;
} VarStmt;

typedef struct {
    Token name;
    TokensList entries;
} EnumStmt;

typedef struct {
    StmtType type;
    union {
        VarStmt declaration;
        EnumStmt enumStmt;
    } as;
} Stmt;

typedef struct {
    LIST_FIELDS(Stmt *);
} StmtList;

Stmt *makeVarStmt(Type *type, StorageClass storageClass, Token identifier, Expr *initializer);
Stmt *makeEnumStmt(Token name, TokensList entries);

Stmt *cloneStmt(Stmt *src);
void printStmtList(StmtList list);
void freeStmt(Stmt *e);

#endif // INCLUDE_KC_STATEMENT_H_
