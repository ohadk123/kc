#include "sema.h"
#include "compiler.h"
#include "type.h"

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

static Type *get_ident_type(Scope *scope, String ident) {
    Stmt *s = find_symbol(scope, ident);
    if (!s) return NULL;
    switch (s->kind) {
        case STMT_VAR:  return s->as.var.type;
        case STMT_FUNC: return s->as.func.funcType;
        default:        UNREACHABLE("Not a symbol kind (%d)", s->kind);
    }
}

static bool check_primitive(Type *t, PrimitiveTypeKind primitive) {
    return t->kind == TYPE_PRIMITIVE && t->as.primitive == primitive;
}

typedef struct {
    TranslationUnit *unit;
    Scope *curr;
} Checker;

static Type *check_expr(Checker *c, Expr *e);

static TokenKind int_lit_to_token_kind(Token integerTok, String fileName) {
    assert(integerTok.kind == TOK_INTEGER_LITERAL);
    uint64_t value = integerTok.as.integerLiteral;
    if (value <= UINT8_MAX) return TOK_U8;
    if (value <= UINT16_MAX) return TOK_U16;
    if (value <= UINT32_MAX) return TOK_U32;
    if (value <= UINT64_MAX) return TOK_U64;
    compile_error(fileName, integerTok.loc, "integer literal is too large");
}

static Type *check_primary(Checker *c, Expr *e) {
    PrimaryExpr prime = e->as.primary;

    switch (prime.value.kind) {
        case TOK_IDENTIFIER: {
            assert(prime.value.kind == TOK_IDENTIFIER);
            Type *type = get_ident_type(c->curr, prime.value.as.identifier);
            if (!type)
                compile_error(c->unit->fileName, e->loc, "unkown symbol '%.*s'", strf(prime.value.as.identifier));
            return type;
        }
        case TOK_STRING_LITERAL: TODO("Evaluate string type");
        case TOK_TRUE:           return type_make_primitive_from_token(TOK_BOOL);
        case TOK_FALSE:          return type_make_primitive_from_token(TOK_BOOL);
        case TOK_CHAR_LITERAL:   return type_make_primitive_from_token(TOK_U8);
        case TOK_INTEGER_LITERAL:
            return type_make_primitive_from_token(int_lit_to_token_kind(prime.value, c->unit->fileName));
        case TOK_FLOAT_LITERAL: return type_make_primitive_from_token(TOK_F64);
        default:                compile_error(c->unit->fileName, e->loc, "Not a type kind (%d)", prime.value.kind);
    }
}

static Type *check_binary(Checker *c, Expr *e) {
    BinaryExpr bin = e->as.binary;

    Type *lhs = check_expr(c, bin.lhs);
    Type *rhs = check_expr(c, bin.rhs);
    Type *type = compare_types(lhs, rhs);
    if (!type) TODO("Type error message");

    switch (bin.op) {
        case TOK_EQUALS_EQUALS:
        case TOK_BANG_EQUALS:
        case TOK_LESS:
        case TOK_LESS_EQUALS:
        case TOK_GREATER:
        case TOK_GREATER_EQUALS:
        case TOK_AMPERSAND_AMPERSAND:
        case TOK_PIPE_PIPE:           type = type_make_primitive_from_token(TOK_BOOL);
        default:                      break;
    }

    return type;
}

static Type *check_unary(Checker *c, Expr *e) {
    UnaryExpr unary = e->as.unary;

    Type *type = check_expr(c, unary.inner);

    if (unary.op == TOK_BANG && type->kind != TYPE_PRIMITIVE && type->as.primitive != TYPE_BOOL)
        compile_error(c->unit->fileName, e->loc, "cannot negate non-bool value");

    if (unary.op == TOK_STAR) {
        if (type->kind != TYPE_POINTER)
            compile_error(c->unit->fileName, e->loc, "cannot dereference non-pointer value");
        type = type->as.pointer;
    } else if (unary.op == TOK_AMPERSAND) {
        type = type_make_pointer(type);
    } else if (unary.op == TOK_MINUS) {
        if (check_primitive(type, TYPE_U8))
            type = type_make_primitive_from_token(TOK_I8);
        else if (check_primitive(type, TYPE_U16))
            type = type_make_primitive_from_token(TOK_I16);
        else if (check_primitive(type, TYPE_U32))
            type = type_make_primitive_from_token(TOK_I32);
        else if (check_primitive(type, TYPE_U64))
            type = type_make_primitive_from_token(TOK_I64);
        else if (check_primitive(type, TYPE_USIZE))
            type = type_make_primitive_from_token(TOK_ISIZE);
    }

    return type;
}

static bool is_lvalue(Expr *e) {
    if (e->kind == EXPR_PRIMARY && e->as.primary.value.kind == TOK_IDENTIFIER) return true;
    if (e->kind == EXPR_GROUPING) return is_lvalue(e->as.grouping.inner);
    if (e->kind == EXPR_UNARY && e->as.unary.op == TOK_STAR) return true;

    return false;
}

static Type *check_assign(Checker *c, Expr *e) {
    AssignExpr ass = e->as.assignment;

    // assignee must be a variable identifier
    Expr *assginee = e->as.assignment.lhs;
    if (!is_lvalue(assginee)) compile_error(c->unit->fileName, e->loc, "expression is not assignable");

    Type *lhs = check_expr(c, ass.lhs);
    Type *rhs = check_expr(c, ass.rhs);
    return compare_types(lhs, rhs);
}

static Type *check_conditional(Checker *c, Expr *e) {
    ConditionalExpr cond = e->as.conditional;

    Type *condType = check_expr(c, cond.condition);
    if (condType->kind != TYPE_PRIMITIVE || condType->as.primitive != TYPE_BOOL)
        compile_error(c->unit->fileName, e->loc, "ternary expression condition is not boolean");

    Type *thenBranch = check_expr(c, cond.thenBranch);
    Type *elseType = check_expr(c, cond.elseBranch);
    return compare_types(thenBranch, elseType);
}

static Type *check_func_call(Checker *c, Expr *e) {
    FuncCallExpr funcCall = e->as.funcCall;

    Type *funcDeclType = check_expr(c, funcCall.func);
    if (funcDeclType->kind != TYPE_FUNC) compile_error(c->unit->fileName, e->loc, "expression is not a function");

    FuncType funcType = funcDeclType->as.func;
    if (funcType.params.len > funcCall.args.len)
        compile_error(c->unit->fileName, e->loc, "too few arguments to function call, expected %zu, got %zu",
                      funcType.params.len, funcCall.args.len);
    else if (funcType.params.len < funcCall.args.len)
        compile_error(c->unit->fileName, e->loc, "too many arguments to function call, expected %zu, got %zu",
                      funcType.params.len, funcCall.args.len);

    for (size_t i = 0; i < funcType.params.len; i++) {
        Type *argType = check_expr(c, funcCall.args.arr[i]);
        Type *paramType = funcType.params.arr[i]->as.var.type;
        if (!compare_types(argType, paramType)) {
            String argTypeStr = type_to_string(argType);
            String paramTypeStr = type_to_string(paramType);
            compile_error(c->unit->fileName, e->loc, "cannot pass argument of type '%.*s' to parameter of type '%.*s'",
                          strf(argTypeStr), strf(paramTypeStr));
        }
    }

    return funcType.retType;
}

static bool is_integer(Type *t) {
    if (t->kind != TYPE_PRIMITIVE) return false;
    PrimitiveTypeKind kind = t->as.primitive;

    switch (kind) {
        case TYPE_VOID:  return false;
        case TYPE_BOOL:  return false;
        case TYPE_U8:    return true;
        case TYPE_U16:   return true;
        case TYPE_U32:   return true;
        case TYPE_U64:   return true;
        case TYPE_USIZE: return true;
        case TYPE_I8:    return true;
        case TYPE_I16:   return true;
        case TYPE_I32:   return true;
        case TYPE_I64:   return true;
        case TYPE_ISIZE: return true;
        case TYPE_F32:   return false;
        case TYPE_F64:   return false;
        default:         return false;
    }
}

static Type *check_index(Checker *c, Expr *e) {
    IndexExpr index = e->as.index;

    Type *arrayType = check_expr(c, index.array);
    if (arrayType->kind != TYPE_ARRAY && arrayType->kind != TYPE_POINTER)
        compile_error(c->unit->fileName, e->loc, "cannot index value which is not array nor pointer");

    Type *indexType = check_expr(c, index.index);
    if (!is_integer(indexType)) compile_error(c->unit->fileName, e->loc, "array index is not an integer");

    return arrayType->kind == TYPE_ARRAY ? arrayType->as.array.elementType : arrayType->as.pointer;
}

static Type *check_expr(Checker *c, Expr *e) {
    Type *type = 0;

    switch (e->kind) {
        case EXPR_PRIMARY:     type = check_primary(c, e); break;
        case EXPR_GROUPING:    type = check_expr(c, e->as.grouping.inner); break;
        case EXPR_BINARY:      type = check_binary(c, e); break;
        case EXPR_UNARY:       type = check_unary(c, e); break;
        case EXPR_ASSIGN:      type = check_assign(c, e); break;
        case EXPR_UNARY_POST:  type = check_expr(c, e->as.unary.inner); break;
        case EXPR_CONDITIONAL: type = check_conditional(c, e); break;
        case EXPR_FUNC_CALL:   type = check_func_call(c, e); break;
        case EXPR_INDEX:       type = check_index(c, e);
    }

    e->type = type;
    return type;
}

static void check_stmt(Checker *c, Stmt *s);

static void check_var(Checker *c, Stmt *s) {
    Stmt *symbol = find_symbol(c->curr, s->as.var.name.as.identifier);
    if (symbol) {
        compile_error(c->unit->fileName, s->loc, "symbol '%.*s' already delcared before at [%.*s:%zu:%zu]",
                      strf(s->as.var.name.as.identifier), strf(c->unit->fileName), s->loc.line, s->loc.col);
    }

    hm_insert(&c->curr->symbols, s->as.var.name.as.identifier, s);

    if (check_primitive(s->as.var.type, TYPE_VOID))
        compile_error(c->unit->fileName, s->loc, "variable cannot be of type 'void'");

    if (s->as.var.init) {
        Type *initType = check_expr(c, s->as.var.init);
        if (!compare_types(initType, s->as.var.type)) TODO("Type error message for variable initializer");
    }
}

static void check_block(Checker *c, Stmt *s) {
    Scope next = (Scope){
        .above = c->curr,
        .symbols = {0},
    };

    c->curr = &next;

    for (size_t i = 0; i < s->as.block.block.len; i++) check_stmt(c, s->as.block.block.arr[i]);

    c->curr = c->curr->above;
}

static void check_while(Checker *c, Stmt *s) {
    if (!check_primitive(check_expr(c, s->as.whileS.condition), TYPE_BOOL))
        compile_error(c->unit->fileName, s->loc, "while condition is not a boolean expression");
    c->curr->inLoop = true;
    check_stmt(c, s->as.whileS.body);
    c->curr->inLoop = false;
}

static void check_if(Checker *c, Stmt *s) {
    if (!check_primitive(check_expr(c, s->as.ifS.condition), TYPE_BOOL)) {
        compile_error(c->unit->fileName, s->loc, "if condition is not a boolean expression");
    }
    check_stmt(c, s->as.ifS.thenBranch);
    if (s->as.ifS.elseBranch) check_stmt(c, s->as.ifS.elseBranch);
}

static void check_for(Checker *c, Stmt *s) {
    Scope forScope = (Scope){
        .above = c->curr,
        .inLoop = true,
    };
    c->curr = &forScope;

    if (s->as.forS.initializer) check_stmt(c, s->as.forS.initializer);
    if (s->as.forS.condition) {
        Type *condType = check_expr(c, s->as.forS.condition);
        if (!check_primitive(condType, TYPE_BOOL))
            compile_error(c->unit->fileName, s->loc, "for loop condition is not a boolean expression");
    }
    if (s->as.forS.increment) check_expr(c, s->as.forS.increment);
    check_stmt(c, s->as.forS.body);

    c->curr = c->curr->above;
}

static void check_return(Checker *c, Stmt *s) {
    Scope *retScope = c->curr;
    while (retScope && !retScope->retType) retScope = retScope->above;
    if (!retScope) compile_error(c->unit->fileName, s->loc, "return statement not in a function");

    if (s->as.returnS.retVal) {
        Type *retValType = check_expr(c, s->as.returnS.retVal);
        if (!compare_types(retValType, retScope->retType)) {
            String retValTypeStr = type_to_string(retValType);
            String retScopeTypeStr = type_to_string(retScope->retType);
            compile_error(c->unit->fileName, s->loc, "cannot return '%.*s' from function returning '%.*s'",
                          strf(retValTypeStr), strf(retScopeTypeStr));
        }
    } else if (check_primitive(retScope->retType, TYPE_VOID))
        compile_error(c->unit->fileName, s->loc, "missing return type");
}

static void check_func(Checker *c, Stmt *s) {
    FuncStmt func = s->as.func;
    Scope funcScope = {
        .symbols = {0},
        .above = c->curr,
        .retType = func.funcType->as.func.retType,
    };
    c->curr = &funcScope;
    StmtList params = func.funcType->as.func.params;
    for (size_t i = 0; i < params.len; i++) check_var(c, params.arr[i]);
    for (size_t i = 0; i < func.block.len; i++) check_stmt(c, func.block.arr[i]);
    c->curr = c->curr->above;
}

static void check_break(Checker *c, Stmt *s) {
    Scope *loopScope = c->curr;
    while (loopScope && !loopScope->inLoop) loopScope = loopScope->above;
    if (!loopScope) compile_error(c->unit->fileName, s->loc, "'break' statement not in loop");
}

static void check_continue(Checker *c, Stmt *s) {
    Scope *loopScope = c->curr;
    while (loopScope && !loopScope->inLoop) loopScope = loopScope->above;
    if (!loopScope) compile_error(c->unit->fileName, s->loc, "'continue' statement not in loop");
}

static void check_stmt(Checker *c, Stmt *s) {
    switch (s->kind) {
        case STMT_NULL:                                     break;
        case STMT_VAR:      check_var(c, s);                break;
        case STMT_EXPR:     check_expr(c, s->as.expr.expr); break;
        case STMT_BLOCK:    check_block(c, s);              break;
        case STMT_WHILE:    check_while(c, s);              break;
        case STMT_DO_WHILE: check_while(c, s);              break;
        case STMT_IF:       check_if(c, s);                 break;
        case STMT_FOR:      check_for(c, s);                break;
        case STMT_RETURN:   check_return(c, s);             break;
        case STMT_FUNC:     check_func(c, s);               break;
        case STMT_BREAK:    check_break(c, s);              break;
        case STMT_CONTINUE: check_continue(c, s);           break;
    }
}

void semantic_analysis(TranslationUnit *unit) {
    Checker c = {0};
    c.curr = &unit->globalSymbols;
    c.unit = unit;

    fill_global_symbol_table(unit);

    for (size_t i = 0; i < unit->ast.len; i++) {
        check_stmt(&c, unit->ast.arr[i]);
    }
}
