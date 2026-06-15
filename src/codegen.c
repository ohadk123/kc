#include "codegen.h"
#include <stdarg.h>

static uint64_t counter(void) {
    static uint64_t i = 0;
    return i++;
}

static String qbe_var(Location l) {
    uint64_t i = counter();
    return str_printf("%%_ktemp_%zu_%zu_%zu", i, l.line, l.col);
}

typedef struct {
    FILE *outf;
    TranslationUnit *unit;
} Generator;

// static int gprintf(Generator *g, const char *fmt, ...) {
//     va_list args;
//     va_start(args, fmt);
//     int ret = vfprintf(g->outf, fmt, args);
//     va_end(args);
//     return ret;
// }

static int gprintfln(Generator *g, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int ret = vfprintf(g->outf, fmt, args);
    va_end(args);
    fprintf(g->outf, "\n");
    return ret;
}

/******************************************************************************
 * Expression CodeGen
 *****************************************************************************/

static String gen_expr(Generator *g, Expr *e);

static String gen_primary(Generator *g, Expr *e) {
    assert(e->kind == EXPR_PRIMARY);
    (void) g;
    Token val = e->as.primary.value;

    switch (val.kind) {
        case TOK_IDENTIFIER:      TODO("%s: TOK_IDENTIFIER", __func__);
        case TOK_STRING_LITERAL:  TODO("%s: TOK_STRING_LITERAL", __func__);
        case TOK_CHAR_LITERAL:    return str_printf("%u", val.as.charLiteral);
        case TOK_INTEGER_LITERAL: return str_printf("%zu", val.as.integerLiteral);
        case TOK_FLOAT_LITERAL:   TODO("%s: TOK_FLOAT_LITERAL", __func__);
        case TOK_TRUE:            return str_from_cstr("1");
        case TOK_FALSE:           return str_from_cstr("0");
        default:                  UNREACHABLE("%s: Unsupported token kind: %s", __func__, tokenTypesStrings[val.kind]);
    }
}

static String gen_binary(Generator *g, Expr *e) {
    (void)g;
    (void)e;
    TODO("%s", __func__);
}

static String gen_unary(Generator *g, Expr *e) {
    assert(e->kind == EXPR_UNARY);
    UnaryExpr un = e->as.unary;

    String out = qbe_var(e->loc);
    switch (un.op) {
        case TOK_MINUS: {
            String inner = gen_expr(g, un.inner);
            gprintfln(g, "%.*s =w sub 0, %.*s", strf(out), strf(inner));
            break;
        }

        case TOK_TILDE: {
            String inner = gen_expr(g, un.inner);
            gprintfln(g, "%.*s =w xor %.*s, -1", strf(out), strf(inner));
            break;
        }

        default: TODO("%s: Unary operator \"%s\"", __func__, tokenTypesStrings[un.op]);
    }

    return out;
}

static String gen_assign(Generator *g, Expr *e) {
    (void)g;
    (void)e;
    TODO("%s", __func__);
}

static String gen_unary_post(Generator *g, Expr *e) {
    (void)g;
    (void)e;
    TODO("%s", __func__);
}

static String gen_conditional(Generator *g, Expr *e) {
    (void)g;
    (void)e;
    TODO("%s", __func__);
}

static String gen_func_call(Generator *g, Expr *e) {
    (void)g;
    (void)e;
    TODO("%s", __func__);
}

static String gen_index(Generator *g, Expr *e) {
    (void)g;
    (void)e;
    TODO("%s", __func__);
}

static String gen_expr(Generator *g, Expr *e) {
    switch (e->kind) {
        case EXPR_PRIMARY:     return gen_primary(g, e);
        case EXPR_GROUPING:    return gen_expr(g, e->as.grouping.inner);
        case EXPR_BINARY:      return gen_binary(g, e);
        case EXPR_UNARY:       return gen_unary(g, e);
        case EXPR_ASSIGN:      return gen_assign(g, e);
        case EXPR_UNARY_POST:  return gen_unary_post(g, e);
        case EXPR_CONDITIONAL: return gen_conditional(g, e);
        case EXPR_FUNC_CALL:   return gen_func_call(g, e);
        case EXPR_INDEX:       return gen_index(g, e);
    }
    UNREACHABLE("Error on expr kind (%d)", e->kind);
}

/******************************************************************************
 * Statement CodeGen
 *****************************************************************************/

static void gen_stmt(Generator *g, Stmt *s);

static void gen_block(Generator *g, Stmt *s) {
    (void)g;
    (void)s;
    TODO("%s", __func__);
}

static void gen_func(Generator *g, Stmt *s) {
    gprintfln(g, "export function w $main() {");
    gprintfln(g, "@start");

    StmtList body = s->as.func.body;
    for (size_t i = 0; i < body.len; i++) gen_stmt(g, body.arr[i]);

    gprintfln(g, "}\n");
}

static void gen_return(Generator *g, Stmt *s) {
    assert(s->kind == STMT_RETURN);
    Expr *retVal = s->as.returnS.retVal;

    if (!retVal)
        gprintfln(g, "ret");
    else
        gprintfln(g, "ret %s", gen_expr(g, retVal));
}

static void gen_var(Generator *g, Stmt *s) {
    (void)g;
    (void)s;
    TODO("%s", __func__);
}

static void gen_while(Generator *g, Stmt *s) {
    (void)g;
    (void)s;
    TODO("%s", __func__);
}

static void gen_do_while(Generator *g, Stmt *s) {
    (void)g;
    (void)s;
    TODO("%s", __func__);
}

static void gen_if(Generator *g, Stmt *s) {
    (void)g;
    (void)s;
    TODO("%s", __func__);
}

static void gen_for(Generator *g, Stmt *s) {
    (void)g;
    (void)s;
    TODO("%s", __func__);
}

static void gen_break(Generator *g, Stmt *s) {
    (void)g;
    (void)s;
    TODO("%s", __func__);
}

static void gen_continue(Generator *g, Stmt *s) {
    (void)g;
    (void)s;
    TODO("%s", __func__);
}

static void gen_stmt(Generator *g, Stmt *s) {
    switch (s->kind) {
        case STMT_NULL:     break;
        case STMT_BLOCK:    gen_block(g, s); break;
        case STMT_FUNC:     gen_func(g, s); break;
        case STMT_RETURN:   gen_return(g, s); break;
        case STMT_VAR:      gen_var(g, s); break;
        case STMT_EXPR:     gen_expr(g, s->as.expr.expr); break;
        case STMT_WHILE:    gen_while(g, s); break;
        case STMT_DO_WHILE: gen_do_while(g, s); break;
        case STMT_IF:       gen_if(g, s); break;
        case STMT_FOR:      gen_for(g, s); break;
        case STMT_BREAK:    gen_break(g, s); break;
        case STMT_CONTINUE: gen_continue(g, s); break;
    }
}

void codegen(TranslationUnit *unit, FILE *outf) {
    if (!outf) outf = stdout;

    Generator g = (Generator){
        .outf = outf,
        .unit = unit,
    };

    for (size_t i = 0; i < unit->ast.len; i++) {
        gen_stmt(&g, unit->ast.arr[i]);
    }
}
