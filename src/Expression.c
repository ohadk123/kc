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

Expr *makeConditionalExpr(Expr *condition, Expr *thenBranch, Expr *elseBranch) {
    Expr *e = malloc(sizeof(Expr));
    e->type = EXPR_CONDITIONAL;
    e->as.conditional.condition = condition;
    e->as.conditional.thenBranch = thenBranch;
    e->as.conditional.elseBranch = elseBranch;
    return e;
}

#define EVAL_BINARY(TOK, op)                                                                                           \
    case TOK:                                                                                                          \
        lhs = eval(root->as.binary.lhs);                                                                               \
        rhs = eval(root->as.binary.rhs);                                                                               \
        return lhs op rhs

#define EVAL_UNARY(TOK, op)                                                                                            \
    case TOK:                                                                                                          \
        return op eval(root->as.unary.inner)

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
                EVAL_BINARY(TOK_PLUS, +);
                EVAL_BINARY(TOK_MINUS, -);
                EVAL_BINARY(TOK_STAR, *);
                EVAL_BINARY(TOK_SLASH, /);
                EVAL_BINARY(TOK_PERCENT, %);
                EVAL_BINARY(TOK_AMPERSAND, &);
                EVAL_BINARY(TOK_CARET, ^);
                EVAL_BINARY(TOK_PIPE, |);
                EVAL_BINARY(TOK_LESS_LESS, <<);
                EVAL_BINARY(TOK_GREATER_GREATER, >>);
                EVAL_BINARY(TOK_PIPE_PIPE, ||);
                EVAL_BINARY(TOK_AMPERSAND_AMPERSAND, &&);
                EVAL_BINARY(TOK_EQUALS_EQUALS, ==);
                EVAL_BINARY(TOK_BANG_EQUALS, !=);
                EVAL_BINARY(TOK_LESS, <);
                EVAL_BINARY(TOK_LESS_EQUALS, <=);
                EVAL_BINARY(TOK_GREATER, >);
                EVAL_BINARY(TOK_GREATER_EQUALS, >=);
                case TOK_EQUALS:
                    return eval(root->as.binary.rhs);
                default:
                    TODO("Binary Operators");
            }
        case EXPR_GROUPING:
            return eval(root->as.grouping.inner);
        case EXPR_UNARY:
            switch (root->as.unary.op) {
                EVAL_UNARY(TOK_PLUS, +);
                EVAL_UNARY(TOK_MINUS, -);
                EVAL_UNARY(TOK_TILDE, ~);
                EVAL_UNARY(TOK_BANG, !);
                case TOK_PLUS_PLUS:
                    return eval(root->as.unary.inner) + 1;
                case TOK_MINUS_MINUS:
                    return eval(root->as.unary.inner) - 1;
                case TOK_AMPERSAND:
                    UNIMPLEMENTED("Unary Ampersand");
                case TOK_STAR:
                    UNIMPLEMENTED("Unary Star");
                default:
                    fprintf(stderr, "Not a valid unary operator: %d(%c)", root->as.unary.op, root->as.unary.op);
                    abort();
            }
        case EXPR_CONDITIONAL:
            return eval(root->as.conditional.condition) ? eval(root->as.conditional.thenBranch)
                                                        : eval(root->as.conditional.elseBranch);
    }
    printf("Unknown expression type: %d\n", root->type);
    UNIMPLEMENTED("Don't come here");
}
