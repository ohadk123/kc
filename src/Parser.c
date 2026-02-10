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

TokenType peekAhead(Parser *p, usize n) {
    if (p->index + n >= p->input.len) return 0;
    return p->input.arr[p->index + n].type;
}

static Expr *parseNum(Parser *p) {
    if (match(p, TOK_INTEGER_LITERAL)) {
        Token prev = previous(p);
        Expr *numExpr = (Expr *)malloc(sizeof(Expr));
        numExpr->type = EXPR_NUM;
        numExpr->as.num = prev;
        return numExpr;
    }

    fprintf(stderr, "[%zu:%zu]: Expected expression\n", peek(p).line, peek(p).col);
    abort();
}

static Expr *parseMult(Parser *p) {
    Expr *expr = parseNum(p);

    while (match(p, TOK_STAR)) {
        Expr *rhs = parseNum(p);

        Expr *multExpr = (Expr *)malloc(sizeof(Expr));
        multExpr->type = EXPR_BINARY;
        multExpr->as.binary.op = TOK_STAR;
        multExpr->as.binary.lhs = expr;
        multExpr->as.binary.rhs = rhs;

        expr = multExpr;
    }

    return expr;
}

static Expr *parsePlus(Parser *p) {
    Expr *expr = parseMult(p);

    while (match(p, TOK_PLUS)) {
        Expr *rhs = parseMult(p);

        Expr *plusExpr = (Expr *)malloc(sizeof(Expr));
        plusExpr->type = EXPR_BINARY;
        plusExpr->as.binary.op = TOK_PLUS;
        plusExpr->as.binary.lhs = expr;
        plusExpr->as.binary.rhs = rhs;

        expr = plusExpr;
    }

    return expr;
}

Expr *parse(TokensList tokens) {
    Parser parser = {
        .input = tokens,
        .fileName = {0},
        .index = 0,
        .hasErros = FALSE,
    };

    Expr *root = parsePlus(&parser);

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
        case EXPR_NUM:
            break;
    }
    free(e);
}

void printExpr(Expr *e, int depth) {
    switch (e->type) {

        case EXPR_NUM:
            for (int i = 0; i < depth; i++) printf("\t");
            printf("%zu", e->as.num.as.integerLiteral);
            break;
        case EXPR_BINARY:
            for (int i = 0; i < depth; i++) printf("\t");

        case EXPR_UNARY:
            break;
    }
}
