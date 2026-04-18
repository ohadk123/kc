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
            compile_error(unit->fileName, s->loc, "symbol \"%.*s\" already delcared before at [%.*s:%zu:%zu]",
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

static TokenKind get_type(Scope *scope, String ident) {
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
        case TOK_F64:   return true;
        default:        return false;
    }
}

static bool is_compatible(TokenKind a, TokenKind b) {
    if (a == b) return true;
    if (is_primitive(a) && is_primitive(b)) return true;
    return false;
}

typedef struct {
    TranslationUnit *unit;
    Scope *curr;
} Analyzer;

static TokenKind check_expr(Analyzer *a, Expr *e) {
    TokenKind type = 0, rhs;
    switch (e->kind) {
        case EXPR_PRIMARY:
            switch (e->as.primary.value.kind) {
                case TOK_IDENTIFIER:
                    type = get_type(a->curr, e->as.primary.value.as.identifier);
                    if (!type)
                        compile_error(a->unit->fileName, e->loc, "unkown symbol %.*s",
                                      strf(e->as.primary.value.as.identifier));
                    break;
                case TOK_STRING_LITERAL:  TODO("Evaluate string type");
                case TOK_TRUE:            type = TOK_BOOL; break;
                case TOK_FALSE:           type = TOK_BOOL; break;
                case TOK_CHAR_LITERAL:    type = TOK_CHAR_LITERAL; break;
                case TOK_INTEGER_LITERAL: type = TOK_INTEGER_LITERAL; break;
                case TOK_FLOAT_LITERAL:   type = TOK_FLOAT_LITERAL; break;
                default:                  compile_error(a->unit->fileName, e->loc, "Not a type kind (%d)", e->as.primary.value.kind);
            }
            break;
        case EXPR_GROUPING: type = check_expr(a, e->as.grouping.inner); break;
        case EXPR_BINARY:
            type = check_expr(a, e->as.binary.lhs);
            rhs = check_expr(a, e->as.binary.rhs);
            if (!is_compatible(type, rhs))
                compile_error(a->unit->fileName, e->loc, "mismatch between types %s and %s", tokenTypesStrings[type],
                              tokenTypesStrings[rhs]);

            switch (e->as.binary.op) {
                case TOK_EQUALS_EQUALS:
                case TOK_BANG_EQUALS:
                case TOK_LESS:
                case TOK_LESS_EQUALS:
                case TOK_GREATER:
                case TOK_GREATER_EQUALS:
                case TOK_AMPERSAND_AMPERSAND:
                case TOK_PIPE_PIPE:           type = TOK_BOOL; break;
                default:                      break;
            }
            break;
        case EXPR_UNARY:
            type = check_expr(a, e->as.unary.inner);
            switch (e->as.unary.op) {
                case TOK_PLUS_PLUS:   break;
                case TOK_MINUS_MINUS: break;
                case TOK_AMPERSAND:   TODO("Pointers");
                case TOK_STAR:        TODO("Pointers");
                case TOK_PLUS:        break;
                case TOK_MINUS:       break;
                case TOK_TILDE:       break;
                case TOK_BANG:        type = TOK_BOOL; break;
                default:              UNREACHABLE("not a unary op (%d)", e->as.unary.op);
            }
            break;

        case EXPR_ASSIGN:
            type = check_expr(a, e->as.assignment.lhs);
            rhs = check_expr(a, e->as.assignment.rhs);
            if (!is_compatible(type, rhs))
                compile_error(a->unit->fileName, e->loc, "cannot assign to %s from type %s", tokenTypesStrings[type],
                              tokenTypesStrings[rhs]);
            break;
        case EXPR_UNARY_POST: type = check_expr(a, e->as.unary.inner); break;
        case EXPR_CONDITIONAL:
            if (check_expr(a, e->as.conditional.condition) != TOK_BOOL)
                compile_error(a->unit->fileName, e->loc, "ternary expression condition is not boolean");

            type = check_expr(a, e->as.conditional.thenBranch);
            TokenKind elseType = check_expr(a, e->as.conditional.elseBranch);
            if (!is_compatible(type, elseType))
                compile_error(a->unit->fileName, e->loc, "mismatch between branches types %s and %s",
                              tokenTypesStrings[type], tokenTypesStrings[elseType]);
            break;
        case EXPR_FUNC_CALL:
            type = check_expr(a, e->as.funcCall.func);
            // TODO: function pointers
            assert(e->as.funcCall.func->kind == EXPR_PRIMARY);
            Stmt *func = find_symbol(a->curr, e->as.funcCall.func->as.primary.value.as.identifier);
            assert(func->kind == STMT_FUNC);
            FuncStmt fs = func->as.func;
            FuncCallExpr fce = e->as.funcCall;

            if (fs.params.len > fce.args.len)
                compile_error(a->unit->fileName, e->loc, "too few arguments to function call, expected %zu, got %zu",
                              fs.params.len, fce.args.len);
            else if (fs.params.len < fce.args.len)
                compile_error(a->unit->fileName, e->loc, "too many arguments to function call, expected %zu, got %zu",
                              fs.params.len, fce.args.len);

            for (size_t i = 0; i < fs.params.len; i++) {
                TokenKind argType = check_expr(a, fce.args.arr[i]);
                TokenKind paramType = fs.params.arr[i]->as.var.type;

                if (!is_compatible(argType, paramType))
                    compile_error(a->unit->fileName, fce.args.arr[i]->loc,
                                  "incompatiable types, passing %s as parameter of type %s", tokenTypesStrings[argType],
                                  tokenTypesStrings[paramType]);
            }
            break;
    }

    e->type = type;
    return type;
}

static void check_stmt(Analyzer *a, Stmt *s);

static void check_var(Analyzer *a, Stmt *s) {
    Stmt *symbol = find_symbol(a->curr, s->as.var.name.as.identifier);
    if (symbol) {
        compile_error(a->unit->fileName, s->loc, "symbol \"%.*s\" already delcared before at [%.*s:%zu:%zu]",
                      strf(s->as.var.name.as.identifier), a->unit->fileName, s->loc.line, s->loc.col);
    }

    if (s->as.var.init) {
        TokenKind initType = check_expr(a, s->as.var.init);
        TokenKind varType = s->as.var.type;

        if (!is_compatible(initType, varType))
            compile_error(a->unit->fileName, s->loc, "cannot assign type %s to variable of type %s",
                          tokenTypesStrings[initType], tokenTypesStrings[varType]);
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
    if (check_expr(a, s->as.whileS.cond) != TOK_BOOL)
        compile_error(a->unit->fileName, s->loc, "while condition is not a boolean expression");
    a->curr->inLoop = true;
    check_stmt(a, s->as.whileS.body);
    a->curr->inLoop = false;
}

static void check_if(Analyzer *a, Stmt *s) {
    if (check_expr(a, s->as.ifS.cond) != TOK_BOOL)
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
        if (!is_compatible(retValType, retScope->retType))
            compile_error(a->unit->fileName, s->loc, "cannot return %s from function returning %s",
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
    if (!loopScope) compile_error(a->unit->fileName, s->loc, "\"break\" statement not in loop");
}

static void check_continue(Analyzer *a, Stmt *s) {
    Scope *loopScope = a->curr;
    while (loopScope && !loopScope->inLoop) loopScope = loopScope->above;
    if (!loopScope) compile_error(a->unit->fileName, s->loc, "\"continue\" statement not in loop");
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
