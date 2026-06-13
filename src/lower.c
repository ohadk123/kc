#include "lower.h"
#include <stdarg.h>

static uint64_t counter(void) {
    static uint64_t count = 0;
    return count++;
}

static String get_label(const char *name, Location loc) {
    return str_printf("__%s.%zu.%zu.%zu", name, loc.line, loc.col, counter());
}

typedef struct {
    FILE *outf;
    TranslationUnit *unit;
} Generator;

static int gprintf(Generator *g, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int ret = vfprintf(g->outf, fmt, args);
    va_end(args);
    return ret;
}

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

static String lower_expr(Generator *g, Expr *e);

static String lower_primary(Generator *g, Expr *e) {
    (void) g;
    (void) e;
    TODO("%s", __func__);
}

static String lower_binary(Generator *g, Expr *e) {
    (void) g;
    (void) e;
    TODO("%s", __func__);
}

// static String lower_lvalue(Generator *g, Expr *e) {
//     switch (e->kind) {
//         case EXPR_PRIMARY: {
//             if (e->as.primary.value.kind != TOK_IDENTIFIER) break;
//             return expect_var(g, e->as.primary.value)->as.var.qbeVarAddr;
//         }
//         case EXPR_GROUPING: return lower_lvalue(g, e->as.grouping.inner);
//         case EXPR_UNARY:    {
//             if (e->as.unary.op != TOK_STAR) break;
//             return lower_expr(g, e->as.unary.inner);
//         }
//         default: break;
//     }
//
//     compile_error(g->unit->fileName, e->loc, "expression is not assignable");
// }

static String lower_unary(Generator *g, Expr *e) {
    (void) g;
    (void) e;
    TODO("%s", __func__);
}

static String lower_assign(Generator *g, Expr *e) {
    (void) g;
    (void) e;
    TODO("%s", __func__);
}

static String lower_unary_post(Generator *g, Expr *e) {
    (void) g;
    (void) e;
    TODO("%s", __func__);
}

static String lower_conditional(Generator *g, Expr *e) {
    (void) g;
    (void) e;
    TODO("%s", __func__);
}

static String lower_func_call(Generator *g, Expr *e) {
    (void) g;
    (void) e;
    TODO("%s", __func__);
}

static String lower_index(Generator *g, Expr *e) {
    (void)g;
    (void)e;
    TODO("%s", __func__);
}

static String lower_expr(Generator *g, Expr *e) {
    switch (e->kind) {
        case EXPR_PRIMARY:     return lower_primary(g, e);
        case EXPR_GROUPING:    return lower_expr(g, e->as.grouping.inner);
        case EXPR_BINARY:      return lower_binary(g, e);
        case EXPR_UNARY:       return lower_unary(g, e);
        case EXPR_ASSIGN:      return lower_assign(g, e);
        case EXPR_UNARY_POST:  return lower_unary_post(g, e);
        case EXPR_CONDITIONAL: return lower_conditional(g, e);
        case EXPR_FUNC_CALL:   return lower_func_call(g, e);
        case EXPR_INDEX:       return lower_index(g, e);
    }
    UNREACHABLE("Error on expr kind (%d)", e->kind);
}

/******************************************************************************
 * Statement CodeGen
 *****************************************************************************/

static void lower_stmt(Generator *g, Stmt *s);

static void lower_block(Generator *g, Stmt *s) {
    (void)g;
    (void)s;
    TODO("%s", __func__);
}

static void lower_return(Generator *g, Stmt *s) {
    (void)g;
    (void)s;
    TODO("%s", __func__);
}

static void lower_var(Generator *g, Stmt *s) {
    (void)g;
    (void)s;
    TODO("%s", __func__);
}

static void lower_while(Generator *g, Stmt *s) {
    (void)g;
    (void)s;
    TODO("%s", __func__);
}

static void lower_do_while(Generator *g, Stmt *s) {
    (void)g;
    (void)s;
    TODO("%s", __func__);
}

static void lower_if(Generator *g, Stmt *s) {
    (void)g;
    (void)s;
    TODO("%s", __func__);
}

static void lower_for(Generator *g, Stmt *s) {
    (void)g;
    (void)s;
    TODO("%s", __func__);
}

static void lower_break(Generator *g, Stmt *s) {
    (void)g;
    (void)s;
    TODO("%s", __func__);
}

static void lower_continue(Generator *g, Stmt *s) {
    (void)g;
    (void)s;
    TODO("%s", __func__);
}

static void lower_stmt(Generator *g, Stmt *s) {
    switch (s->kind) {
        case STMT_NULL:                                   break;
        case STMT_BLOCK:    lower_block(g, s);              break;
        case STMT_RETURN:   lower_return(g, s);             break;
        case STMT_VAR:      lower_var(g, s);                break;
        case STMT_EXPR:     lower_expr(g, s->as.expr.expr); break;
        case STMT_WHILE:    lower_while(g, s);              break;
        case STMT_DO_WHILE: lower_do_while(g, s);           break;
        case STMT_IF:       lower_if(g, s);                 break;
        case STMT_FOR:      lower_for(g, s);                break;
        case STMT_BREAK:    lower_break(g, s);              break;
        case STMT_CONTINUE: lower_continue(g, s);           break;
    }
    UNREACHABLE("Error on stmt kind (%d)", s->kind);
}

static void lower_func(Generator *g, Tld *d) {
    (void)g;
    (void)d;
    (void) lower_stmt;
    TODO("%s", __func__);
}

static void lower_tld(Generator *g, Tld *d) {
    switch (d->kind) {
        case TLD_FUNC: lower_func(g, d); return;
    }
    UNREACHABLE("Error on top level decl kind (%d)", d->kind);
}

void lower_ast(TranslationUnit *unit, FILE *outf) {
    if (!outf) outf = stdout;

    Generator g = (Generator){
        .outf = outf,
        .unit = unit,
    };

    (void) gprintf(&g, "");
    (void) gprintfln(&g, "");
    (void) get_label("", (Location){0});

    for (size_t i = 0; i < unit->ast.len; i++) {
        lower_tld(&g, unit->ast.arr[i]);
    }
}
