#include "expression.h"

static Expr *make_expr(ExprKind kind) {
    Expr *e = malloc(sizeof(Expr));
    e->kind = kind;
    return e;
}

Expr *expr_make_primary(Token val) {
    Expr *e = make_expr(EXPR_PRIMARY);
    e->as.primary.value = val;
    return e;
}

Expr *expr_make_grouping(Expr *inner) {
    Expr *e = make_expr(EXPR_GROUPING);
    e->as.grouping.inner = inner;
    return e;
}

Expr *expr_make_binary(TokenKind op, Expr *lhs, Expr *rhs) {
    Expr *e = make_expr(EXPR_BINARY);
    e->as.binary.op = op;
    e->as.binary.lhs = lhs;
    e->as.binary.rhs = rhs;
    return e;
}

Expr *expr_make_unary(TokenKind op, Expr *inner) {
    Expr *e = make_expr(EXPR_UNARY);
    e->as.unary.op = op;
    e->as.unary.inner = inner;
    return e;
}

Expr *expr_make_unary_post(TokenKind op, Expr *inner) {
    Expr *e = make_expr(EXPR_UNARY_POST);
    e->as.unary.op = op;
    e->as.unary.inner = inner;
    return e;
}

Expr *expr_make_conditional(Expr *cond, Expr *thenBranch, Expr *elseBranch) {
    Expr *e = make_expr(EXPR_CONDITIONAL);
    e->as.conditional.condition = cond;
    e->as.conditional.thenBranch = thenBranch;
    e->as.conditional.elseBranch = elseBranch;
    return e;
}
