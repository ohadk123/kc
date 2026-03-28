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

typedef struct _Expr Expr;

typedef enum {
    EXPR_PRIMARY,
    EXPR_GROUPING,
    EXPR_BINARY,
    EXPR_UNARY,
    EXPR_UNARY_POST,
    EXPR_CONDITIONAL,
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
    Expr *condition;
    Expr *thenBranch;
    Expr *elseBranch;
} ConditionalExpr;

struct _Expr {
    ExprKind kind;
    union {
        PrimaryExpr primary;
        GroupingExpr grouping;
        BinaryExpr binary;
        UnaryExpr unary;
        ConditionalExpr conditional;
    } as;
};

Expr *expr_make_primary(Token val);
Expr *expr_make_grouping(Expr *inner);
Expr *expr_make_binary(TokenKind op, Expr *lhs, Expr *rhs);
Expr *expr_make_unary(TokenKind op, Expr *inner);
Expr *expr_make_unary_post(TokenKind op, Expr *inner);
Expr *expr_make_conditional(Expr *cond, Expr *thenBranch, Expr *elseBranch);

void print_expr(Expr *expr, int indent);

#endif // EXPRESSION_H
