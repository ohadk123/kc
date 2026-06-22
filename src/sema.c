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

static bool is_lvalue(Expr *e) {
    if (e->kind == EXPR_PRIMARY && e->as.primary.value.kind == TOK_IDENTIFIER && e->as.primary.decl) {
        switch (e->as.primary.decl->kind) {
            case STMT_VAR:      return true;
            case STMT_NULL:
            case STMT_EXPR:
            case STMT_BLOCK:
            case STMT_WHILE:
            case STMT_DO_WHILE:
            case STMT_IF:
            case STMT_FOR:
            case STMT_RETURN:
            case STMT_FUNC:
            case STMT_BREAK:
            case STMT_CONTINUE: return false;
        }
    }
    if (e->kind == EXPR_GROUPING) return is_lvalue(e->as.grouping.inner); // inner expression is lvalue? (x)++
    if (e->kind == EXPR_UNARY && e->as.unary.op == TOK_STAR) return true; // pointer dereference

    return false;
}

/******************************************************************************
 * Evaluating Expressions
 *****************************************************************************/

static bool eval_expr(Expr *e, int64_t *out);

static bool eval_primary(Expr *e, int64_t *out) {
    assert(e->kind == EXPR_PRIMARY);
    Token prim = e->as.primary.value;

    switch (prim.kind) {
        case TOK_IDENTIFIER:      return false;
        case TOK_STRING_LITERAL:  TODO("%s: TOK_STRING_LITERAL", __func__);
        case TOK_CHAR_LITERAL:    *out = prim.as.charLiteral; return true;
        case TOK_INTEGER_LITERAL: *out = prim.as.integerLiteral; return true;
        case TOK_FLOAT_LITERAL:   *out = (int64_t) prim.as.floatLiteral; return true;
        case TOK_TRUE:            *out = 1; return true;
        case TOK_FALSE:           *out = 0; return true;
        default:                  UNREACHABLE("%s: Unsupported token kind: %s", __func__, tokenTypesStrings[prim.kind]);
    }
}

static bool eval_binary(Expr *e, int64_t *out) {
    assert (e->kind == EXPR_BINARY);
    BinaryExpr binExpr = e->as.binary;

    int64_t lhs, rhs;
    if (!eval_expr(binExpr.lhs, &lhs)) return false;
    if (!eval_expr(binExpr.rhs, &rhs)) return false;

    if ((binExpr.op == TOK_SLASH || binExpr.op == TOK_PERCENT) && rhs == 0) return false;

    switch (binExpr.op) {
        case TOK_PLUS:                *out = lhs +  rhs; break;
        case TOK_MINUS:               *out = lhs -  rhs; break;
        case TOK_STAR:                *out = lhs *  rhs; break;
        case TOK_SLASH:               *out = lhs /  rhs; break;
        case TOK_PERCENT:             *out = lhs %  rhs; break;
        case TOK_CARET:               *out = lhs ^  rhs; break;
        case TOK_LESS_LESS:           *out = lhs << rhs; break;
        case TOK_GREATER_GREATER:     *out = lhs >> rhs; break;
        case TOK_AMPERSAND:           *out = lhs &  rhs; break;
        case TOK_PIPE:                *out = lhs |  rhs; break;
        case TOK_EQUALS_EQUALS:       *out = lhs == rhs; break;
        case TOK_BANG_EQUALS:         *out = lhs != rhs; break;
        case TOK_GREATER:             *out = lhs >  rhs; break;
        case TOK_GREATER_EQUALS:      *out = lhs >= rhs; break;
        case TOK_LESS:                *out = lhs <  rhs; break;
        case TOK_LESS_EQUALS:         *out = lhs <= rhs; break;
        case TOK_AMPERSAND_AMPERSAND: *out = lhs && rhs; break;
        case TOK_PIPE_PIPE:           *out = lhs || rhs; break;
        default:                      UNREACHABLE("Not an binary operator (%s)", tokenTypesStrings[binExpr.op]);
    }

    return true;
}

static bool eval_unary(Expr *e, int64_t *out) {
    assert(e->kind == EXPR_UNARY);
    UnaryExpr unExpr = e->as.unary;

    int64_t inner;
    if (!eval_expr(unExpr.inner, &inner)) return false;

    switch (unExpr.op) {
        case TOK_PLUS:        *out =  inner; break;
        case TOK_MINUS:       *out = -inner; break;
        case TOK_TILDE:       *out = ~inner; break;
        case TOK_BANG:        *out = !inner; break;

        case TOK_PLUS_PLUS:
        case TOK_MINUS_MINUS:
        case TOK_STAR:
        case TOK_AMPERSAND: return false;

        default: UNREACHABLE("%s: Not an unary operator (%s)", __func__, tokenTypesStrings[unExpr.op]);
    }

    return true;
}

static bool eval_cond(Expr *e, int64_t *out) {
    assert(e->kind == EXPR_CONDITIONAL);
    ConditionalExpr c = e->as.conditional;

    int64_t condVal, thenVal, elseVal;
    if (!eval_expr(c.condition, &condVal)) return false;
    if (!eval_expr(c.thenBranch, &thenVal)) return false;
    if (!eval_expr(c.elseBranch, &elseVal)) return false;

    *out = condVal ? thenVal : elseVal;
    return true;
}

static bool eval_expr(Expr *e, int64_t *out) {
    switch (e->kind) {
        case EXPR_PRIMARY:     return eval_primary(e, out);
        case EXPR_GROUPING:    return eval_expr(e->as.grouping.inner, out);
        case EXPR_BINARY:      return eval_binary(e, out);
        case EXPR_UNARY:       return eval_unary(e, out);
        case EXPR_ASSIGN:      return false;
        case EXPR_UNARY_POST:  return false;
        case EXPR_CONDITIONAL: return eval_cond(e, out);
        case EXPR_FUNC_CALL:   return false;
        case EXPR_INDEX:       return false;
    }
}

typedef struct {
    TranslationUnit *unit;
    Scope *curr;
    bool hadError;

    bool inFunc;
    int loopCount;
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

static Stmt *expect_var(Checker *c, Token *nameTok) {
    assert(nameTok->kind == TOK_IDENTIFIER);
    Stmt *found = find_symbol(c->curr, nameTok->as.identifier);
    if (!found) checker_error(c, nameTok, "Unkown symbol '%.*s'", strf(nameTok->as.identifier));
    if (found->kind != STMT_VAR)
        checker_error(c, nameTok, "Cannot use symbol '%.*s' as a variable", nameTok->as.identifier);
    return found;
}

static void declare_var(Checker *c, Stmt *varStmt) {
    assert(varStmt->kind == STMT_VAR);

    String varName = varStmt->as.var.name.as.identifier;
    Stmt *found = hm_find_val(&c->curr->symbols, varName);
    if (found)
        checker_error(c, varStmt, "Symbol '%.*s' already delcared before at [%.*s:%zu:%zu]", strf(varName),
                      strf(c->unit->fileName), varStmt->loc.line, varStmt->loc.col);

    hm_insert(&c->curr->symbols, varName, varStmt);
}

static Stmt *expect_func(Checker *c, Token *nameTok) {
    assert(nameTok->kind == TOK_IDENTIFIER);

    String funcName = nameTok->as.identifier;
    Stmt *found = hm_find_val(&c->unit->globalSymbols.symbols, funcName);
    if (!found) checker_error(c, nameTok, "Call to undeclared function '%.*s'", strf(funcName));
    return found;
}

/******************************************************************************
 * Expression Checking
 *****************************************************************************/

static void check_expr(Checker *c, Expr *e);

static void check_primary(Checker *c, Expr *e) {
    assert(e->kind == EXPR_PRIMARY);
    Token prim = e->as.primary.value;

    switch (prim.kind) {
        case TOK_IDENTIFIER:      e->as.primary.decl = expect_var(c, &prim); return;
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

    check_expr(c, e->as.binary.lhs);
    check_expr(c, e->as.binary.rhs);
}

static void check_unary(Checker *c, Expr *e) {
    assert(e->kind == EXPR_UNARY || e->kind == EXPR_UNARY_POST);

    UnaryExpr un = e->as.unary;
    check_expr(c, un.inner);

    if ((un.op == TOK_PLUS_PLUS || un.op == TOK_MINUS_MINUS) && !is_lvalue(un.inner))
        checker_error(c, e, "Expression is not assignable");
}

static void check_assign(Checker *c, Expr *e) {
    assert(e->kind == EXPR_ASSIGN);

    AssignExpr assign = e->as.assignment;
    check_expr(c, assign.lhs);
    check_expr(c, assign.rhs);

    if (!is_lvalue(assign.lhs)) checker_error(c, e, "Expression is not assignable");
}

static void check_conditional(Checker *c, Expr *e) {
    assert(e->kind == EXPR_CONDITIONAL);

    check_expr(c, e->as.conditional.condition);
    check_expr(c, e->as.conditional.elseBranch);
    check_expr(c, e->as.conditional.thenBranch);
}

static void check_func_call(Checker *c, Expr *e) {
    assert(e->kind == EXPR_FUNC_CALL);
    FuncCallExpr funcCallExpr = e->as.funcCall;

    assert(funcCallExpr.func->kind == EXPR_PRIMARY);
    Stmt *funcStmt = expect_func(c, &funcCallExpr.func->as.primary.value);
    if (!funcStmt) return;

    size_t paramCount = funcStmt->as.func.params.len;
    size_t argCount = funcCallExpr.args.len;
    if (argCount > paramCount)
        checker_error(c, e, "Too many arguments to function call expected %zu, got %zu", paramCount, argCount);
    else if (argCount < paramCount)
        checker_error(c, e, "Too few arguments to function call expected %zu, got %zu", paramCount, argCount);

    funcCallExpr.func->as.primary.decl = funcStmt;

    for (size_t i = 0; i < argCount; i++) {
        check_expr(c, funcCallExpr.args.arr[i]);
    }
}

static void check_index(Checker *c, Expr *e) {
    assert(e->kind == EXPR_INDEX);

    check_expr(c, e->as.index.index);
    check_expr(c, e->as.index.array);
}

static void check_expr(Checker *c, Expr *e) {
    switch (e->kind) {
        case EXPR_PRIMARY:     check_primary(c, e);                 return;
        case EXPR_GROUPING:    check_expr(c, e->as.grouping.inner); return;
        case EXPR_BINARY:      check_binary(c, e);                  return;
        case EXPR_UNARY:       check_unary(c, e);                   return;
        case EXPR_ASSIGN:      check_assign(c, e);                  return;
        case EXPR_UNARY_POST:  check_unary(c, e);                   return;
        case EXPR_CONDITIONAL: check_conditional(c, e);             return;
        case EXPR_FUNC_CALL:   check_func_call(c, e);               return;
        case EXPR_INDEX:       check_index(c, e);                   return;
    }
}

/******************************************************************************
 * Statement Checking
 *****************************************************************************/

static void check_stmt(Checker *c, Stmt *s);

static void check_var(Checker *c, Stmt *s) {
    assert(s->kind == STMT_VAR);
    VarStmt varStmt = s->as.var;
    bool isExtern = varStmt.isExtern;
    if (isExtern) TODO("Extern variables");
    bool isPub = varStmt.isPub;
    if (isPub) TODO("Pub variables");
    bool isStatic = varStmt.isStatic;
    if (isStatic) TODO("Static variables");

    check_expr(c, s->as.var.init);
    declare_var(c, s);
}

static void check_block(Checker *c, Stmt *s) {
    assert(s->kind == STMT_BLOCK);
    StmtList block = s->as.block.block;

    enter_scope(c);
    for (size_t i = 0; i < block.len; i++) check_stmt(c, block.arr[i]);
    exit_scope(c);
}

static void check_while(Checker *c, Stmt *s) {
    assert(s->kind == STMT_WHILE || s->kind == STMT_DO_WHILE);
    WhileStmt whileStmt = s->as.whileS;

    check_expr(c, whileStmt.condition);

    enter_scope(c);
    c->loopCount++;

    check_stmt(c, whileStmt.body);

    exit_scope(c);
    c->loopCount--;
}

static void check_if(Checker *c, Stmt *s) {
	assert(s->kind == STMT_IF);
	IfStmt ifS = s->as.ifS;

	check_expr(c, ifS.condition);
	
	enter_scope(c);
	check_stmt(c, ifS.thenBranch);
	exit_scope(c);

	if (ifS.elseBranch) {
		enter_scope(c);
		check_stmt(c, ifS.elseBranch);
		exit_scope(c);
	}
}

static void check_for(Checker *c, Stmt *s) {
    assert(s->kind == STMT_FOR);
    ForStmt forStmt = s->as.forS;

    enter_scope(c);
    c->loopCount++;

    if (forStmt.initializer) check_stmt(c, forStmt.initializer);
    if (forStmt.condition) check_expr(c, forStmt.condition);
    if (forStmt.increment) check_expr(c, forStmt.increment);
    check_stmt(c, forStmt.body);

    exit_scope(c);
    c->loopCount--;
}

static void check_return(Checker *c, Stmt *s) {
    if (!c->inFunc) checker_error(c, s, "'return' statement not in a function");
    if (s->as.returnS.retVal) check_expr(c, s->as.returnS.retVal);
}

static void check_func(Checker *c, Stmt *s) {
    assert(s->kind == STMT_FUNC);
    FuncStmt funcStmt = s->as.func;


    enter_scope(c);
    c->inFunc = true;

    StmtList params = funcStmt.params;
    for (size_t i = 0; i < params.len; i++) declare_var(c, params.arr[i]);

    if (!funcStmt.isExtern) {
        StmtList body = funcStmt.body;
        for (size_t i = 0; i < body.len; i++) check_stmt(c, body.arr[i]);
    }

    exit_scope(c);
    c->inFunc = false;
}

static void check_break(Checker *c, Stmt *s) {
    assert(s->kind == STMT_BREAK);
    if (c->loopCount < 1) checker_error(c, s, "'break' statement not in a loop");
}

static void check_continue(Checker *c, Stmt *s) {
    assert(s->kind == STMT_CONTINUE);
    if (c->loopCount < 1) checker_error(c, s, "'continue' statement not in a loop");
}

static void check_stmt(Checker *c, Stmt *s) {
    switch (s->kind) {
        case STMT_NULL:                                     return;
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
        .inFunc = false,
        .loopCount = 0,
    };

    c.hadError = fill_global_symbol_table(unit);

    for (size_t i = 0; i < unit->ast.len; i++) {
        check_stmt(&c, unit->ast.arr[i]);
    }

    return !c.hadError;
}
