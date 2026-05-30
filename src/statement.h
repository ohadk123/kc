#ifndef STATEMENT_H
#define STATEMENT_H

#include "expression.h"

typedef struct _Type Type;

typedef struct _Stmt Stmt;
typedef struct {
    LIST_FIELDS(Stmt *);
} StmtList;

typedef enum {
    STMT_NULL,
    STMT_VAR,
    STMT_EXPR,
    STMT_BLOCK,
    STMT_WHILE,
    STMT_DO_WHILE,
    STMT_IF,
    STMT_FOR,
    STMT_RETURN,
    STMT_FUNC,

    // struct-less statements
    STMT_BREAK,
    STMT_CONTINUE,
} StmtKind;

typedef struct {
    Type *type;
    Token name;
    Expr *init;

    String qbeVarAddr;
} VarStmt;

typedef struct {
    Expr *expr;
} ExprStmt;

typedef struct {
    StmtList block;
} BlockStmt;

typedef struct {
    Expr *condition;
    Stmt *body;
} WhileStmt;

typedef struct {
    Expr *condition;
    Stmt *thenBranch;
    Stmt *elseBranch;
} IfStmt;

typedef struct {
    Stmt *initializer;
    Expr *condition;
    Expr *increment;
    Stmt *body;
} ForStmt;

typedef struct {
    Expr *retVal;
} ReturnStmt;

typedef struct {
    Token name;
    Type *funcType;
    StmtList block;
} FuncStmt;

struct _Stmt {
    StmtKind kind;
    Location loc;
    union {
        VarStmt var;
        ExprStmt expr;
        BlockStmt block;
        WhileStmt whileS;
        WhileStmt doWhile;
        IfStmt ifS;
        ForStmt forS;
        ReturnStmt returnS;
        FuncStmt func;
    } as;
};

Stmt *stmt_make_null(Location loc);
Stmt *stmt_make_var(Type *type, Token name, Expr *initalizer, Location loc);
Stmt *stmt_make_expr(Expr *inner, Location loc);
Stmt *stmt_make_block(StmtList block, Location loc);
Stmt *stmt_make_while(Expr *cond, Stmt *body, Location loc);
Stmt *stmt_make_do_while(Expr *cond, Stmt *body, Location loc);
Stmt *stmt_make_if(Expr *cond, Stmt *thenBranch, Stmt *elseBranch, Location loc);
Stmt *stmt_make_for(Stmt *init, Expr *cond, Expr *inc, Stmt *body, Location loc);
Stmt *stmt_make_return(Expr *ret_val, Location loc);
Stmt *stmt_make_func(Type *funcType, Token name, StmtList block, Location loc);

Stmt *stmt_make_break(Location loc);
Stmt *stmt_make_continue(Location loc);

void print_stmt(Stmt *stmt, int indent);
Token get_top_level_name(Stmt *s);

#endif // !STATEMENT_H
