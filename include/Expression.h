#ifndef _INCLUDE_INCLUDE_EXPRESSION_H_
#define _INCLUDE_INCLUDE_EXPRESSION_H_

#include "Token.h"

typedef struct Expr Expr;

typedef struct {
    Expr *expr;
} Arg;

typedef struct {
    LIST_FIELDS(Arg);
} ArgsList;

// ***************************************************

typedef enum {
    EXPR_NUM,
    EXPR_BINARY,
    EXPR_UNARY,
} ExprType;

typedef struct {
    TokenType op;
    Expr *lhs;
    Expr *rhs;
} BinaryExpr;

typedef struct {
    TokenType op;
    Expr *inner;
} UnaryExpr;

typedef struct {
    Token ident;
    ArgsList args;
} FuncCallExpr;

struct Expr {
    ExprType type;
    union {
        Token *var;
        Token num;
        BinaryExpr binary;
        UnaryExpr unary;
        FuncCallExpr funcCall;
    } as;
};

void freeExpr(Expr *e);
int eval(Expr *root);

#endif  // _INCLUDE_INCLUDE_EXPRESSION_H_
