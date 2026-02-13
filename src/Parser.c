#include "Parser.h"
#include <libk/Errors.h>
#include <stdarg.h>
#include <stdio.h>

typedef struct {
    TokensList input;
    String fileName;
    usize index;
    Bool hasErrors;
} Parser;

static Bool isAtEnd(Parser *p) { return p->index == p->input.len; }

static Token previous(Parser *p) { return p->input.arr[p->index - 1]; }

static Bool match(Parser *p, usize count, ...) {
    if (isAtEnd(p)) return FALSE;

    va_list args;
    va_start(args, count);
    for (usize i = 0; i < count; i++) {
        if (p->input.arr[p->index].type == va_arg(args, TokenType)) {
            va_end(args);
            p->index++;
            return TRUE;
        }
    }
    va_end(args);
    return FALSE;
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
    if (match(p, 1, expected)) return;
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
static Expr *multiplicative(Parser *p);
static Expr *unary(Parser *p);
static Expr *postfix(Parser *p);
static Expr *primary(Parser *p);

//*****************************************************************************

static Expr *expression(Parser *p) { return comma(p); }

static Expr *comma(Parser *p) {
    Expr *expr = assignment(p);

    while (match(p, 1, TOK_COMMA)) {
        Expr *rhs = assignment(p);
        expr = makeBinaryExpr(TOK_COMMA, expr, rhs);
    }

    return expr;
}

#define DESUGAR_ASSIGNMENT(op)                                                                                         \
    else if (match(p, 1, op##_EQUALS)) {                                                                               \
        Expr *rhs = assignment(p);                                                                                     \
        expr = makeBinaryExpr(TOK_EQUALS, expr, makeBinaryExpr(op, cloneExpr(expr), rhs));                             \
    }

static Expr *assignment(Parser *p) {
    Expr *expr = conditional(p);

    if (match(p, 1, TOK_EQUALS)) {
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

    if (match(p, 1, TOK_QUESTION_MARK)) {
        Expr *trueBranch = expression(p);
        expect(p, TOK_COLON, "Expected \':\'");
        Expr *falseBranch = conditional(p);
        expr = makeConditionalExpr(expr, trueBranch, falseBranch);
    }

    return expr;
}

static Expr *logicalOr(Parser *p) {
    Expr *expr = logicalAnd(p);

    while (match(p, 1, TOK_PIPE_PIPE)) {
        Expr *rhs = logicalAnd(p);
        expr = makeBinaryExpr(TOK_PIPE_PIPE, expr, rhs);
    }

    return expr;
}

static Expr *logicalAnd(Parser *p) {
    Expr *expr = bitwiseOr(p);

    while (match(p, 1, TOK_AMPERSAND_AMPERSAND)) {
        Expr *rhs = bitwiseOr(p);
        expr = makeBinaryExpr(TOK_AMPERSAND_AMPERSAND, expr, rhs);
    }

    return expr;
}

static Expr *bitwiseOr(Parser *p) {
    Expr *expr = bitwiseXor(p);

    while (match(p, 1, TOK_PIPE)) {
        Expr *rhs = bitwiseXor(p);
        expr = makeBinaryExpr(TOK_PIPE, expr, rhs);
    }

    return expr;
}

static Expr *bitwiseXor(Parser *p) {
    Expr *expr = bitwiseAnd(p);

    while (match(p, 1, TOK_CARET)) {
        Expr *rhs = bitwiseAnd(p);
        expr = makeBinaryExpr(TOK_CARET, expr, rhs);
    }

    return expr;
}

static Expr *bitwiseAnd(Parser *p) {
    Expr *expr = equality(p);

    while (match(p, 1, TOK_AMPERSAND)) {
        Expr *rhs = equality(p);
        expr = makeBinaryExpr(TOK_AMPERSAND, expr, rhs);
    }

    return expr;
}

static Expr *equality(Parser *p) {
    Expr *expr = relational(p);

    while (match(p, 2, TOK_EQUALS_EQUALS, TOK_BANG_EQUALS)) {
        TokenType op = previous(p).type;
        Expr *rhs = relational(p);
        expr = makeBinaryExpr(op, expr, rhs);
    }

    return expr;
}

static Expr *relational(Parser *p) {
    Expr *expr = shift(p);

    while (match(p, 4, TOK_LESS, TOK_GREATER, TOK_LESS_EQUALS, TOK_GREATER_EQUALS)) {
        TokenType op = previous(p).type;
        Expr *rhs = shift(p);
        expr = makeBinaryExpr(op, expr, rhs);
    }

    return expr;
}

static Expr *shift(Parser *p) {
    Expr *expr = additive(p);

    while (match(p, 2, TOK_LESS_LESS, TOK_GREATER_GREATER)) {
        TokenType op = previous(p).type;
        Expr *rhs = additive(p);
        expr = makeBinaryExpr(op, expr, rhs);
    }

    return expr;
}

static Expr *additive(Parser *p) {
    Expr *expr = multiplicative(p);

    while (match(p, 2, TOK_PLUS, TOK_MINUS)) {
        TokenType op = previous(p).type;
        Expr *rhs = multiplicative(p);
        expr = makeBinaryExpr(op, expr, rhs);
    }

    return expr;
}

static Expr *multiplicative(Parser *p) {
    Expr *expr = unary(p);

    while (match(p, 3, TOK_STAR, TOK_SLASH, TOK_PERCENT)) {
        TokenType op = previous(p).type;
        Expr *rhs = unary(p);
        expr = makeBinaryExpr(op, expr, rhs);
    }

    return expr;
}

// expr = makeBinaryExpr(TOK_EQUALS, expr, makeBinaryExpr(op, expr, rhs));
static Expr *unary(Parser *p) {
    if (match(p, 2, TOK_PLUS_PLUS, TOK_MINUS_MINUS)) {
        TokenType op = previous(p).type == TOK_PLUS_PLUS ? TOK_PLUS : TOK_MINUS;
        Expr *inner = unary(p);
        Token oneToken = (Token){
            .type = TOK_INTEGER_LITERAL,
            .as.integerLiteral = 1,
            .col = previous(p).col,
            .line = previous(p).line,
        };
        Expr *oneExpr = makePrimaryExpr(oneToken);
        return makeBinaryExpr(TOK_EQUALS, inner, makeBinaryExpr(op, inner, oneExpr));
    }

    if (match(p, 6, TOK_AMPERSAND, TOK_STAR, TOK_PLUS, TOK_MINUS, TOK_TILDE, TOK_BANG)) {
        TokenType op = previous(p).type;
        Expr *inner = unary(p);
        return makeUnaryExpr(op, inner);
    }

    return postfix(p);
}

static Expr *postfix(Parser *p) {
    Expr *expr = primary(p);

    while (TRUE) {
        if (match(p, 1, TOK_LEFT_BRACKET)) {
            Expr *index = expression(p);
            expect(p, TOK_RIGHT_BRACKET, "Missing ']' at the end of indexing");
            expr = makeIndexExpr(expr, index);
        } else if (match(p, 1, TOK_LEFT_PAREN)) {
            ArgsList args = {0};
            if (!match(p, 1, TOK_RIGHT_PAREN)) {
                do {
                    Expr *arg = assignment(p);
                    appendSingle(&args, arg);
                } while (match(p, 1, TOK_COMMA));
                expect(p, TOK_RIGHT_PAREN, "Missing ')' at the end of function call");
            }
            expr = makeFuncCallExpr(expr, args);
        } else if (match(p, 2, TOK_DOT, TOK_MINUS_GREATER)) {
            TokenType op = previous(p).type;
            expect(p, TOK_IDENTIFIER, "Expected member");
            expr = makeMemberExpr(op, expr, previous(p));
        } else if (match(p, 2, TOK_PLUS_PLUS, TOK_MINUS_MINUS)) {
            TokenType op = previous(p).type;
            expr = makeUnaryExpr(op, expr);
        } else {
            break;
        }
    }

    return expr;
}

static Expr *primary(Parser *p) {
    if (match(p, 5, TOK_INTEGER_LITERAL, TOK_FLOAT_LITERAL, TOK_CHAR_LITERAL, TOK_STRING_LITERAL, TOK_IDENTIFIER)) {
        Token prev = previous(p);
        return makePrimaryExpr(prev);
    } else if (match(p, 1, TOK_LEFT_PAREN)) {
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
        .hasErrors = FALSE,
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
        case EXPR_INDEX:
            freeExpr(e->as.index.index);
            freeExpr(e->as.index.name);
            break;
        case EXPR_FUNC_CALL:
            freeExpr(e->as.funcCall.callee);
            for (usize i = 0; i < e->as.funcCall.args.len; i++) {
                freeExpr(e->as.funcCall.args.arr[i]);
            }
            break;
        case EXPR_MEMBER:
            freeExpr(e->as.member.object);
            break;
            // default:
            //     fprintf(stderr, "Unknown expression type: %d\n", e->type);
            //     abort();
    }
    free(e);
}
