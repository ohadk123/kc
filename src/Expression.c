#include "Expression.h"

#include <libk/Errors.h>

Expr *makePrimaryExpr(Token value) {
    Expr *e = malloc(sizeof(Expr));
    e->type = EXPR_LITERAL;
    e->as.primary.value = value;
    return e;
}

Expr *makeGroupingExpr(Expr *inner) {
    Expr *e = malloc(sizeof(Expr));
    e->type = EXPR_GROUPING;
    e->as.grouping.inner = inner;
    return e;
}

Expr *makeBinaryExpr(TokenType op, Expr *lhs, Expr *rhs) {
    Expr *e = malloc(sizeof(Expr));
    e->type = EXPR_BINARY;
    e->as.binary.op = op;
    e->as.binary.lhs = lhs;
    e->as.binary.rhs = rhs;
    return e;
}

Expr *makeUnaryExpr(TokenType op, Expr *inner) {
    Expr *e = malloc(sizeof(Expr));
    e->type = EXPR_UNARY;
    e->as.unary.op = op;
    e->as.unary.inner = inner;
    return e;
}

int eval(Expr *root) {
    int lhs, rhs;

    switch (root->type) {
        case EXPR_LITERAL:
            switch (root->as.primary.value.type) {
                case TOK_INTEGER_LITERAL:
                    return root->as.primary.value.as.integerLiteral;
                default:
                    TODO("Primary Expressions");
            }
        case EXPR_BINARY:
            switch (root->as.binary.op) {
                case TOK_PLUS:
                    lhs = eval(root->as.binary.lhs);
                    rhs = eval(root->as.binary.rhs);
                    return lhs + rhs;
                case TOK_MINUS:
                    lhs = eval(root->as.binary.lhs);
                    rhs = eval(root->as.binary.rhs);
                    return lhs - rhs;
                case TOK_STAR:
                    lhs = eval(root->as.binary.lhs);
                    rhs = eval(root->as.binary.rhs);
                    return lhs * rhs;
                case TOK_SLASH:
                    lhs = eval(root->as.binary.lhs);
                    rhs = eval(root->as.binary.rhs);
                    return lhs / rhs;
                default:
                    TODO("Binary Operators");
            }
        case EXPR_GROUPING:
            return eval(root->as.grouping.inner);
        case EXPR_UNARY:
            TODO("Unary Expressions");
    }
    UNIMPLEMENTED("Don't come here");
}
