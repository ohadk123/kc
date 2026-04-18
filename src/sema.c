#include "sema.h"
#include "compiler.h"

void fill_global_symbol_table(TranslationUnit *unit) {
    for (size_t i = 0; i < unit->ast.len; i++) {
        Stmt *s = unit->ast.arr[i];
        Token nameTok = get_top_level_name(s);
        assert(nameTok.kind == TOK_IDENTIFIER);
        String key = nameTok.as.identifier;

        if (!hm_insert(&unit->globalSymbols.symbols, key, s)) {
            Stmt *first = hm_find_val(&unit->globalSymbols.symbols, key);
            compile_error(unit->fileName, s->loc, "symbol '%.*s' already delcared before at [%.*s:%zu:%zu]",
                          (int)key.len, key.data, (int)unit->fileName.len, unit->fileName.data, first->loc.line,
                          first->loc.col);
        }
    }
}

static Stmt *find_symbol(Scope *scope, String symbol) {
    while (scope) {
        Stmt *var = hm_find_val(&scope->symbols, symbol);
        if (var) return var;
        scope = scope->above;
    }

    return 0;
}

static TokenKind get_ident_type(Scope *scope, String ident) {
    Stmt *s = find_symbol(scope, ident);
    if (!s) return 0;
    switch (s->kind) {
        case STMT_VAR:  return s->as.var.type;
        case STMT_FUNC: return s->as.func.retType;
        default:        UNREACHABLE("Not a symbol kind (%d)", s->kind);
    }
}

static bool is_primitive(TokenKind kind) {
    switch (kind) {
        case TOK_INTEGER_LITERAL:
        case TOK_FLOAT_LITERAL:
        case TOK_BOOL:
        case TOK_U8:
        case TOK_U16:
        case TOK_U32:
        case TOK_U64:
        case TOK_USIZE:
        case TOK_I8:
        case TOK_I16:
        case TOK_I32:
        case TOK_I64:
        case TOK_ISIZE:
        case TOK_F32:
        case TOK_F64:             return true;
        default:                  return false;
    }
}

static int type_priorities[] = {
    [TOK_INTEGER_LITERAL] = 0,
    [TOK_FLOAT_LITERAL] = 1,
    [TOK_BOOL] = 2,
    [TOK_I8] = 3,
    [TOK_U8] = 4,
    [TOK_I16] = 5,
    [TOK_U16] = 6,
    [TOK_I32] = 7,
    [TOK_U32] = 8,
    [TOK_ISIZE] = 9,
    [TOK_USIZE] = 10,
    [TOK_I64] = 11,
    [TOK_U64] = 12,
    [TOK_F32] = 13,
    [TOK_F64] = 14,
};

static TokenKind choose_primitive(TokenKind a, TokenKind b) {
    assert(is_primitive(a) && is_primitive(b) && "Both args must be primitives");
    int pa = type_priorities[a];
    int pb = type_priorities[b];

    if (pa > pb) return a;
    return b;
}

typedef struct {
    TranslationUnit *unit;
    Scope *curr;
} Analyzer;

static Stmt *expect_symbol(Analyzer *a, Token ident) {
    assert(ident.kind == TOK_IDENTIFIER);
    String symbolName = ident.as.identifier;
    Stmt *s = find_symbol(a->curr, symbolName);
    if (s) return s;

    compile_error(a->unit->fileName, ident.loc, "identifier '%.*s' is undeclared", strf(symbolName));
}

static TokenKind check_expr(Analyzer *a, Expr *e);

static TokenKind check_primary(Analyzer *a, Expr *e) {
    PrimaryExpr prime = e->as.primary;

    switch (prime.value.kind) {
        case TOK_IDENTIFIER: {
            assert(prime.value.kind == TOK_IDENTIFIER);
            TokenKind type = get_ident_type(a->curr, prime.value.as.identifier);
            if (!type)
                compile_error(a->unit->fileName, e->loc, "unkown symbol '%.*s'", strf(prime.value.as.identifier));
            return type;
        }
        case TOK_STRING_LITERAL:  TODO("Evaluate string type");
        case TOK_TRUE:            return TOK_BOOL;
        case TOK_FALSE:           return TOK_BOOL;
        case TOK_CHAR_LITERAL:    return TOK_CHAR_LITERAL;
        case TOK_INTEGER_LITERAL: return TOK_INTEGER_LITERAL;
        case TOK_FLOAT_LITERAL:   return TOK_FLOAT_LITERAL;
        default:                  compile_error(a->unit->fileName, e->loc, "Not a type kind (%d)", prime.value.kind);
    }
}

static TokenKind check_binary(Analyzer *a, Expr *e) {
    BinaryExpr bin = e->as.binary;

    TokenKind lhs = check_expr(a, bin.lhs);
    TokenKind rhs = check_expr(a, bin.rhs);
    TokenKind type = choose_primitive(lhs, rhs);

    switch (bin.op) {
        case TOK_EQUALS_EQUALS:
        case TOK_BANG_EQUALS:
        case TOK_LESS:
        case TOK_LESS_EQUALS:
        case TOK_GREATER:
        case TOK_GREATER_EQUALS:
        case TOK_AMPERSAND_AMPERSAND:
        case TOK_PIPE_PIPE:
            type = TOK_BOOL;
        default: break;
    }

    return type;
}

static TokenKind check_unary(Analyzer *a, Expr *e) {
    UnaryExpr unary = e->as.unary;

    TokenKind type = check_expr(a, unary.inner);
    if (unary.op == TOK_BANG && type != TOK_BOOL) compile_error(a->unit->fileName, e->loc, "cannot negate non-bool value");
    if (unary.op == TOK_STAR || unary.op == TOK_AMPERSAND) TODO("Pointers");
    return type;
}

static TokenKind check_assign(Analyzer *a, Expr *e) {
    AssignExpr ass = e->as.assignment;

    // assignee must be a variable identifier
    Expr *assginee = e->as.assignment.lhs;
    if (assginee->kind != EXPR_PRIMARY || assginee->as.primary.value.kind != TOK_IDENTIFIER
         || expect_symbol(a, assginee->as.primary.value)->kind != STMT_VAR)
        compile_error(a->unit->fileName, e->loc, "expression is not assignable");

    TokenKind lhs = check_expr(a, ass.lhs);
    TokenKind rhs = check_expr(a, ass.rhs);
    return choose_primitive(lhs, rhs); // TODO: type checking
}

static TokenKind check_conditional(Analyzer *a, Expr *e) {
    ConditionalExpr cond = e->as.conditional;

    if (check_expr(a, cond.condition) != TOK_BOOL)
        compile_error(a->unit->fileName, e->loc, "ternary expression condition is not boolean");

    TokenKind thenBranch = check_expr(a, cond.thenBranch);
    TokenKind elseType   = check_expr(a, cond.elseBranch);
    return choose_primitive(thenBranch, elseType); // TODO: type checking
}

static TokenKind check_func_call(Analyzer *a, Expr *e) {
    FuncCallExpr funcCall = e->as.funcCall;

    // TODO: function pointers
    assert(funcCall.func->kind == EXPR_PRIMARY);
    Stmt *funcStmt = expect_symbol(a, funcCall.func->as.primary.value);
    if (funcStmt->kind != STMT_FUNC)
        compile_error(a->unit->fileName, funcStmt->loc, "object is not callable");

    FuncStmt funcDecl = funcStmt->as.func;
    if (funcDecl.params.len > funcCall.args.len)
        compile_error(a->unit->fileName, e->loc, "too few arguments to function call, expected %zu, got %zu",
                      funcDecl.params.len, funcCall.args.len);
    else if (funcDecl.params.len < funcCall.args.len)
        compile_error(a->unit->fileName, e->loc, "too many arguments to function call, expected %zu, got %zu",
                      funcDecl.params.len, funcCall.args.len);

    for (size_t i = 0; i < funcDecl.params.len; i++) {
        TokenKind argType = check_expr(a, funcCall.args.arr[i]);
        TokenKind paramType = funcDecl.params.arr[i]->as.var.type;
        if (type_priorities[argType] > type_priorities[paramType])
            compile_error(a->unit->fileName, e->loc, "cannot pass param of type '%s' with arg of type '%s'",
                    tokenTypesStrings[paramType], tokenTypesStrings[argType]);
    }

    return funcDecl.retType;
}

static TokenKind check_expr(Analyzer *a, Expr *e) {
    TokenKind type = 0;

    switch (e->kind) {
        case EXPR_PRIMARY:     type = check_primary(a, e);                 break;
        case EXPR_GROUPING:    type = check_expr(a, e->as.grouping.inner); break;
        case EXPR_BINARY:      type = check_binary(a, e);                  break;
        case EXPR_UNARY:       type = check_unary(a, e);                   break;
        case EXPR_ASSIGN:      type = check_assign(a, e);                  break;
        case EXPR_UNARY_POST:  type = check_expr(a, e->as.unary.inner);    break;
        case EXPR_CONDITIONAL: type = check_conditional(a, e);             break;
        case EXPR_FUNC_CALL:   type = check_func_call(a, e);               break;
    }

    e->type = type;
    return type;
}

static void check_stmt(Analyzer *a, Stmt *s);

static void check_var(Analyzer *a, Stmt *s) {
    Stmt *symbol = find_symbol(a->curr, s->as.var.name.as.identifier);
    if (symbol) {
        compile_error(a->unit->fileName, s->loc, "symbol '%.*s' already delcared before at [%.*s:%zu:%zu]",
                      strf(s->as.var.name.as.identifier), a->unit->fileName, s->loc.line, s->loc.col);
    }

    if (s->as.var.init) {
        TokenKind initType = check_expr(a, s->as.var.init);
        if (type_priorities[initType] > type_priorities[s->as.var.type])
            compile_error(a->unit->fileName, s->loc, "cannot initialize variable of type '%s' with value of type '%s'",
                          tokenTypesStrings[s->as.var.type], tokenTypesStrings[initType]);
    }

    hm_insert(&a->curr->symbols, s->as.var.name.as.identifier, s);
}

static void check_block(Analyzer *a, Stmt *s) {
    Scope next = (Scope){
        .above = a->curr,
        .symbols = {0},
    };

    a->curr = &next;

    for (size_t i = 0; i < s->as.block.block.len; i++) check_stmt(a, s->as.block.block.arr[i]);

    a->curr = a->curr->above;
}

static void check_while(Analyzer *a, Stmt *s) {
    if (check_expr(a, s->as.whileS.condition) != TOK_BOOL)
        compile_error(a->unit->fileName, s->loc, "while condition is not a boolean expression");
    a->curr->inLoop = true;
    check_stmt(a, s->as.whileS.body);
    a->curr->inLoop = false;
}

static void check_if(Analyzer *a, Stmt *s) {
    if (check_expr(a, s->as.ifS.condition) != TOK_BOOL)
        compile_error(a->unit->fileName, s->loc, "if condition is not a boolean expression");
    check_stmt(a, s->as.ifS.thenBranch);
    if (s->as.ifS.elseBranch) check_stmt(a, s->as.ifS.elseBranch);
}

static void check_for(Analyzer *a, Stmt *s) {
    Scope forScope = (Scope){
        .above = a->curr,
        .inLoop = true,
    };
    a->curr = &forScope;

    if (s->as.forS.initializer) check_stmt(a, s->as.forS.initializer);
    if (s->as.forS.condition) check_expr(a, s->as.forS.condition);
    if (s->as.forS.increment) check_expr(a, s->as.forS.increment);
    check_stmt(a, s->as.forS.body);

    a->curr = a->curr->above;
}

static void check_return(Analyzer *a, Stmt *s) {
    Scope *retScope = a->curr;
    while (retScope && !retScope->retType) retScope = retScope->above;
    if (!retScope) compile_error(a->unit->fileName, s->loc, "return statement not in a function");

    if (s->as.returnS.retVal) {
        TokenKind retValType = check_expr(a, s->as.returnS.retVal);
        if (type_priorities[retValType] > type_priorities[retScope->retType])
            compile_error(a->unit->fileName, s->loc, "cannot return '%s' from function returning '%s'",
                          tokenTypesStrings[retValType], tokenTypesStrings[retScope->retType]);
    } else if (retScope->retType != TOK_VOID)
        compile_error(a->unit->fileName, s->loc, "missing return type");
}

static void check_func(Analyzer *a, Stmt *s) {
    FuncStmt func = s->as.func;
    Scope funcScope = {
        .symbols = {0},
        .above = a->curr,
        .retType = func.retType,
    };
    a->curr = &funcScope;
    for (size_t i = 0; i < func.params.len; i++) check_var(a, func.params.arr[i]);
    for (size_t i = 0; i < func.block.len; i++) check_stmt(a, func.block.arr[i]);
    a->curr = a->curr->above;
}

static void check_break(Analyzer *a, Stmt *s) {
    Scope *loopScope = a->curr;
    while (loopScope && !loopScope->inLoop) loopScope = loopScope->above;
    if (!loopScope) compile_error(a->unit->fileName, s->loc, "'break' statement not in loop");
}

static void check_continue(Analyzer *a, Stmt *s) {
    Scope *loopScope = a->curr;
    while (loopScope && !loopScope->inLoop) loopScope = loopScope->above;
    if (!loopScope) compile_error(a->unit->fileName, s->loc, "'continue' statement not in loop");
}

static void check_stmt(Analyzer *a, Stmt *s) {
    switch (s->kind) {
        case STMT_VAR:      check_var(a, s); break;
        case STMT_EXPR:     check_expr(a, s->as.expr.expr); break;
        case STMT_BLOCK:    check_block(a, s); break;
        case STMT_WHILE:    check_while(a, s); break;
        case STMT_IF:       check_if(a, s); break;
        case STMT_FOR:      check_for(a, s); break;
        case STMT_RETURN:   check_return(a, s); break;
        case STMT_FUNC:     check_func(a, s); break;
        case STMT_BREAK:    check_break(a, s); break;
        case STMT_CONTINUE: check_continue(a, s); break;
    }
}

void semantic_analysis(TranslationUnit *unit) {
    Analyzer a = {0};
    a.curr = &unit->globalSymbols;
    a.unit = unit;

    fill_global_symbol_table(unit);

    for (size_t i = 0; i < unit->ast.len; i++) {
        check_stmt(&a, unit->ast.arr[i]);
    }
}
