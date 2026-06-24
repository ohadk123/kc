#ifndef EXPRESSION_H
#define EXPRESSION_H

/**
 * Expressions - oredered by precedence: (first is highest precedence)
 * 1. Primary: literals, indetifiers, parenthesized expressions
 * 2. Cast: cast<>()
 * 3. Postfix: func-calls [] . -> ++ --
 * 4. Unary: & * + - ~ ~ ! ++ --
 * 5. multiplicative: * / %
 * 6. additive: - +
 * 7. shift: << >>
 * 8. relational: < > <= >=
 * 9. equality: == !=
 * 10. bitwise AND: &
 * 11. bitwise XOR: ^
 * 12. bitwise OR: |
 * 13. logical AND: &&
 * 14. logical OR: ||
 * 15. conditional: ?:
 * 16. assignment: = += -= *= /= %= &= ^= |= <<= >>=
 * 17. comma: ,
 * 18. expression;
 */

#include "token.h"

typedef struct _Stmt Stmt;

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
    EXPR_INDEX,
    EXPR_CAST,
} ExprKind;

typedef enum {
    PRIM_IDENTIFIER      = TOK_IDENTIFIER,
    PRIM_STRING_LITERAL  = TOK_STRING_LITERAL,
    PRIM_CHAR_LITERAL    = TOK_CHAR_LITERAL,
    PRIM_INTEGER_LITERAL = TOK_INTEGER_LITERAL,
    PRIM_LONG_LITERAL    = TOK_LONG_LITERAL,
    PRIM_FLOAT_LITERAL   = TOK_FLOAT_LITERAL,
    PRIM_DOUBLE_LITERAL  = TOK_DOUBLE_LITERAL,
    PRIM_TRUE            = TOK_TRUE,
    PRIM_FALSE           = TOK_FALSE,
} PrimaryKind;

typedef struct {
    PrimaryKind kind;
    union {
        String identifier;
        String stringLiteral;
        uint8_t charLiteral;
        uint32_t integerLiteral;
        uint64_t longLiteral;
        float floatLiteral;
        double doubleLiteral;
        char unknown;
    } as;
} PrimaryValue;

typedef struct {
    PrimaryValue value;
    Stmt *decl;
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

typedef struct {
    Expr *array;
    Expr *index;
} IndexExpr;

typedef struct {
    TokenKind type;
    Expr *inner;
} CastExpr;

struct _Expr {
    ExprKind kind;
    Location loc;
    union {
        PrimaryExpr primary;
        GroupingExpr grouping;
        BinaryExpr binary;
        UnaryExpr unary;
        AssignExpr assignment;
        ConditionalExpr conditional;
        FuncCallExpr funcCall;
        IndexExpr index;
        CastExpr cast;
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
Expr *expr_make_index(Expr *array, Expr *index, Location loc);
Expr *expr_make_cast(TokenKind type, Expr *inner, Location loc);

void print_expr(Expr *expr, int indent);

#endif // EXPRESSION_H
