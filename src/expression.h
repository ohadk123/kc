#ifndef EXPRESSION_H
#define EXPRESSION_H

/**
 * Expressions - oredered by precedence: (first is highest precedence)
 * 1. Primary: literals, indetifiers, parenthesized expressions
 * 2. Postfix: func-calls [] . -> ++ --
 * 3. Unary: & * + - ~ ~ ! ++ --
 * 4. multiplicative: * / %
 * 5. additive: - +
 * 6. shift: << >>
 * 7. relational: < > <= >=
 * 8. equality: == !=
 * 9. bitwise AND: &
 * 10. bitwise XOR: ^
 * 11. bitwise OR: |
 * 12. logical AND: &&
 * 13. logical OR: ||
 * 14. conditional: ?:
 * 15. assignment: = += -= *= /= %= &= ^= |= <<= >>=
 * 16. comma: ,
 * 17. expression;
 */

#include "token.h"

typedef struct _Type Type;

typedef struct _Expr Expr;
typedef struct {
    LIST_FIELDS(Expr *);
} ExprList;

typedef enum {
    EXPR_PRIMARY,
    EXPR_GROUPING,
    EXPR_BINARY,
    EXPR_UNARY,
    EXPR_ASSIGN,
    EXPR_UNARY_POST,
    EXPR_CONDITIONAL,
    EXPR_FUNC_CALL,
} ExprKind;

typedef struct {
    Token value;
} PrimaryExpr;

typedef struct {
    Expr *inner;
} GroupingExpr;

typedef struct {
    TokenKind op;
    Expr *lhs;
    Expr *rhs;
} BinaryExpr;

typedef struct {
    TokenKind op;
    Expr *inner;
} UnaryExpr;

typedef struct {
    TokenKind op;
    Expr *lhs;
    Expr *rhs;
} AssignExpr;

typedef struct {
    Expr *condition;
    Expr *thenBranch;
    Expr *elseBranch;
} ConditionalExpr;

typedef struct {
    Expr *func;
    ExprList args;
} FuncCallExpr;

struct _Expr {
    ExprKind kind;
    Location loc;
    Type *type;
    union {
        PrimaryExpr primary;
        GroupingExpr grouping;
        BinaryExpr binary;
        UnaryExpr unary;
        AssignExpr assignment;
        ConditionalExpr conditional;
        FuncCallExpr funcCall;
    } as;
};

Expr *expr_make_primary(Token val, Location loc);
Expr *expr_make_grouping(Expr *inner, Location loc);
Expr *expr_make_binary(TokenKind op, Expr *lhs, Expr *rhs, Location loc);
Expr *expr_make_unary(TokenKind op, Expr *inner, Location loc);
Expr *expr_make_unary_post(TokenKind op, Expr *inner, Location loc);
Expr *expr_make_assign(TokenKind op, Expr *lhs, Expr *rhs, Location loc);
Expr *expr_make_conditional(Expr *cond, Expr *thenBranch, Expr *elseBranch, Location loc);
Expr *expr_make_func_call(Expr *func, ExprList args, Location loc);

void print_expr(Expr *expr, int indent);

#endif // EXPRESSION_H
