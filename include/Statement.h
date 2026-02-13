#ifndef INCLUDE_KC_STATEMENT_H_
#define INCLUDE_KC_STATEMENT_H_

#include "Expression.h"

typedef enum {
    STMT_DECLARATION,
} StmtType;

typedef struct {
    TokenType type;
    Token identifier;
    Expr *initializer;
} DeclStmt;

typedef struct {
    StmtType type;
    union {
        DeclStmt declaration;
    } as;
} Stmt;

typedef struct {
    LIST_FIELDS(Stmt *);
} StmtList;

Stmt *makeDeclStmt(TokenType type, Token identifier, Expr *initializer);

Stmt *cloneStmt(Stmt *src);
void printStmt(Stmt *root);
void freeStmt(Stmt *e);

#endif // INCLUDE_KC_STATEMENT_H_
