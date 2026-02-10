#include "Parser.h"
#include <stdio.h>

typedef struct {
    TokensList input;
    String fileName;
    usize index;
    Bool hasErros;
} Parser;

static Bool isAtEnd(Parser *p) { return p->index == p->input.len; }

static Token previous(Parser *p) {
    return p->input.arr[p->index - 1];
}

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

__attribute__ ((__noreturn__))
static void parseError(Parser *p, cstr msg) {
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

static Expr *parseExpression(Parser *p);
static Expr *parseAdd(Parser *p);
static Expr *parseMult(Parser *p);
static Expr *parsePrimary(Parser *p);

static Expr *parseExpression(Parser *p) {
    return parseAdd(p);
}

static Expr *parseAdd(Parser *p) {
    Expr *expr = parseMult(p);

    while (match(p, TOK_PLUS)) {
        Expr *rhs = parseMult(p);
        expr = makeBinaryExpr(TOK_PLUS, expr, rhs);
    }

    while (match(p, TOK_MINUS)) {
        Expr *rhs = parseMult(p);
        expr = makeBinaryExpr(TOK_MINUS, expr, rhs);
    }

    return expr;
}

static Expr *parseMult(Parser *p) {
    Expr *expr = parsePrimary(p);

    while (match(p, TOK_STAR)) {
        Expr *rhs = parsePrimary(p);
        expr = makeBinaryExpr(TOK_STAR, expr, rhs);
    }

    while (match(p, TOK_SLASH)) {
        Expr *rhs = parsePrimary(p);
        expr = makeBinaryExpr(TOK_SLASH, expr, rhs);
    }

    return expr;
}

static Expr *parsePrimary(Parser *p) {
    if (match(p, TOK_INTEGER_LITERAL)) {
        Token prev = previous(p);
        return makePrimaryExpr(prev);
    } else if (match(p, TOK_LEFT_PAREN)) {
        Expr *inner = parseExpression(p);
        expect(p, TOK_RIGHT_PAREN, "Expected \')\'");
        return makeGroupingExpr(inner);
    }

    parseError(p, "Expected expression");
}

Expr *parse(TokensList tokens) {
    Parser parser = {
        .input = tokens,
        .fileName = {0},
        .index = 0,
        .hasErros = FALSE,
    };

    Expr *root = parseExpression(&parser);

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
        }
    free(e);
}
