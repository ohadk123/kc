#include "Parser.h"
#include <libk/Errors.h>
#include <stdio.h>

typedef struct {
    TokensList input;
    String fileName;
    usize index;
    Bool hasErros;
} Parser;

static Bool isAtEnd(Parser *p) { return p->index == p->input.len; }

static Token previous(Parser *p) { return p->input.arr[p->index - 1]; }

static Bool match(Parser *p, TokenType next) {
    if (isAtEnd(p)) return FALSE;
    if (p->input.arr[p->index].type != next) return FALSE;

    p->index++;
    return TRUE;
}

static Token peek(Parser *p) {
    if (isAtEnd(p)) return (Token){0};
    return p->input.arr[p->index];
}

__attribute__((__noreturn__)) static void parseError(Parser *p, cstr msg) {
    fprintf(stderr, "[%zu:%zu]: %s\n", peek(p).line, peek(p).col, msg);
    abort();
}

static void expect(Parser *p, TokenType expected, cstr msg) {
    if (match(p, expected)) return;
    parseError(p, msg);
}

/******************************************************************************
 * Parsing Functions
 *****************************************************************************/

static Expr *expression(Parser *p);
static Expr *comma(Parser *p);
static Expr *conditional(Parser *p);
static Expr *logicalOr(Parser *p);
static Expr *logicalAnd(Parser *p);
static Expr *bitwiseOr(Parser *p);
static Expr *bitwiseXor(Parser *p);
static Expr *bitwiseAnd(Parser *p);
static Expr *assignment(Parser *p);
static Expr *equality(Parser *p);
static Expr *relational(Parser *p);
static Expr *shift(Parser *p);
static Expr *additive(Parser *p);
static Expr *multipicative(Parser *p);
static Expr *primary(Parser *p);

//*****************************************************************************

static Expr *expression(Parser *p) { return comma(p); }

static Expr *comma(Parser *p) {
    Expr *expr = assignment(p);

    while (match(p, TOK_COMMA)) {
        Expr *rhs = assignment(p);
        expr = makeBinaryExpr(TOK_COMMA, expr, rhs);
    }

    return expr;
}

#define DESUGAR_ASSIGNMENT(op)                                                                                         \
    else if (match(p, op##_EQUALS)) {                                                                                  \
        Expr *rhs = assignment(p);                                                                                     \
        expr = makeBinaryExpr(TOK_EQUALS, expr, makeBinaryExpr(op, expr, rhs));                                        \
    }

static Expr *assignment(Parser *p) {
    Expr *expr = conditional(p);

    if (match(p, TOK_EQUALS)) {
        Expr *rhs = assignment(p);
        expr = makeBinaryExpr(TOK_EQUALS, expr, rhs);
    }
    DESUGAR_ASSIGNMENT(TOK_PLUS)
    DESUGAR_ASSIGNMENT(TOK_MINUS)
    DESUGAR_ASSIGNMENT(TOK_STAR)
    DESUGAR_ASSIGNMENT(TOK_SLASH)
    DESUGAR_ASSIGNMENT(TOK_PERCENT)
    DESUGAR_ASSIGNMENT(TOK_AMPERSAND)
    DESUGAR_ASSIGNMENT(TOK_CARET)
    DESUGAR_ASSIGNMENT(TOK_PIPE)
    DESUGAR_ASSIGNMENT(TOK_LESS_LESS)
    DESUGAR_ASSIGNMENT(TOK_GREATER_GREATER)

    return expr;
}

static Expr *conditional(Parser *p) {
    Expr *expr = logicalOr(p);

    if (match(p, TOK_QUESTION_MARK)) {
        Expr *trueBranch = expression(p);
        expect(p, TOK_COLON, "Expected \':\'");
        Expr *falseBranch = conditional(p);
        expr = makeConditionalExpr(expr, trueBranch, falseBranch);
    }

    return expr;
}

static Expr *logicalOr(Parser *p) {
    Expr *expr = logicalAnd(p);

    return expr;
}

static Expr *logicalAnd(Parser *p) {
    Expr *expr = bitwiseOr(p);

    return expr;
}

static Expr *bitwiseOr(Parser *p) {
    Expr *expr = bitwiseXor(p);

    return expr;
}

static Expr *bitwiseXor(Parser *p) {
    Expr *expr = bitwiseAnd(p);

    return expr;
}

static Expr *bitwiseAnd(Parser *p) {
    Expr *expr = equality(p);

    return expr;
}

static Expr *equality(Parser *p) {
    Expr *expr = relational(p);

    return expr;
}

static Expr *relational(Parser *p) {
    Expr *expr = shift(p);

    return expr;
}

static Expr *shift(Parser *p) {
    Expr *expr = additive(p);

    return expr;
}

static Expr *additive(Parser *p) {
    Expr *expr = multipicative(p);

    while (match(p, TOK_PLUS)) {
        Expr *rhs = multipicative(p);
        expr = makeBinaryExpr(TOK_PLUS, expr, rhs);
    }

    while (match(p, TOK_MINUS)) {
        Expr *rhs = multipicative(p);
        expr = makeBinaryExpr(TOK_MINUS, expr, rhs);
    }

    return expr;
}

static Expr *multipicative(Parser *p) {
    Expr *expr = primary(p);

    while (match(p, TOK_STAR)) {
        Expr *rhs = primary(p);
        expr = makeBinaryExpr(TOK_STAR, expr, rhs);
    }

    while (match(p, TOK_SLASH)) {
        Expr *rhs = primary(p);
        expr = makeBinaryExpr(TOK_SLASH, expr, rhs);
    }

    return expr;
}

static Expr *primary(Parser *p) {
    if (match(p, TOK_INTEGER_LITERAL)) {
        Token prev = previous(p);
        return makePrimaryExpr(prev);
    } else if (match(p, TOK_LEFT_PAREN)) {
        Expr *inner = expression(p);
        expect(p, TOK_RIGHT_PAREN, "Expected \')\'");
        return makeGroupingExpr(inner);
    }

    parseError(p, "Expected expression");
}

//*****************************************************************************

Expr *parse(TokensList tokens) {
    Parser parser = {
        .input = tokens,
        .fileName = {0},
        .index = 0,
        .hasErros = FALSE,
    };

    Expr *root = expression(&parser);

    return root;
}

void freeExpr(Expr *e) {
    switch (e->type) {
        case EXPR_BINARY:
            freeExpr(e->as.binary.lhs);
            freeExpr(e->as.binary.rhs);
            break;
        case EXPR_UNARY:
            freeExpr(e->as.unary.inner);
            break;
        case EXPR_LITERAL:
            break;
        case EXPR_GROUPING:
            freeExpr(e->as.grouping.inner);
            break;
        case EXPR_CONDITIONAL:
            freeExpr(e->as.conditional.condition);
            freeExpr(e->as.conditional.thenBranch);
            freeExpr(e->as.conditional.elseBranch);
            break;
        default:
            fprintf(stderr, "Unknown expression type: %d\n", e->type);
            abort();
    }
    free(e);
}
