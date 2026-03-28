#include "parser.h"

typedef struct {
    TokensList input;
    size_t index;
    String fileName;
} Parser;

static bool is_at_end(Parser *p) { return p->input.arr[p->index].kind == TOK_EOF; }

static Token previous(Parser *p) { return p->input.arr[p->index - 1]; }

static bool match_arr(Parser *p, size_t count, const TokenKind *tokens) {
    if (is_at_end(p)) return false;

    for (size_t i = 0; i < count; i++) {
        if (p->input.arr[p->index].kind == tokens[i]) {
            p->index++;
            return true;
        }
    }
    return false;
}

#define match(p, ...) match_arr(p, sizeof((TokenKind[]){__VA_ARGS__}) / sizeof(TokenKind), (TokenKind[]){__VA_ARGS__})

inline static Token peek(Parser *p) {
    if (is_at_end(p)) return (Token){0};
    return p->input.arr[p->index];
}

inline static Token peek_ahead(Parser *p, size_t offset) {
    if (p->index + offset >= p->input.len) return (Token){0};
    return p->input.arr[p->index + offset];
}

__attribute__((__noreturn__)) static void parse_error(Parser *p, const char *msg) {
    fprintf(stderr, "[%.*s:%zu:%zu]: Error: %s\n", (int)p->fileName.len, p->fileName.data, peek(p).line, peek(p).col,
            msg);
    abort();
}

// static void parse_warning(Parser *p, const char *msg) {
//     fprintf(stderr, "[%.*s:%zu:%zu]: Warning: %s\n", (int)p->fileName.len, p->fileName.data, peek(p).line, peek(p).col,
//             msg);
// }

static Token expect(Parser *p, TokenKind expected, const char *msg) {
    if (match(p, expected)) return previous(p);
    parse_error(p, msg);
}

/******************************************************************************
 * Expression Parsing
 *****************************************************************************/

static Expr *expression(Parser *p);

// primary := INTEGER_LITERAL | FLOAT_LITERAL | CHAR_LITERAL | STRING_LITERAL | TRUE | FALSE | IDENTIFIER |
// '(' expression ')'
static Expr *primary_expr(Parser *p) {
    if (match(p, TOK_INTEGER_LITERAL, TOK_FLOAT_LITERAL, TOK_CHAR_LITERAL, TOK_STRING_LITERAL, TOK_TRUE, TOK_FALSE,
              TOK_IDENTIFIER)) {
        Token primary = previous(p);
        return expr_make_primary(primary);
    } else if (match(p, TOK_LEFT_PAREN)) {
        Expr *inner = expression(p);
        expect(p, TOK_RIGHT_PAREN, "Missing closing ')'");
        return expr_make_grouping(inner);
    }

    parse_error(p, "Expected expression");
}

// postfix := primary { '++' | '--' }*
static Expr *postfix_expr(Parser *p) {
    Expr *expr = primary_expr(p);

    while (true) {
        if (match(p, TOK_PLUS_PLUS, TOK_MINUS_MINUS)) {
            TokenKind op = previous(p).kind;
            expr = expr_make_unary_post(op, expr);
        } else {
            break;
        }
    }

    return expr;
}

// unary := ('&' | '*' | '+' | '-' | '~' | '!' | '++' | '--') unary
//        | postfix
static Expr *unary_expr(Parser *p) {
    if (match(p, TOK_PLUS_PLUS, TOK_MINUS_MINUS, TOK_AMPERSAND, TOK_STAR, TOK_PLUS, TOK_MINUS, TOK_TILDE, TOK_BANG)) {
        TokenKind op = previous(p).kind;
        Expr *inner = unary_expr(p);
        return expr_make_unary(op, inner);
    }

    return postfix_expr(p);
}

// multiplicative := unary {('*' | '/' | '%') unary}*
static Expr *multiplicative_expr(Parser *p) {
    Expr *expr = unary_expr(p);

    while (match(p, TOK_STAR, TOK_SLASH, TOK_PERCENT)) {
        TokenKind op = previous(p).kind;
        Expr *rhs = unary_expr(p);
        expr = expr_make_binary(op, expr, rhs);
    }

    return expr;
}

// additive := multiplicative {('+' | '-') multiplicative}*
static Expr *additive_expr(Parser *p) {
    Expr *expr = multiplicative_expr(p);

    while (match(p, TOK_PLUS, TOK_MINUS)) {
        TokenKind op = previous(p).kind;
        Expr *rhs = multiplicative_expr(p);
        expr = expr_make_binary(op, expr, rhs);
    }

    return expr;
}

// shift := additive {('<<' | '>>') additive}*
static Expr *shift_expr(Parser *p) {
    Expr *expr = additive_expr(p);

    while (match(p, TOK_LESS_LESS, TOK_GREATER_GREATER)) {
        TokenKind op = previous(p).kind;
        Expr *rhs = additive_expr(p);
        expr = expr_make_binary(op, expr, rhs);
    }

    return expr;
}

// relational := shift {('<' | '>' | '<=' | '>=') shift}*
static Expr *relational_expr(Parser *p) {
    Expr *expr = shift_expr(p);

    while (match(p, TOK_LESS, TOK_GREATER, TOK_LESS_EQUALS, TOK_GREATER_EQUALS)) {
        TokenKind op = previous(p).kind;
        Expr *rhs = shift_expr(p);
        expr = expr_make_binary(op, expr, rhs);
    }

    return expr;
}

// equality := relational {('==' | '!=') relational}*
static Expr *equality_expr(Parser *p) {
    Expr *expr = relational_expr(p);

    while (match(p, TOK_EQUALS_EQUALS, TOK_BANG_EQUALS)) {
        TokenKind op = previous(p).kind;
        Expr *rhs = relational_expr(p);
        expr = expr_make_binary(op, expr, rhs);
    }

    return expr;
}

// bitwiseAnd := equality {'&' equality}*
static Expr *bitwise_and_expr(Parser *p) {
    Expr *expr = equality_expr(p);

    while (match(p, TOK_AMPERSAND)) {
        Expr *rhs = equality_expr(p);
        expr = expr_make_binary(TOK_AMPERSAND, expr, rhs);
    }

    return expr;
}

// bitwiseXor := bitwise_and {'^' bitwiseAnd}*
static Expr *bitwise_xor_expr(Parser *p) {
    Expr *expr = bitwise_and_expr(p);

    while (match(p, TOK_CARET)) {
        Expr *rhs = bitwise_and_expr(p);
        expr = expr_make_binary(TOK_CARET, expr, rhs);
    }

    return expr;
}

// bitwiseOr := bitwise_xor {'|' bitwiseXor}*
static Expr *bitwise_or_expr(Parser *p) {
    Expr *expr = bitwise_xor_expr(p);

    while (match(p, TOK_PIPE)) {
        Expr *rhs = bitwise_xor_expr(p);
        expr = expr_make_binary(TOK_PIPE, expr, rhs);
    }

    return expr;
}

// logicalAnd := bitwise_or {'&&' bitwiseOr}*
static Expr *logical_and_expr(Parser *p) {
    Expr *expr = bitwise_or_expr(p);

    while (match(p, TOK_AMPERSAND_AMPERSAND)) {
        Expr *rhs = bitwise_or_expr(p);
        expr = expr_make_binary(TOK_AMPERSAND_AMPERSAND, expr, rhs);
    }

    return expr;
}

// logicalOr := logical_and {'||' logicalAnd}*
static Expr *logical_or_expr(Parser *p) {
    Expr *expr = logical_and_expr(p);

    while (match(p, TOK_PIPE_PIPE)) {
        Expr *rhs = logical_and_expr(p);
        expr = expr_make_binary(TOK_PIPE_PIPE, expr, rhs);
    }

    return expr;
}

// conditional := logical_or {'?' expression ':' conditional}?
static Expr *conditional_expr(Parser *p) {
    Expr *expr = logical_or_expr(p);

    if (match(p, TOK_QUESTION_MARK)) {
        Expr *trueBranch = expression(p);
        expect(p, TOK_COLON, "Expected \':\'");
        Expr *falseBranch = conditional_expr(p);
        expr = expr_make_conditional(expr, trueBranch, falseBranch);
    }

    return expr;
}

// assignment := conditional {('=' | '+=' | '-=' | '*=' | '/=' | '%=' | '&=' | '^=' | '|=' | '<<=' | '>>=') assignment}*
static Expr *assignment_expr(Parser *p) {
    Expr *expr = conditional_expr(p);

    if (match(p, TOK_EQUALS, TOK_PLUS_EQUALS, TOK_MINUS_EQUALS, TOK_STAR_EQUALS, TOK_SLASH_EQUALS, TOK_PERCENT_EQUALS,
              TOK_AMPERSAND_EQUALS, TOK_CARET_EQUALS, TOK_PIPE_EQUALS, TOK_LESS_LESS_EQUALS,
              TOK_GREATER_GREATER_EQUALS)) {
        TokenKind op = previous(p).kind;
        Expr *rhs = assignment_expr(p);
        expr = expr_make_binary(op, expr, rhs);
    }

    return expr;
}

static Expr *expression(Parser *p) { return assignment_expr(p); }

/******************************************************************************
 * Statement Parsing
 *****************************************************************************/
Stmt *statement(Parser *p);

Stmt *var_stmt(Parser *p) {
    TokenKind type = previous(p).kind;
    Token name = expect(p, TOK_IDENTIFIER, "Missing variable name");
    Expr *init = NULL;
    if (match(p, TOK_EQUALS)) init = expression(p);

    return stmt_make_var(type, name, init);
}

Stmt *for_stmt(Parser *p) {
    expect(p, TOK_LEFT_PAREN, "Exected '(' after after for");

    Stmt *init = NULL;
    if (!match(p, TOK_SEMICOLON)) init = statement(p);
    expect(p, TOK_SEMICOLON, "Expected ';' after for-loop initializer");

    Expr *cond = NULL;
    if (!match(p, TOK_SEMICOLON)) cond = expression(p);
    expect(p, TOK_SEMICOLON, "Expected ';' after for-loop condition");

    Expr *inc = NULL;
    if (!match(p, TOK_RIGHT_PAREN)) inc = expression(p);
    expect(p, TOK_RIGHT_PAREN, "Expected ')' after for clauses");

    Stmt *body = statement(p);

    return stmt_make_for(init, cond, inc, body);
}

Stmt *while_stmt(Parser *p) {
    expect(p, TOK_LEFT_PAREN, "Exected '(' after after while");

    Expr *cond = expression(p);
    expect(p, TOK_RIGHT_PAREN, "Expected ')' after while condition");

    Stmt *body = statement(p);

    return stmt_make_while(cond, body);
}

Stmt *if_stmt(Parser *p) {
    expect(p, TOK_LEFT_PAREN, "Expected '(' after if");

    Expr *cond = expression(p);
    expect(p, TOK_RIGHT_PAREN, "Expected ')' after condition");

    Stmt *thenBranch = statement(p);

    Stmt *elseBranch = NULL;
    if (match(p, TOK_ELSE)) elseBranch = statement(p);

    return stmt_make_if(cond, thenBranch, elseBranch);
}

Stmt *block_stmt(Parser *p) {
    StmtList body = {0};

    while (!match(p, TOK_RIGHT_BRACE) && !is_at_end(p)) {
        Stmt *s = statement(p);
        list_append(&body, s);
    }

    return stmt_make_block(body);
}

Stmt *statement(Parser *p) {
    if (match(p, TOK_U8, TOK_U16, TOK_U32, TOK_U64, TOK_USIZE, TOK_I8, TOK_I16, TOK_I32, TOK_I64, TOK_ISIZE, TOK_F32,
              TOK_F64, TOK_BOOL))
        return var_stmt(p);
    if (match(p, TOK_FOR)) return for_stmt(p);
    if (match(p, TOK_WHILE)) return while_stmt(p);
    if (match(p, TOK_IF)) return if_stmt(p);
    if (match(p, TOK_LEFT_BRACE)) return block_stmt(p);

    Expr *expr = expression(p);
    return stmt_make_expr(expr);
}

StmtList parse(TokensList input, String fileName) {
    Parser p = (Parser){
        .input = input,
        .index = 0,
        .fileName = fileName,
    };

    StmtList ast = {0};

    while (is_at_end(&p)) {
        Stmt *s = statement(&p);
        list_append(&ast, s);
    }

    return ast;
}
