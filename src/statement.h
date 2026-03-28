#ifndef STATEMENT_H
#define STATEMENT_H

#include "expression.h"

typedef struct _Stmt Stmt;
typedef struct {
    LIST_FIELDS(Stmt);
} StmtList;

typedef enum {
    STMT_VAR,
    STMT_EXPR,
    STMT_BLOCK,
    STMT_WHILE,
    STMT_IF,
    STMT_FOR,

    // struct-less statements
    STMT_BREAK,
    STMT_CONTINUE,
} StmtKind;

typedef struct {
    TokenKind type;
    Token name;
    Expr *init;
} VarStmt;

typedef struct {
    Expr *expr;
} ExprStmt;

typedef struct {
    StmtList block;
} BlockStmt;

typedef struct {
    Expr *cond;
    Stmt *body;
} WhileStmt;

typedef struct {
    Expr *cond;
    Stmt *thenBranch;
    Stmt *elseBranch;
} IfStmt;

typedef struct {
    Stmt *initializer;
    Expr *condition;
    Expr *increment;
    Stmt *body;
} ForStmt;

struct _Stmt {
    StmtKind kind;
    union {
        VarStmt var;
        ExprStmt expr;
        BlockStmt block;
        WhileStmt whileS;
        IfStmt ifS;
        ForStmt forS;
    } as;
};

Stmt *stmt_make_var(TokenKind type, Token name, Expr *initalizer);
Stmt *stmt_make_expr(Expr *inner);
Stmt *stmt_make_block(StmtList block);
Stmt *stmt_make_while(Expr *cond, Stmt *body);
Stmt *stmt_make_if(Expr *cond, Stmt *thenBranch, Stmt *elseBranch);
Stmt *stmt_make_for(Stmt *init, Expr *cond, Expr *inc, Stmt *body);

#endif // !STATEMENT_H
