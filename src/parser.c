#include "parser.h"
#include "type.h"

typedef struct {
    TranslationUnit *unit;
    size_t index;
} Parser;

#define parser_error(p, fmt, ...) compile_error(p->unit->fileName, peek((p)).loc, fmt, ##__VA_ARGS__)

static bool is_at_end(Parser *p) { return p->unit->tokens.arr[p->index].kind == TOK_EOF; }

static Token previous(Parser *p) { return p->unit->tokens.arr[p->index - 1]; }

static bool match_arr(Parser *p, size_t count, const TokenKind *tokens) {
    if (is_at_end(p)) return false;

    for (size_t i = 0; i < count; i++) {
        if (p->unit->tokens.arr[p->index].kind == tokens[i]) {
            p->index++;
            return true;
        }
    }
    return false;
}

#define match(p, ...) match_arr(p, sizeof((TokenKind[]){__VA_ARGS__}) / sizeof(TokenKind), (TokenKind[]){__VA_ARGS__})

TokenKind types[] = {TOK_U8,  TOK_U16, TOK_U32,   TOK_U64, TOK_USIZE, TOK_I8,   TOK_I16,
                     TOK_I32, TOK_I64, TOK_ISIZE, TOK_F32, TOK_F64,   TOK_BOOL, TOK_VOID};
#define match_types(p) match_arr((p), sizeof(types) / sizeof(*types), types)

inline static Token peek(Parser *p) { return p->unit->tokens.arr[p->index]; }

inline static Token peek_ahead(Parser *p, size_t offset) {
    if (p->index + offset >= p->unit->tokens.len) return (Token){0};
    return p->unit->tokens.arr[p->index + offset];
}

static Token expect(Parser *p, TokenKind expected, const char *msg) {
    if (match(p, expected)) return previous(p);
    parser_error(p, msg);
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
        return expr_make_primary(primary, primary.loc);
    } else if (match(p, TOK_LEFT_PAREN)) {
        Location leftParenLoc = previous(p).loc;
        Expr *inner = expression(p);
        expect(p, TOK_RIGHT_PAREN, "Expected closing ')'");
        return expr_make_grouping(inner, leftParenLoc);
    }

    parser_error(p, "Expected expression");
}

// postfix := primary { '++' | '--' }*
//          | postfix_expr '(' argument_list ')'
static Expr *postfix_expr(Parser *p) {
    Expr *expr = primary_expr(p);

    while (true) {
        if (match(p, TOK_PLUS_PLUS, TOK_MINUS_MINUS)) {
            Token op = previous(p);
            expr = expr_make_unary_post(op.kind, expr, op.loc);
        } else if (match(p, TOK_LEFT_PAREN)) {
            ExprList args = {0};
            if (!match(p, TOK_RIGHT_PAREN)) {
                while (true) {
                    Expr *arg = expression(p);
                    list_append(&args, arg);

                    if (!match(p, TOK_COMMA)) {
                        expect(p, TOK_RIGHT_PAREN, "Expected ')'");
                        break;
                    }
                }
            }

            expr = expr_make_func_call(expr, args, expr->loc);
        } else if (match(p, TOK_LEFT_BRACKET)) {
            Expr *index = expression(p);
            expect(p, TOK_RIGHT_BRACKET, "Expected ']'");
            expr = expr_make_index(expr, index, expr->loc);
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
        Token op = previous(p);
        Expr *inner = unary_expr(p);
        return expr_make_unary(op.kind, inner, op.loc);
    }

    return postfix_expr(p);
}

// multiplicative := unary {('*' | '/' | '%') unary}*
static Expr *multiplicative_expr(Parser *p) {
    Expr *expr = unary_expr(p);

    while (match(p, TOK_STAR, TOK_SLASH, TOK_PERCENT)) {
        Token op = previous(p);
        Expr *rhs = unary_expr(p);
        expr = expr_make_binary(op.kind, expr, rhs, op.loc);
    }

    return expr;
}

// additive := multiplicative {('+' | '-') multiplicative}*
static Expr *additive_expr(Parser *p) {
    Expr *expr = multiplicative_expr(p);

    while (match(p, TOK_PLUS, TOK_MINUS)) {
        Token op = previous(p);
        Expr *rhs = multiplicative_expr(p);
        expr = expr_make_binary(op.kind, expr, rhs, op.loc);
    }

    return expr;
}

// shift := additive {('<<' | '>>') additive}*
static Expr *shift_expr(Parser *p) {
    Expr *expr = additive_expr(p);

    while (match(p, TOK_LESS_LESS, TOK_GREATER_GREATER)) {
        Token op = previous(p);
        Expr *rhs = additive_expr(p);
        expr = expr_make_binary(op.kind, expr, rhs, op.loc);
    }

    return expr;
}

// relational := shift {('<' | '>' | '<=' | '>=') shift}*
static Expr *relational_expr(Parser *p) {
    Expr *expr = shift_expr(p);

    while (match(p, TOK_LESS, TOK_GREATER, TOK_LESS_EQUALS, TOK_GREATER_EQUALS)) {
        Token op = previous(p);
        Expr *rhs = shift_expr(p);
        expr = expr_make_binary(op.kind, expr, rhs, op.loc);
    }

    return expr;
}

// equality := relational {('==' | '!=') relational}*
static Expr *equality_expr(Parser *p) {
    Expr *expr = relational_expr(p);

    while (match(p, TOK_EQUALS_EQUALS, TOK_BANG_EQUALS)) {
        Token op = previous(p);
        Expr *rhs = relational_expr(p);
        expr = expr_make_binary(op.kind, expr, rhs, op.loc);
    }

    return expr;
}

// bitwiseAnd := equality {'&' equality}*
static Expr *bitwise_and_expr(Parser *p) {
    Expr *expr = equality_expr(p);

    while (match(p, TOK_AMPERSAND)) {
        Token op = previous(p);
        Expr *rhs = equality_expr(p);
        expr = expr_make_binary(op.kind, expr, rhs, op.loc);
    }

    return expr;
}

// bitwiseXor := bitwise_and {'^' bitwiseAnd}*
static Expr *bitwise_xor_expr(Parser *p) {
    Expr *expr = bitwise_and_expr(p);

    while (match(p, TOK_CARET)) {
        Token op = previous(p);
        Expr *rhs = bitwise_and_expr(p);
        expr = expr_make_binary(op.kind, expr, rhs, op.loc);
    }

    return expr;
}

// bitwiseOr := bitwise_xor {'|' bitwiseXor}*
static Expr *bitwise_or_expr(Parser *p) {
    Expr *expr = bitwise_xor_expr(p);

    while (match(p, TOK_PIPE)) {
        Token op = previous(p);
        Expr *rhs = bitwise_xor_expr(p);
        expr = expr_make_binary(op.kind, expr, rhs, op.loc);
    }

    return expr;
}

// logicalAnd := bitwise_or {'&&' bitwiseOr}*
static Expr *logical_and_expr(Parser *p) {
    Expr *expr = bitwise_or_expr(p);

    while (match(p, TOK_AMPERSAND_AMPERSAND)) {
        Token op = previous(p);
        Expr *rhs = bitwise_or_expr(p);
        expr = expr_make_binary(op.kind, expr, rhs, op.loc);
    }

    return expr;
}

// logicalOr := logical_and {'||' logicalAnd}*
static Expr *logical_or_expr(Parser *p) {
    Expr *expr = logical_and_expr(p);

    while (match(p, TOK_PIPE_PIPE)) {
        Token op = previous(p);
        Expr *rhs = logical_and_expr(p);
        expr = expr_make_binary(op.kind, expr, rhs, op.loc);
    }

    return expr;
}

// conditional := logical_or {'?' expression ':' conditional}?
static Expr *conditional_expr(Parser *p) {
    Expr *expr = logical_or_expr(p);

    if (match(p, TOK_QUESTION_MARK)) {
        Location qmarkLoc = previous(p).loc;
        Expr *trueBranch = expression(p);
        expect(p, TOK_COLON, "Expected \':\'");
        Expr *falseBranch = conditional_expr(p);
        expr = expr_make_conditional(expr, trueBranch, falseBranch, qmarkLoc);
    }

    return expr;
}

// assignment := conditional {('=' | '+=' | '-=' | '*=' | '/=' | '%=' | '&=' | '^=' | '|=' | '<<=' | '>>=') assignment}*
static Expr *assignment_expr(Parser *p) {
    Expr *expr = conditional_expr(p);

    if (match(p, TOK_EQUALS, TOK_PLUS_EQUALS, TOK_MINUS_EQUALS, TOK_STAR_EQUALS, TOK_SLASH_EQUALS, TOK_PERCENT_EQUALS,
              TOK_AMPERSAND_EQUALS, TOK_CARET_EQUALS, TOK_PIPE_EQUALS, TOK_LESS_LESS_EQUALS,
              TOK_GREATER_GREATER_EQUALS)) {
        Token op = previous(p);
        Expr *rhs = assignment_expr(p);
        expr = expr_make_assign(op.kind, expr, rhs, op.loc);
    }

    return expr;
}

static Expr *expression(Parser *p) { return assignment_expr(p); }

/******************************************************************************
 * Statement Parsing
 *****************************************************************************/

Stmt *statement(Parser *p);
Type *parse_type(Parser *p);

Stmt *var_stmt(Parser *p) {
    Type *type = parse_type(p);

    Token name = expect(p, TOK_IDENTIFIER, "Expected variable name");
    Expr *init = NULL;
    if (match(p, TOK_EQUALS)) init = expression(p);

    expect(p, TOK_SEMICOLON, "Expected ';' after declaration");

    return stmt_make_var(type, name, init, name.loc);
}

Stmt *for_stmt(Parser *p) {
    Location loc = previous(p).loc;
    expect(p, TOK_LEFT_PAREN, "Exected '(' after after for");

    Stmt *init = NULL;
    if (match_types(p))
        init = var_stmt(p);
    else if (!match(p, TOK_SEMICOLON)) {
        init = stmt_make_expr(expression(p), previous(p).loc);
        expect(p, TOK_SEMICOLON, "Expected ';' after expression");
    }

    Expr *cond = NULL;
    if (!match(p, TOK_SEMICOLON)) {
        cond = expression(p);
        expect(p, TOK_SEMICOLON, "Expected ';' after for-loop condition");
    }

    Expr *inc = NULL;
    if (!match(p, TOK_RIGHT_PAREN)) {
        inc = expression(p);
        expect(p, TOK_RIGHT_PAREN, "Expected ')' after for clauses");
    }

    Stmt *body = statement(p);

    return stmt_make_for(init, cond, inc, body, loc);
}

Stmt *while_stmt(Parser *p) {
    Location loc = previous(p).loc;
    expect(p, TOK_LEFT_PAREN, "Exected '(' after after while");

    Expr *cond = expression(p);
    expect(p, TOK_RIGHT_PAREN, "Expected ')' after while condition");

    Stmt *body = statement(p);

    return stmt_make_while(cond, body, loc);
}

Stmt *if_stmt(Parser *p) {
    Location loc = previous(p).loc;
    expect(p, TOK_LEFT_PAREN, "Expected '(' after if");

    Expr *cond = expression(p);
    expect(p, TOK_RIGHT_PAREN, "Expected ')' after condition");

    Stmt *thenBranch = statement(p);

    Stmt *elseBranch = NULL;
    if (match(p, TOK_ELSE)) elseBranch = statement(p);

    return stmt_make_if(cond, thenBranch, elseBranch, loc);
}

StmtList stmt_list(Parser *p) {
    StmtList body = {0};

    while (!match(p, TOK_RIGHT_BRACE) && !is_at_end(p)) {
        Stmt *s = statement(p);
        list_append(&body, s);
    }

    return body;
}

Stmt *block_stmt(Parser *p) {
    Location loc = previous(p).loc;
    return stmt_make_block(stmt_list(p), loc);
}

Stmt *break_stmt(Parser *p) {
    Location loc = previous(p).loc;
    expect(p, TOK_SEMICOLON, "Expected ';' after break");
    return stmt_make_break(loc);
}

Stmt *continue_stmt(Parser *p) {
    Location loc = previous(p).loc;
    expect(p, TOK_SEMICOLON, "Expected ';' after continue");
    return stmt_make_continue(loc);
}

Stmt *return_stmt(Parser *p) {
    Location loc = previous(p).loc;
    Expr *ret_val = NULL;
    if (!match(p, TOK_SEMICOLON)) {
        ret_val = expression(p);
        expect(p, TOK_SEMICOLON, "Expected ';'");
    }
    return stmt_make_return(ret_val, loc);
}

Stmt *statement(Parser *p) {
    if (match_types(p)) return var_stmt(p);
    if (match(p, TOK_FOR)) return for_stmt(p);
    if (match(p, TOK_WHILE)) return while_stmt(p);
    if (match(p, TOK_IF)) return if_stmt(p);
    if (match(p, TOK_LEFT_BRACE)) return block_stmt(p);
    if (match(p, TOK_BREAK)) return break_stmt(p);
    if (match(p, TOK_CONTINUE)) return continue_stmt(p);
    if (match(p, TOK_RETURN)) return return_stmt(p);

    Location loc = peek(p).loc;
    Expr *expr = expression(p);
    expect(p, TOK_SEMICOLON, "Expected ';' after expression");
    return stmt_make_expr(expr, loc);
}

StmtList params_list(Parser *p) {
    StmtList params = {0};
    if (match(p, TOK_RIGHT_PAREN)) return params;

    while (true) {
        if (!match_types(p)) parser_error(p, "Expected parameter type");
        Type *type = parse_type(p);
        expect(p, TOK_IDENTIFIER, "Expected parameter name");
        Token name = previous(p);
        Stmt *param = stmt_make_var(type, name, NULL, name.loc);
        list_append(&params, param);
        if (!match(p, TOK_COMMA)) {
            expect(p, TOK_RIGHT_PAREN, "Expected ')'");
            break;
        }
    }

    return params;
}

Type *parse_type(Parser *p) {
    TokenKind typeToken = previous(p).kind;
    Type *baseType = type_make_primitive(TokenToPrimitive[typeToken]);

    while (!(peek(p).kind == TOK_IDENTIFIER)) {
        if (match(p, TOK_STAR)) {
            baseType = type_make_pointer(baseType);
        } else if (match(p, TOK_LEFT_BRACKET)) {
            Expr *len = NULL;
            if (!match(p, TOK_RIGHT_BRACKET)) {
                len = expression(p);
                expect(p, TOK_RIGHT_BRACKET, "Expected ']'");
            }
            baseType = type_make_array(baseType, len);
        } else if (match(p, TOK_LEFT_PAREN)) {
            StmtList params = params_list(p);
            baseType = type_make_func(baseType, params);
        } else {
            parser_error(p, "Expected '*', '[', '(', or identifier after type");
        }
    }

    return baseType;
}

Stmt *func_def_stmt(Parser *p) {
    Type *type = parse_type(p);
    Token name = expect(p, TOK_IDENTIFIER, "Expected function name");

    expect(p, TOK_LEFT_PAREN, "Expectd '('");
    StmtList params = params_list(p);

    expect(p, TOK_LEFT_BRACE, "Expected '{'");
    StmtList body = stmt_list(p);

    Type *funcType = type_make_func(type, params);
    return stmt_make_func(funcType, name, body, name.loc);
}

Stmt *top_level_decl(Parser *p) {
    if (match_types(p)) return func_def_stmt(p);

    parser_error(p, "Unkown top level declaration");
}

void translation_unit(Parser *p) {
    while (!is_at_end(p)) {
        Stmt *s = top_level_decl(p);
        list_append(&p->unit->ast, s);
    }
}

void parse(TranslationUnit *unit) {
    Parser p = (Parser){
        .unit = unit,
        .index = 0,
    };

    translation_unit(&p);
}
