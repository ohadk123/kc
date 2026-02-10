#ifndef _INCLUDE_INCLUDE_EXPRESSION_H_
#define _INCLUDE_INCLUDE_EXPRESSION_H_

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

#include "Token.h"

typedef struct Expr Expr;

typedef enum {
    EXPR_LITERAL,
    EXPR_GROUPING,
    EXPR_BINARY,
    EXPR_UNARY,
} ExprType;

typedef struct {
    Token value;
} PrimaryExpr;

typedef struct {
    Expr *inner;
} GroupingExpr;

typedef struct {
    TokenType op;
    Expr *lhs;
    Expr *rhs;
} BinaryExpr;

typedef struct {
    TokenType op;
    Expr *inner;
} UnaryExpr;

struct Expr {
    ExprType type;
    union {
        PrimaryExpr primary;
        GroupingExpr grouping;
        BinaryExpr binary;
        UnaryExpr unary;
    } as;
};

Expr *makePrimaryExpr(Token value);
Expr *makeGroupingExpr(Expr *inner);
Expr *makeBinaryExpr(TokenType op, Expr *lhs, Expr *rhs);
Expr *makeUnaryExpr(TokenType op, Expr *inner);

void freeExpr(Expr *e);
int eval(Expr *root);

#endif  // _INCLUDE_INCLUDE_EXPRESSION_H_
