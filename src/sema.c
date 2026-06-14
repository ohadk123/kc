#include "sema.h"
#include "compiler.h"

bool fill_global_symbol_table(TranslationUnit *unit) {
    bool hadError = false;

    for (size_t i = 0; i < unit->ast.len; i++) {
        Stmt *s = unit->ast.arr[i];
        Token nameTok = get_top_level_name(s);
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

static bool is_lvalue(Expr *e) {
    if (e->kind == EXPR_PRIMARY && e->as.primary.value.kind == TOK_IDENTIFIER) return true; // variable
    if (e->kind == EXPR_GROUPING) return is_lvalue(e->as.grouping.inner); // inner expression is lvalue? (x)++
    if (e->kind == EXPR_UNARY && e->as.unary.op == TOK_STAR) return true; // pointer dereference

    return false;
}

typedef struct {
    TranslationUnit *unit;
    Scope *curr;
    bool hadError;
} Checker;

#define checker_error(c, o, fmt, ...) \
    c->hadError = compile_err_no_abort((c)->unit->fileName, (o)->loc, fmt, ##__VA_ARGS__)

/******************************************************************************
 * Scope Helpers
 *****************************************************************************/

static void enter_scope(Checker *c) {
    Scope *new = calloc(1, sizeof(Scope));
    new->above = c->curr;
    c->curr = new;
}

static void exit_scope(Checker *c) {
    Scope *old = c->curr;
    c->curr = old->above;
    free(old);
}

/******************************************************************************
 * Symbol Resolution
 *****************************************************************************/

// Find symbol is scope bottom to top
static Stmt *find_symbol(Scope *scope, String symbol) {
    while (scope) {
        Stmt *var = hm_find_val(&scope->symbols, symbol);
        if (var) return var;
        scope = scope->above;
    }

    return NULL;
}

static Stmt *expect_var(Checker *c, Token nameTok) {
    assert(nameTok.kind == TOK_IDENTIFIER);
    Stmt *found = find_symbol(c->curr, nameTok.as.identifier);
    if (!found) compile_error(c->unit->fileName, nameTok.loc, "Unkown symbol '%.*s'", strf(nameTok.as.identifier));
    return found;
}

static void declare_var(Checker *c, Stmt *varStmt) {
    assert(varStmt->kind == STMT_VAR);

    String varName = varStmt->as.var.name.as.identifier;
    Stmt *found = hm_find_val(&c->curr->symbols, varName);
    if (found)
        compile_error(c->unit->fileName, varStmt->loc, "Symbol '%.*s' already delcared before at [%.*s:%zu:%zu]",
                      strf(varName), strf(c->unit->fileName), varStmt->loc.line, varStmt->loc.col);

    hm_insert(&c->curr->symbols, varName, varStmt);
}


static void check_expr(Checker *c, Expr *e);
static void check_stmt(Checker *c, Stmt *s);

static void check_primary(Checker *c, Expr *e) {
    assert(e->kind == EXPR_PRIMARY);
    Token prim = e->as.primary.value;

    (void) c;
    switch (prim.kind) {
        case TOK_IDENTIFIER:      TODO("%s: TOK_IDENTIFIER", __func__);
        case TOK_STRING_LITERAL:  TODO("%s: TOK_STRING_LITERAL", __func__);
        case TOK_CHAR_LITERAL:    return;
        case TOK_INTEGER_LITERAL: return;
        case TOK_FLOAT_LITERAL:   return;
        case TOK_TRUE:            return;
        case TOK_FALSE:           return;
        default:                  UNREACHABLE("%s: Unsupported token kind: %s", __func__, tokenTypesStrings[prim.kind]);
    }
}

static void check_binary(Checker *c, Expr *e) {
    assert(e->kind == EXPR_BINARY);
    BinaryExpr bin = e->as.binary;

    check_expr(c, bin.lhs);
    check_expr(c, bin.rhs);
}

static void check_unary(Checker *c, Expr *e) {
    assert(e->kind == EXPR_UNARY || e->kind == EXPR_UNARY_POST);
    UnaryExpr un = e->as.unary;

    if ((un.op == TOK_PLUS_PLUS || un.op == TOK_MINUS_MINUS) && !is_lvalue(un.inner))
        checker_error(c, e, "Expression is not assignable");

    check_expr(c, un.inner);
}

static void check_assign(Checker *c, Expr *e) {
    assert(e->kind == EXPR_ASSIGN);
    AssignExpr ass = e->as.assignment;

    Expr *asignee = ass.lhs;
    if (!is_lvalue(asignee)) checker_error(c, e, "Expression is not assignable");

    check_expr(c, ass.lhs);
    check_expr(c, ass.rhs);
}

static void check_conditional(Checker *c, Expr *e) {
    assert(e->kind == EXPR_CONDITIONAL);
    ConditionalExpr cond = e->as.conditional;

    check_expr(c, cond.condition);
    check_expr(c, cond.elseBranch);
    check_expr(c, cond.thenBranch);
}

static void check_func_call(Checker *c, Expr *e) {
    (void)c;
    (void)e;
    TODO("%s", __func__);
}

static void check_index(Checker *c, Expr *e) {
    assert(e->kind == EXPR_INDEX);
    IndexExpr i = e->as.index;

    check_expr(c, i.index);
    check_expr(c, i.array);
}

static void check_expr(Checker *c, Expr *e) {
    switch (e->kind) {
        case EXPR_PRIMARY:     check_primary(c, e);                 return;
        case EXPR_GROUPING:    check_expr(c, e->as.grouping.inner); return;
        case EXPR_BINARY:      check_binary(c, e);                  return;
        case EXPR_UNARY:       check_unary(c, e);                   return;
        case EXPR_ASSIGN:      check_assign(c, e);                  return;
        case EXPR_UNARY_POST:  check_unary(c, e);              return;
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
    assert(s->kind == STMT_BLOCK);
    StmtList block = s->as.block.block;

    enter_scope(c);
    for (size_t i = 0; i < block.len; i++) check_stmt(c, block.arr[i]);
    exit_scope(c);
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

static void check_func(Checker *c, Stmt *s) {
    assert(s->kind == STMT_FUNC);
    (void) c;
    StmtList body = s->as.func.body;
    for (size_t i = 0; i < body.len; i++) {
        check_stmt(c, body.arr[i]);
    }
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
        case STMT_FUNC:     check_func(c, s);               return;
        case STMT_BREAK:    check_break(c, s);              return;
        case STMT_CONTINUE: check_continue(c, s);           return;
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
        check_stmt(&c, unit->ast.arr[i]);
    }

    return !c.hadError;
}
