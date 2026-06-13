#include "sema.h"
#include "compiler.h"

bool fill_global_symbol_table(TranslationUnit *unit) {
    bool hadError = false;

    for (size_t i = 0; i < unit->ast.len; i++) {
        TLStmt *s = unit->ast.arr[i];
        Token nameTok = s->name;
        assert(nameTok.kind == TOK_IDENTIFIER);
        String key = nameTok.as.identifier;

        if (!hm_insert(&unit->globalSymbols.symbols, key, s)) {
            Stmt *first = hm_find_val(&unit->globalSymbols.symbols, key);
            hadError =
                compile_err_no_abort(unit->fileName, s->loc, "symbol '%.*s' already delcared before at [%.*s:%zu:%zu]",
                                     strf(key), strf(unit->fileName), first->loc.line, first->loc.col);
        }
    }

    return hadError;
}

// static Stmt *find_symbol(Scope *scope, String symbol) {
//     while (scope) {
//         Stmt *var = hm_find_val(&scope->symbols, symbol);
//         if (var) return var;
//         scope = scope->above;
//     }
//
//     return 0;
// }
//
// static bool is_lvalue(Expr *e) {
//     if (e->kind == EXPR_PRIMARY && e->as.primary.value.kind == TOK_IDENTIFIER) return true;
//     if (e->kind == EXPR_GROUPING) return is_lvalue(e->as.grouping.inner);
//     if (e->kind == EXPR_UNARY && e->as.unary.op == TOK_STAR) return true;
//
//     return false;
// }

typedef struct {
    TranslationUnit *unit;
    Scope *curr;
    bool hadError;
} Checker;

static void check_primary(Checker *c, Expr *e) {
    (void)c;
    (void)e;
    TODO("%s", __func__);
}

static void check_binary(Checker *c, Expr *e) {
    (void)c;
    (void)e;
    TODO("%s", __func__);
}

static void check_unary(Checker *c, Expr *e) {
    (void)c;
    (void)e;
    TODO("%s", __func__);
}

static void check_unary_post(Checker *c, Expr *e) {
    (void)c;
    (void)e;
    TODO("%s", __func__);
}

static void check_assign(Checker *c, Expr *e) {
    (void)c;
    (void)e;
    TODO("%s", __func__);
}

static void check_conditional(Checker *c, Expr *e) {
    (void)c;
    (void)e;
    TODO("%s", __func__);
}

static void check_func_call(Checker *c, Expr *e) {
    (void)c;
    (void)e;
    TODO("%s", __func__);
}

static void check_index(Checker *c, Expr *e) {
    (void)c;
    (void)e;
    TODO("%s", __func__);
}

static void check_expr(Checker *c, Expr *e) {
    switch (e->kind) {
        case EXPR_PRIMARY:     check_primary(c, e);                 return;
        case EXPR_GROUPING:    check_expr(c, e->as.grouping.inner); return;
        case EXPR_BINARY:      check_binary(c, e);                  return;
        case EXPR_UNARY:       check_unary(c, e);                   return;
        case EXPR_ASSIGN:      check_assign(c, e);                  return;
        case EXPR_UNARY_POST:  check_unary_post(c, e);              return;
        case EXPR_CONDITIONAL: check_conditional(c, e);             return;
        case EXPR_FUNC_CALL:   check_func_call(c, e);               return;
        case EXPR_INDEX:       check_index(c, e);                   return;
    }
}

static void check_stmt(Checker *c, Stmt *s);

static void check_var(Checker *c, Stmt *s) {
    (void)c;
    (void)s;
    TODO("%s", __func__);
}

static void check_block(Checker *c, Stmt *s) {
    (void)c;
    (void)s;
    TODO("%s", __func__);
}

static void check_while(Checker *c, Stmt *s) {
    (void)c;
    (void)s;
    TODO("%s", __func__);
}

static void check_if(Checker *c, Stmt *s) {
    (void)c;
    (void)s;
    TODO("%s", __func__);
}

static void check_for(Checker *c, Stmt *s) {
    (void)c;
    (void)s;
    TODO("%s", __func__);
}

static void check_return(Checker *c, Stmt *s) {
    (void)c;
    (void)s;
    TODO("%s", __func__);
}

static void check_break(Checker *c, Stmt *s) {
    (void)c;
    (void)s;
    TODO("%s", __func__);
}

static void check_continue(Checker *c, Stmt *s) {
    (void)c;
    (void)s;
    TODO("%s", __func__);
}

static void check_stmt(Checker *c, Stmt *s) {
    switch (s->kind) {
        case STMT_NULL: return;
        case STMT_VAR:      check_var(c, s);                return;
        case STMT_EXPR:     check_expr(c, s->as.expr.expr); return;
        case STMT_BLOCK:    check_block(c, s);              return;
        case STMT_WHILE:    check_while(c, s);              return;
        case STMT_DO_WHILE: check_while(c, s);              return;
        case STMT_IF:       check_if(c, s);                 return;
        case STMT_FOR:      check_for(c, s);                return;
        case STMT_RETURN:   check_return(c, s);             return;
        case STMT_BREAK:    check_break(c, s);              return;
        case STMT_CONTINUE: check_continue(c, s);           return;
    }
}

static void check_func(Checker *c, TLStmt *s) {
    assert(s->kind == TLSTMT_FUNC);
    (void) c;
    StmtList body = s->as.func.body;
    for (size_t i = 0; i < body.len; i++) {
        check_stmt(c, body.arr[i]);
    }
}

static void check_tlstmt(Checker *c, TLStmt *s) {
    switch (s->kind) {
        case TLSTMT_FUNC: check_func(c, s); return;
    }
}

bool semantic_analysis(TranslationUnit *unit) {
    Checker c = {
        .unit = unit,
        .curr = &unit->globalSymbols,
        .hadError = false,
    };

    c.hadError = fill_global_symbol_table(unit);

    for (size_t i = 0; i < unit->ast.len; i++) {
        check_tlstmt(&c, unit->ast.arr[i]);
    }

    return !c.hadError;
}
