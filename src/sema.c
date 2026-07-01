#include "sema.h"
#include "compiler.h"

static bool fill_global_symbol_table(TranslationUnit *unit) {
    bool hadError = false;

    for (size_t i = 0; i < unit->ast.len; i++) {
        Stmt *s = unit->ast.arr[i];
        String name = get_top_level_name(s);
        String key = name;

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
    if (e->kind == EXPR_PRIMARY && e->as.primary.value.kind == PRIM_IDENTIFIER && e->as.primary.decl) {
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
            case STMT_SWITCH:
            case STMT_BREAK:
            case STMT_CONTINUE: return false;
        }
    }
    if (e->kind == EXPR_GROUPING) return is_lvalue(e->as.grouping.inner); // inner expression is lvalue? (x)++
    if (e->kind == EXPR_UNARY && e->as.unary.op == UN_STAR) return true; // pointer dereference

    return false;
}

/******************************************************************************
 * Evaluating Expressions
 *****************************************************************************/

static bool eval_expr(Expr *e, int64_t *out);

static bool eval_primary(Expr *e, int64_t *out) {
    assert(e->kind == EXPR_PRIMARY);

    switch (e->as.primary.value.kind) {
        case PRIM_IDENTIFIER:      return false;
        case PRIM_STRING_LITERAL:  TODO("%s: TOK_STRING_LITERAL", __func__);
        case PRIM_CHAR_LITERAL:    *out = e->as.primary.value.as.charLiteral; return true;
        case PRIM_INTEGER_LITERAL:
        case PRIM_LONG_LITERAL:    *out = e->as.primary.value.as.longLiteral; return true;
        case PRIM_FLOAT_LITERAL:
        case PRIM_DOUBLE_LITERAL:  *out = (int64_t) e->as.primary.value.as.floatLiteral; return true;
        case PRIM_TRUE:            *out = 1; return true;
        case PRIM_FALSE:           *out = 0; return true;
    }

    UNREACHABLE("Invalid Primary Kind (%s)", tokenTypesStrings[e->as.primary.value.kind]);
}

static bool eval_binary(Expr *e, int64_t *out) {
    assert (e->kind == EXPR_BINARY);

    int64_t lhs, rhs;
    if (!eval_expr(e->as.binary.lhs, &lhs)) return false;
    if (!eval_expr(e->as.binary.rhs, &rhs)) return false;

    if ((e->as.binary.op == BIN_SLASH || e->as.binary.op == BIN_PERCENT) && rhs == 0) return false;

    switch (e->as.binary.op) {
        case BIN_PLUS:                *out = lhs +  rhs; return true;
        case BIN_MINUS:               *out = lhs -  rhs; return true;
        case BIN_STAR:                *out = lhs *  rhs; return true;
        case BIN_SLASH:               *out = lhs /  rhs; return true;
        case BIN_PERCENT:             *out = lhs %  rhs; return true;
        case BIN_CARET:               *out = lhs ^  rhs; return true;
        case BIN_LESS_LESS:           *out = lhs << rhs; return true;
        case BIN_GREATER_GREATER:     *out = lhs >> rhs; return true;
        case BIN_AMPERSAND:           *out = lhs &  rhs; return true;
        case BIN_PIPE:                *out = lhs |  rhs; return true;
        case BIN_EQUALS_EQUALS:       *out = lhs == rhs; return true;
        case BIN_BANG_EQUALS:         *out = lhs != rhs; return true;
        case BIN_GREATER:             *out = lhs >  rhs; return true;
        case BIN_GREATER_EQUALS:      *out = lhs >= rhs; return true;
        case BIN_LESS:                *out = lhs <  rhs; return true;
        case BIN_LESS_EQUALS:         *out = lhs <= rhs; return true;
        case BIN_AMPERSAND_AMPERSAND: *out = lhs && rhs; return true;
        case BIN_PIPE_PIPE:           *out = lhs || rhs; return true;
    }
    UNREACHABLE("Invalid Binary Operator (%s)", tokenTypesStrings[e->as.binary.op]);
}

static bool eval_unary(Expr *e, int64_t *out) {
    assert(e->kind == EXPR_UNARY);

    int64_t inner;
    if (!eval_expr(e->as.unary.inner, &inner)) return false;

    switch (e->as.unary.op) {
        case TOK_PLUS:        *out =  inner; break;
        case TOK_MINUS:       *out = -inner; break;
        case TOK_TILDE:       *out = ~inner; break;
        case TOK_BANG:        *out = !inner; break;

        case TOK_PLUS_PLUS:
        case TOK_MINUS_MINUS:
        case TOK_STAR:
        case TOK_AMPERSAND: return false;

        default: UNREACHABLE("Invalid Unary Operator (%s)", tokenTypesStrings[e->as.unary.op]);
    }

    return true;
}

static bool eval_cond(Expr *e, int64_t *out) {
    assert(e->kind == EXPR_CONDITIONAL);

    int64_t condVal, thenVal, elseVal;
    if (!eval_expr(e->as.conditional.condition, &condVal)) return false;
    if (!eval_expr(e->as.conditional.thenBranch, &thenVal)) return false;
    if (!eval_expr(e->as.conditional.elseBranch, &elseVal)) return false;

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
        case EXPR_CAST:        return eval_expr(e->as.cast.inner, out);
    }

    UNREACHABLE("Not a valid expression kind (%d)", e->kind);
}

/******************************************************************************
 * Sematic Analysis Checker
 *****************************************************************************/

typedef struct {
    TranslationUnit *unit;
    Scope *curr;
    bool hadError;

    Type *ret;
    int loopCount;
} Checker;

#define checker_error(c, l, fmt, ...) \
    c->hadError = compile_err_no_abort((c)->unit->fileName, l, fmt, ##__VA_ARGS__)

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

static Stmt *expect_var(Checker *c, String name, Location loc) {
    Stmt *found = find_symbol(c->curr, name);
    if (!found) checker_error(c, loc, "Unkown symbol '%.*s'", strf(name));
    else if (found->kind != STMT_VAR) {
        checker_error(c, loc, "Cannot use symbol '%.*s' as a variable", strf(name));
        return NULL;
    }
    return found;
}

static void declare_var(Checker *c, Stmt *varStmt) {
    assert(varStmt->kind == STMT_VAR);

    String name = varStmt->as.var.name;
    Stmt *found = hm_find_val(&c->curr->symbols, name);
    if (found)
        checker_error(c, varStmt->loc, "Symbol '%.*s' already delcared before at [%.*s:%zu:%zu]", strf(name),
                      strf(c->unit->fileName), varStmt->loc.line, varStmt->loc.col);

    hm_insert(&c->curr->symbols, name, varStmt);
}

static Stmt *expect_func(Checker *c, String name, Location loc) {
    Stmt *found = hm_find_val(&c->unit->globalSymbols.symbols, name);
    if (!found) checker_error(c, loc, "Call to undeclared function '%.*s'", strf(name));
    else if (found->kind != STMT_FUNC) checker_error(c, loc, "Symbol cannot be called");
    return found;
}

static bool fits_in_type(int64_t v, const Type *t) {
      if (t->size >= 8) return true;                 // i64 holds any int64_t
      int64_t bits = (int64_t)t->size * 8;
      int64_t min  = -(INT64_C(1) << (bits - 1));
      int64_t max  =  (INT64_C(1) << (bits - 1)) - 1;
      return v >= min && v <= max;
  }

static void convert_expr_type(Checker *c, Expr **slot, Type *target, Location loc) {
    Type *src = (*slot)->type;
    if (type_equal(src, target)) return; // same type - no problem

    if (target->size >= src->size) { // widening - make it explicit
        *slot = expr_make_cast(target, *slot, loc);
        (*slot)->type = target;
        return;
    }

    int64_t v;
    if (eval_expr(*slot, &v) && fits_in_type(v, target)) {
        *slot = expr_make_cast(target, *slot, loc);
        return;
    }

    // narrowing - force explicit cast
    checker_error(c, loc, "Implicit narrowing from %s to %s is not allowed, use cast<%s>(x)", type_name(src),
                  type_name(target), type_name(target));
}

/******************************************************************************
 * Expression Checking
 *****************************************************************************/

static Type *check_expr(Checker *c, Expr *e);

static Type *check_primary(Checker *c, Expr *e) {
    assert(e->kind == EXPR_PRIMARY);

    switch (e->as.primary.value.kind) {
        case PRIM_IDENTIFIER: {
            Stmt *decl = expect_var(c, e->as.primary.value.as.identifier, e->loc);
            e->as.primary.decl = decl;
            return decl ? decl->as.var.type : type_i32;
        }
        case PRIM_STRING_LITERAL:  TODO("%s: TOK_STRING_LITERAL", __func__);
        case PRIM_TRUE:
        case PRIM_FALSE:
        case PRIM_CHAR_LITERAL:
        case PRIM_INTEGER_LITERAL: return type_i32;
        case PRIM_LONG_LITERAL:    return type_i64;
        case PRIM_FLOAT_LITERAL:   TODO("%s: PRIM_FLOAT_LITERAL", __func__);
        case PRIM_DOUBLE_LITERAL:  TODO("%s: PRIM_DOUBLE_LITERAL", __func__);
    }

    UNREACHABLE("Invalid PrimaryKind (%s)", tokenTypesStrings[e->as.primary.value.kind]);
}

static Type *check_binary(Checker *c, Expr *e) {
    assert(e->kind == EXPR_BINARY);

    check_expr(c, e->as.binary.lhs);
    check_expr(c, e->as.binary.rhs);

    switch (e->as.binary.op) {
        // arithemtic - return common type
        case BIN_PLUS:
        case BIN_MINUS:
        case BIN_STAR:
        case BIN_SLASH:
        case BIN_PERCENT:
        case BIN_CARET:
        case BIN_PIPE:
        case BIN_AMPERSAND: {
            Type *target = type_common(e->as.binary.lhs->type, e->as.binary.rhs->type);
            convert_expr_type(c, &e->as.binary.lhs, target, e->as.binary.lhs->loc);
            convert_expr_type(c, &e->as.binary.rhs, target, e->as.binary.rhs->loc);
            return target;
        }

        // shifts - return lhs type
        case BIN_LESS_LESS:
        case BIN_GREATER_GREATER: {
            int64_t v;
            if (eval_expr(e->as.binary.rhs, &v) && v >= e->as.binary.lhs->type->size * 8)
                checker_error(c, e->loc, "Shift amount %ld out of range for type %s (%zu)", v,
                              type_name(e->as.binary.lhs->type), e->as.binary.lhs->type->size * 8);
            return e->as.binary.lhs->type;
        }

        // comparison - return bool
        case BIN_EQUALS_EQUALS:
        case BIN_BANG_EQUALS:
        case BIN_GREATER:
        case BIN_GREATER_EQUALS:
        case BIN_LESS:
        case BIN_LESS_EQUALS:
        case BIN_AMPERSAND_AMPERSAND:
        case BIN_PIPE_PIPE: {
            Type *target = type_common(e->as.binary.lhs->type, e->as.binary.rhs->type);
            convert_expr_type(c, &e->as.binary.lhs, target, e->as.binary.lhs->loc);
            convert_expr_type(c, &e->as.binary.rhs, target, e->as.binary.rhs->loc);
            return type_i32;
        }
    }

    UNREACHABLE("Invalid Binary Operator (%s)", tokenTypesStrings[e->as.binary.op]);
}

static Type *check_unary(Checker *c, Expr *e) {
    assert(e->kind == EXPR_UNARY || e->kind == EXPR_UNARY_POST);

    check_expr(c, e->as.unary.inner);

    if ((e->as.unary.op == UN_PLUS_PLUS || e->as.unary.op == UN_MINUS_MINUS) && !is_lvalue(e->as.unary.inner))
        checker_error(c, e->loc, "Expression is not assignable");

    return e->as.unary.op == UN_BANG ? type_i32 : e->as.unary.inner->type;
}

static Type *check_assign(Checker *c, Expr *e) {
    assert(e->kind == EXPR_ASSIGN);

    check_expr(c, e->as.assignment.lhs);
    check_expr(c, e->as.assignment.rhs);

    if (!is_lvalue(e->as.assignment.lhs)) checker_error(c, e->loc, "Expression is not assignable");

    convert_expr_type(c, &e->as.assignment.rhs, e->as.assignment.lhs->type, e->loc);
    return e->as.assignment.lhs->type;
}

static Type *check_conditional(Checker *c, Expr *e) {
    assert(e->kind == EXPR_CONDITIONAL);

    check_expr(c, e->as.conditional.condition);
    Type *thenType = check_expr(c, e->as.conditional.thenBranch);
    Type *elseType = check_expr(c, e->as.conditional.elseBranch);

    Type *common = (type_common(thenType, elseType));
    convert_expr_type(c, &e->as.conditional.thenBranch, common, e->loc);
    convert_expr_type(c, &e->as.conditional.elseBranch, common, e->loc);
    return common;
}

static Type *check_func_call(Checker *c, Expr *e) {
    assert(e->kind == EXPR_FUNC_CALL);

    assert(e->as.funcCall.func->kind == EXPR_PRIMARY);
    Stmt *funcStmt = expect_func(c, e->as.funcCall.func->as.primary.value.as.identifier, e->loc);
    if (!funcStmt) return type_i32;

    size_t paramCount = funcStmt->as.func.params.len;
    size_t argsCount = e->as.funcCall.args.len;
    if (argsCount != paramCount) {
        if (argsCount > paramCount)
            checker_error(c, e->loc, "Too many arguments to function call expected %zu, got %zu", paramCount, argsCount);
        else if (argsCount < paramCount)
            checker_error(c, e->loc, "Too few arguments to function call expected %zu, got %zu", paramCount, argsCount);
        return type_i32;
    }

    e->as.funcCall.func->as.primary.decl = funcStmt;

    for (size_t i = 0; i < argsCount; i++) {
        Expr **arg = &e->as.funcCall.args.arr[i];
        check_expr(c, *arg);
        convert_expr_type(c, arg, funcStmt->as.func.params.arr[i]->as.var.type, (*arg)->loc);
    }

    return funcStmt->as.func.ret;
}

static Type *check_index(Checker *c, Expr *e) {
    (void)c;
    (void)e;

    TODO("%s", __func__);
}

static Type *check_cast(Checker *c, Expr *e) {
    assert(e->kind == EXPR_CAST);

    check_expr(c, e->as.cast.inner);
    return e->as.cast.target;
}

static Type *check_expr(Checker *c, Expr *e) {
    Type *t = NULL;
    switch (e->kind) {
        case EXPR_PRIMARY:     t = check_primary(c, e);                 break;
        case EXPR_GROUPING:    t = check_expr(c, e->as.grouping.inner); break;
        case EXPR_BINARY:      t = check_binary(c, e);                  break;
        case EXPR_UNARY:       t = check_unary(c, e);                   break;
        case EXPR_ASSIGN:      t = check_assign(c, e);                  break;
        case EXPR_UNARY_POST:  t = check_unary(c, e);                   break;
        case EXPR_CONDITIONAL: t = check_conditional(c, e);             break;
        case EXPR_FUNC_CALL:   t = check_func_call(c, e);               break;
        case EXPR_INDEX:       t = check_index(c, e);                   break;
        case EXPR_CAST:        t = check_cast(c, e);                    break;
    }
    if (!t) UNREACHABLE("Invalid Expression Kind (%d)", e->kind);

    return e->type = t;
}

/******************************************************************************
 * Statement Checking
 *****************************************************************************/

static void check_stmt(Checker *c, Stmt *s);

static void check_var(Checker *c, Stmt *s) {
    assert(s->kind == STMT_VAR);

    if (s->as.var.specifier == SPEC_EXTERN) return;

    bool isGlobal = c->curr == &c->unit->globalSymbols;
    bool isStatic = s->as.var.specifier == SPEC_STATIC;

    if (s->as.var.init) {
        if (!eval_expr(s->as.var.init, &s->as.var.initVal) && (isGlobal || isStatic))
            checker_error(c, s->loc, "Initalizer not a compile time constant");
        check_expr(c, s->as.var.init);
        convert_expr_type(c, &s->as.var.init, s->as.var.type, s->loc);
    }

    if (!isGlobal) declare_var(c, s);
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
    if (!c->ret) checker_error(c, s->loc, "'return' statement not in a function");
    if (s->as.returnS.retVal) {
        check_expr(c, s->as.returnS.retVal);
        convert_expr_type(c, &s->as.returnS.retVal, c->ret, s->as.returnS.retVal->loc);
    }
}

static void check_func(Checker *c, Stmt *s) {
    assert(s->kind == STMT_FUNC);

    enter_scope(c);
    c->ret = s->as.func.ret;

    StmtList params = s->as.func.params;
    for (size_t i = 0; i < params.len; i++) declare_var(c, params.arr[i]);

    if (s->as.func.specifier != SPEC_EXTERN) {
        StmtList body = s->as.func.body;
        for (size_t i = 0; i < body.len; i++) check_stmt(c, body.arr[i]);
    }

    exit_scope(c);
    c->ret = NULL;
}

static void check_break(Checker *c, Stmt *s) {
    assert(s->kind == STMT_BREAK);
    if (c->loopCount < 1) checker_error(c, s->loc, "'break' statement not in a loop");
}

static void check_continue(Checker *c, Stmt *s) {
    assert(s->kind == STMT_CONTINUE);
    if (c->loopCount < 1) checker_error(c, s->loc, "'continue' statement not in a loop");
}

static void check_switch(Checker *c, Stmt *s) {
    assert(s->kind == STMT_SWITCH);
    (void) c;
    TODO("%s", __func__);
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
        case STMT_SWITCH:   check_switch(c, s);             return;
        case STMT_BREAK:    check_break(c, s);              return;
        case STMT_CONTINUE: check_continue(c, s);           return;
    }
}

bool semantic_analysis(TranslationUnit *unit) {
    Checker c = {
        .unit = unit,
        .curr = &unit->globalSymbols,
        .hadError = false,
        .ret = NULL,
        .loopCount = 0,
    };

    c.hadError = fill_global_symbol_table(unit);

    for (size_t i = 0; i < unit->ast.len; i++) {
        check_stmt(&c, unit->ast.arr[i]);
    }

    return !c.hadError;
}
