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

static String qbe_data(Stmt *s, bool mangle) {
    String name = get_top_level_name(s);
    uint64_t i = counter();
    return mangle ? str_printf("$_%s_%zu_%zu_%zu", name, i, s->loc.line, s->loc.col)
                  : str_printf("$%.*s", strf(name));
}

static String qbe_label(const char *name, Location l) {
    uint64_t i = counter();
    return str_printf("@_%s_%zu_%zu_%zu", name, i, l.line, l.col);
}

static char qbe_type(Type *type) {
    switch (type->kind) {
        case TYPE_ERR: UNREACHABLE("Error TypeKind");
        case TYPE_I32: return 'w';
        case TYPE_I64: return 'l';
    }
    UNREACHABLE("Invalid TypeKind (%d)", type->kind);
}

typedef struct {
    String breakLbl;
    String continueLbl;
} LoopLabels;

typedef struct {
    LIST_FIELDS(LoopLabels);
} LabelStack;

typedef struct {
    FILE *outf;
    TranslationUnit *unit;
    LabelStack labels;
    StmtList data;

    bool inFunc;
} Generator;

static inline void accumulate_data(Generator *g, Stmt *s, bool mangle) {
    list_append(&g->data, s);
    s->as.var.qbe_var = qbe_data(s, mangle);
}

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
    fprintf(g->outf, "\n%*s", g->inFunc ? 4 : 0, " ");
    return ret;
}

/******************************************************************************
 * Loop Labels Stack
 *****************************************************************************/

static int gprint_lbl(Generator *g, String lbl) {
    return gprintfln(g, "%.*s", strf(lbl));
}

static void push_labels(Generator *g, String breakLbl, String contLbl) {
    LoopLabels l = (LoopLabels) {breakLbl, contLbl};
    list_append(&g->labels, l);
}

static void pop_labels(Generator *g) {
    g->labels.len--;
}

static String get_break_lbl(Generator *g) {
    return g->labels.arr[g->labels.len - 1].breakLbl;
}

static String get_cont_lbl(Generator *g) {
    return g->labels.arr[g->labels.len - 1].continueLbl;
}

/******************************************************************************
 * Expression CodeGen
 *****************************************************************************/

static String gen_expr(Generator *g, Expr *e);

static String get_lvalue(Generator *g, Expr *e) {
    switch (e->kind) {
        case EXPR_PRIMARY:  return e->as.primary.decl->as.var.qbe_var;
        case EXPR_GROUPING: return get_lvalue(g, e->as.grouping.inner);
        case EXPR_UNARY:    return gen_expr(g, e->as.unary.inner);
        default: UNREACHABLE("%s: Not an expression that can be lvalue", __func__);
    }
}

static String gen_primary(Generator *g, Expr *e) {
    assert(e->kind == EXPR_PRIMARY);
    PrimaryValue val = e->as.primary.value;

    switch (val.kind) {
        case PRIM_IDENTIFIER: {
            String out = qbe_var(e->loc);
            String qvar = get_lvalue(g, e);
            char qbeType = qbe_type(e->type);
            gprintfln(g, "%.*s =%c load%c %.*s", strf(out), qbeType, qbeType, strf(qvar));
            return out;
        }
        case PRIM_STRING_LITERAL:  TODO("%s: PRIM_STRING_LITERAL", __func__);
        case PRIM_CHAR_LITERAL:    return str_printf("%u", val.as.charLiteral);
        case PRIM_INTEGER_LITERAL: return str_printf("%u", val.as.integerLiteral);
        case PRIM_LONG_LITERAL:    return str_printf("%llu", val.as.longLiteral);
        case PRIM_FLOAT_LITERAL:   TODO("%s: PRIM_FLOAT_LITERAL", __func__);
        case PRIM_DOUBLE_LITERAL:  TODO("%s: PRIM_DOUBLE_LITERAL", __func__);
        case PRIM_TRUE:            return str_from_cstr("1");
        case PRIM_FALSE:           return str_from_cstr("0");
    }

    UNREACHABLE("Invalid TokenKind (%s)", tokenTypesStrings[val.kind]);
}
static const char *get_bin_op(BinOp op) {
    switch (op) {
        case BIN_PLUS:            return "add";
        case BIN_MINUS:           return "sub";
        case BIN_STAR:            return "mul";
        case BIN_SLASH:           return "div";
        case BIN_PERCENT:         return "rem";
        case BIN_CARET:           return "xor";
        case BIN_LESS_LESS:       return "shl";
        case BIN_GREATER_GREATER: return "sar";
        case BIN_AMPERSAND:       return "and";
        case BIN_PIPE:            return "or";

        // Comparison operators need qbe type suffix
        case BIN_EQUALS_EQUALS:   return "ceq";
        case BIN_BANG_EQUALS:     return "cne";
        case BIN_GREATER:         return "csgt";
        case BIN_GREATER_EQUALS:  return "csge";
        case BIN_LESS:            return "cslt";
        case BIN_LESS_EQUALS:     return "csle";
        case BIN_AMPERSAND_AMPERSAND: break;
        case BIN_PIPE_PIPE:           break;
    }

    UNREACHABLE("Invalid Binary Operator (%s)", tokenTypesStrings[op]);
}

static String gen_binary(Generator *g, Expr *e) {
    assert(e->kind == EXPR_BINARY);
    BinaryExpr bin = e->as.binary;

    String out = qbe_var(e->loc);
    switch (bin.op) {
        case BIN_PLUS:
        case BIN_MINUS:
        case BIN_STAR:
        case BIN_SLASH:
        case BIN_PERCENT:
        case BIN_CARET:
        case BIN_LESS_LESS:
        case BIN_GREATER_GREATER:
        case BIN_AMPERSAND:
        case BIN_PIPE: {
            String lhs = gen_expr(g, bin.lhs);
            String rhs = gen_expr(g, bin.rhs);
            gprintfln(g, "%.*s =%c %s %.*s, %.*s", strf(out), qbe_type(e->type), get_bin_op(bin.op),
                      strf(lhs), strf(rhs));
            break;
        }

        case BIN_EQUALS_EQUALS:
        case BIN_BANG_EQUALS:
        case BIN_GREATER:
        case BIN_GREATER_EQUALS:
        case BIN_LESS:
        case BIN_LESS_EQUALS: {
            String lhs = gen_expr(g, bin.lhs);
            String rhs = gen_expr(g, bin.rhs);
            Type *common = type_common(bin.lhs->type, bin.rhs->type);
            gprintfln(g, "%.*s =%c %s%c %.*s, %.*s", strf(out), qbe_type(e->type), get_bin_op(bin.op),
                      qbe_type(common), strf(lhs), strf(rhs));
            break;
        }

        case BIN_AMPERSAND_AMPERSAND:
        case BIN_PIPE_PIPE: {
            String one = qbe_label("one", e->loc);
            String zero = qbe_label("zero", e->loc);
            String rhsL = qbe_label("rhs", e->loc);
            String end = qbe_label("end", e->loc);
            bool isAnd = bin.op == BIN_AMPERSAND_AMPERSAND;

            String lhs = gen_expr(g, bin.lhs);
            String tempLhs = qbe_var(e->loc);
            gprintfln(g, "%.*s =w cne%c %.*s, 0", strf(tempLhs), qbe_type(bin.lhs->type), strf(lhs));
            gprintfln(g, "jnz %.*s, %.*s, %.*s", strf(tempLhs), strf(isAnd ? rhsL : one), strf(isAnd ? zero : rhsL));

            gprint_lbl(g, rhsL);
            String rhs = gen_expr(g, bin.rhs);
            String tempRhs = qbe_var(e->loc);
            gprintfln(g, "%.*s =w cne%c %.*s, 0", strf(tempRhs), qbe_type(bin.rhs->type), strf(rhs));
            gprintfln(g, "jnz %.*s, %.*s, %.*s", strf(tempRhs), strf(one), strf(zero));

            gprint_lbl(g, zero);
            gprintfln(g, "jmp %.*s", strf(end));

            gprint_lbl(g, one);
            gprintfln(g, "jmp %.*s", strf(end));

            gprint_lbl(g, end);
            gprintfln(g, "%.*s =%c phi %.*s 0, %.*s 1", strf(out), qbe_type(e->type), strf(zero), strf(one));
            break;
        }

        default: TODO("%s: Binary operator \"%s\"", __func__, tokenTypesStrings[bin.op]);
    }

    return out;
}

static String gen_unary(Generator *g, Expr *e) {
    assert(e->kind == EXPR_UNARY);
    UnaryExpr un = e->as.unary;

    String out = qbe_var(e->loc);
    switch (un.op) {
        case TOK_MINUS: {
            String inner = gen_expr(g, un.inner);
            gprintfln(g, "%.*s =%c sub 0, %.*s", strf(out), qbe_type(e->type), strf(inner));
            break;
        }

        case TOK_TILDE: {
            String inner = gen_expr(g, un.inner);
            gprintfln(g, "%.*s =%c xor %.*s, -1", strf(out), qbe_type(e->type), strf(inner));
            break;
        }

        case TOK_BANG: {
            String inner = gen_expr(g, un.inner);
            gprintfln(g, "%.*s =%c ceq%c %.*s, 0", strf(out), qbe_type(e->type), qbe_type(e->as.unary.inner->type),
                      strf(inner));
            break;
        }

        case TOK_PLUS_PLUS:
        case TOK_MINUS_MINUS: {
            String inner = get_lvalue(g, un.inner);
            String loadTemp = qbe_var(e->loc);
            const char *op = un.op == UN_PLUS_PLUS ? "add" : "sub";

            char qbeType = qbe_type(e->type);
            gprintfln(g, "%.*s =%c load%c %.*s", strf(loadTemp), qbeType, qbeType, strf(inner));
            gprintfln(g, "%.*s =%c %s %.*s, 1", strf(out), qbeType, op, strf(loadTemp));
            gprintfln(g, "store%c %.*s, %.*s", qbeType, strf(out), strf(inner));
            break;
        }

        default: TODO("Invalid Unary operator (%s)", tokenTypesStrings[un.op]);
    }

    return out;
}

static const char *get_assign_op(AssignOp op) {
   switch (op) {
        case ASS_PLUS_EQUALS:            return "add";
        case ASS_MINUS_EQUALS:           return "sub";
        case ASS_STAR_EQUALS:            return "mul";
        case ASS_SLASH_EQUALS:           return "div";
        case ASS_PERCENT_EQUALS:         return "rem";
        case ASS_AMPERSAND_EQUALS:       return "and";
        case ASS_PIPE_EQUALS:            return "or";
        case ASS_CARET_EQUALS:           return "xor";
        case ASS_LESS_LESS_EQUALS:       return "shl";
        case ASS_GREATER_GREATER_EQUALS: return "sar";
        case ASS_EQUALS:                 break;
   }

    UNREACHABLE("Invalid assignment operator (%s)", tokenTypesStrings[op]);
}


static String gen_assign(Generator *g, Expr *e) {
    assert(e->kind == EXPR_ASSIGN);
    AssignExpr assign = e->as.assignment;

    String lhs = get_lvalue(g, assign.lhs);
    String rhs = gen_expr(g, assign.rhs);
    String out = qbe_var(e->loc);

    char qbeType = qbe_type(e->type);
    if (assign.op == ASS_EQUALS) {
        gprintfln(g, "%.*s =%c copy %.*s", strf(out), qbeType, strf(rhs));
    } else {
        String old = qbe_var(e->loc);
        gprintfln(g, "%.*s =%c load%c %.*s", strf(old), qbeType, qbeType, strf(lhs));
        gprintfln(g, "%.*s =%c %s %.*s, %.*s", strf(out), qbeType, get_assign_op(assign.op), strf(old), strf(rhs));
    }

    gprintfln(g, "store%c %.*s, %.*s", qbeType, strf(out), strf(lhs));
    return out;
}

static String gen_unary_post(Generator *g, Expr *e) {
    assert(e->kind == EXPR_UNARY_POST);
    UnaryExpr un = e->as.unary;

    String out = qbe_var(e->loc);
    String temp = qbe_var(e->loc);
    String inner = get_lvalue(g, un.inner);
    const char *op = un.op == UN_PLUS_PLUS ? "add" : "sub";

    char qbeType = qbe_type(e->type);
    gprintfln(g, "%.*s =%c load%c %.*s", strf(out), qbeType, qbeType, strf(inner));
    gprintfln(g, "%.*s =%c %s %.*s, 1", strf(temp), qbeType, op, strf(out));
    gprintfln(g, "store%c %.*s, %.*s", qbeType, strf(temp), strf(inner));

    return out;
}


static String gen_conditional(Generator *g, Expr *e) {
    assert(e->kind == EXPR_CONDITIONAL);
    ConditionalExpr c = e->as.conditional;

    String out = qbe_var(e->loc);
    String cond = gen_expr(g, c.condition);
    String iftLbl = qbe_label("ift", e->loc);
    String iftEndLbl = qbe_label("iftEnd", e->loc);
    String iffLbl = qbe_label("iff", e->loc);
    String iffEndLbl = qbe_label("iftEnd", e->loc);
    String end = qbe_label("end", e->loc);

    gprintfln(g, "jnz %.*s, %.*s, %.*s", strf(cond), strf(iftLbl), strf(iffLbl));

    gprint_lbl(g, iftLbl);
    String iftVal = gen_expr(g, c.thenBranch);
    gprint_lbl(g, iftEndLbl);
    gprintfln(g, "jmp %.*s", strf(end));

    gprint_lbl(g, iffLbl);
    String iffVal = gen_expr(g, c.elseBranch);
    gprint_lbl(g, iffEndLbl);
    gprintfln(g, "jmp %.*s", strf(end));

    gprint_lbl(g, end);
    gprintfln(g, "%.*s =%c phi %.*s %.*s, %.*s %.*s", strf(out), qbe_type(e->type), strf(iftEndLbl), strf(iftVal), strf(iffEndLbl),
              strf(iffVal));

    return out;
}

static String gen_func_call(Generator *g, Expr *e) {
    assert(e->kind == EXPR_FUNC_CALL);
    FuncCallExpr funcCallExpr = e->as.funcCall;

    ExprList args = funcCallExpr.args;
    struct { LIST_FIELDS(String); } argsNames = {0};
    for (size_t i = 0; i < args.len; i++) {
        String argName = gen_expr(g, args.arr[i]);
        list_append(&argsNames, argName);
    }

    String out = qbe_var(e->loc);
    gprintf(g, "%.*s =%c call $%.*s(", strf(out), qbe_type(e->type), strf(funcCallExpr.func->as.primary.decl->as.func.name));
    for (size_t i = 0; i < argsNames.len; i++) {
        gprintf(g, "%c %.*s, ", qbe_type(args.arr[i]->type), strf(argsNames.arr[i]));
    }
    gprintfln(g, ")");

    return out;
}

static String gen_index(Generator *g, Expr *e) {
    (void)g;
    (void)e;
    TODO("%s", __func__);
}

static String gen_cast(Generator *g, Expr *e) {
    assert(e->kind == EXPR_CAST);

    String inner = gen_expr(g, e->as.cast.inner);
    String out = qbe_var(e->loc);
    if (e->type->size > e->as.cast.inner->type->size) {
        gprintfln(g, "%.*s =%c exts%c %.*s", strf(out), qbe_type(e->type), qbe_type(e->as.cast.inner->type),
                  strf(inner));
    } else {
        gprintfln(g, "%.*s =%c copy %.*s", strf(out), qbe_type(e->type), strf(inner));
    }

    return out;
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
        case EXPR_CAST:        return gen_cast(g, e);
    }
    UNREACHABLE("Error on expr kind (%d)", e->kind);
}

/******************************************************************************
 * Statement CodeGen
 *****************************************************************************/

static void gen_stmt(Generator *g, Stmt *s);

static void gen_block(Generator *g, Stmt *s) {
	assert(s->kind == STMT_BLOCK);
	StmtList block = s->as.block.block;

	for (size_t i = 0; i < block.len; i++) {
		gen_stmt(g, block.arr[i]);
	}
}

static void gen_func(Generator *g, Stmt *s) {
    assert(s->kind == STMT_FUNC);

    if (s->as.func.specifier == SPEC_EXTERN) return;

    if (s->as.func.specifier == SPEC_PUB) gprintf(g, "export ");
    gprintf(g, "function %c $%.*s(", qbe_type(s->as.func.ret), strf(s->as.func.name));

    StmtList params = s->as.func.params;
    for (size_t i = 0; i < params.len; i++) {
        gprintf(g, "%c %%%.*s, ", qbe_type(params.arr[i]->as.var.type), strf(params.arr[i]->as.var.name));
    }
    gprintfln(g, ") {");

    g->inFunc = true;
    gprintfln(g, "@start");

    for (size_t i = 0; i < params.len; i++) {
        String qvar = qbe_var(s->loc);
        VarStmt var = params.arr[i]->as.var;
        gprintfln(g, "%.*s =l alloc%d %d", strf(qvar), var.type->align, var.type->size);
        gprintfln(g, "store%c %%%.*s, %.*s", qbe_type(var.type), strf(var.name), strf(qvar));
        params.arr[i]->as.var.qbe_var = qvar;
    }

    StmtList body = s->as.func.body;
    for (size_t i = 0; i < body.len; i++) gen_stmt(g, body.arr[i]);

    gprintfln(g, "@end");

    g->inFunc = false;
    gprintfln(g, "ret 0");
    gprintfln(g, "}\n");
}

static void gen_return(Generator *g, Stmt *s) {
    assert(s->kind == STMT_RETURN);
    Expr *retVal = s->as.returnS.retVal;

    if (!retVal)
        gprintfln(g, "ret");
    else {
        String ret = gen_expr(g, retVal);
        gprintfln(g, "ret %.*s", strf(ret));
    }

    String nextLbl = qbe_label("next", s->loc);
    gprint_lbl(g, nextLbl);
}

static void gen_var(Generator *g, Stmt *s) {
    assert(s->kind == STMT_VAR);
    VarStmt varStmt = s->as.var;
    if (varStmt.specifier == SPEC_STATIC) {
        accumulate_data(g, s, true);
        return;
    }

    if (!g->inFunc) return;

    String qvar = qbe_var(s->loc);
    gprintfln(g, "%.*s =l alloc%d %d", strf(qvar), varStmt.type->align, varStmt.type->size);
    assert(varStmt.init);
    String init = gen_expr(g, varStmt.init);
    gprintfln(g, "store%c %.*s, %.*s", qbe_type(varStmt.type), strf(init), strf(qvar));
    s->as.var.qbe_var = qvar;
}

static void gen_while(Generator *g, Stmt *s) {
    assert(s->kind == STMT_WHILE);
    WhileStmt whileStmt = s->as.whileS;

    String condLbl = qbe_label("cond", s->loc);
    String bodyLbl = qbe_label("loop", s->loc);
    String endLbl = qbe_label("end", s->loc);

    push_labels(g, endLbl, condLbl);

    gprint_lbl(g, condLbl);
    String condVal = gen_expr(g, whileStmt.condition);
    gprintfln(g, "jnz %.*s, %.*s, %.*s", strf(condVal), strf(bodyLbl), strf(endLbl));

    gprint_lbl(g, bodyLbl);
    gen_stmt(g, whileStmt.body);
    gprintfln(g, "jmp %.*s", strf(condLbl));

    gprint_lbl(g, endLbl);
    pop_labels(g);
}

static void gen_do_while(Generator *g, Stmt *s) {
    assert(s->kind == STMT_DO_WHILE);
    WhileStmt w = s->as.whileS;

    String condLbl = qbe_label("cond", s->loc);
    String bodyLbl = qbe_label("loop", s->loc);
    String endLbl = qbe_label("end", s->loc);

    push_labels(g, endLbl, condLbl);

    gprint_lbl(g, bodyLbl);
    gen_stmt(g, w.body);

    gprint_lbl(g, condLbl);
    String condVal = gen_expr(g, w.condition);
    gprintfln(g, "jnz %.*s, %.*s, %.*s", strf(condVal), strf(bodyLbl), strf(endLbl));

    gprint_lbl(g, endLbl);
    pop_labels(g);
}

static void gen_if(Generator *g, Stmt *s) {
	assert(s->kind == STMT_IF);
	IfStmt ifStmt = s->as.ifS;

	String iftLbl = qbe_label("then", s->loc);
	String endLbl = qbe_label("end", s->loc);
	String iffLbl = ifStmt.elseBranch ? qbe_label("else", s->loc) : endLbl;

	String condVal = gen_expr(g, ifStmt.condition);
    String tempCond = qbe_var(s->loc);
    gprintfln(g, "%.*s =w cne%c %.*s, 0", strf(tempCond), qbe_type(ifStmt.condition->type), strf(condVal));
	gprintfln(g, "jnz %.*s, %.*s, %.*s", strf(tempCond), strf(iftLbl), strf(iffLbl));
    gprint_lbl(g, iftLbl);
	gen_stmt(g, ifStmt.thenBranch);
	gprintfln(g, "jmp %.*s", strf(endLbl));

	if (ifStmt.elseBranch) {
		gprint_lbl(g, iffLbl);
		gen_stmt(g, ifStmt.elseBranch);
		gprintfln(g, "jmp %.*s", strf(endLbl));
	}

	gprint_lbl(g, endLbl);
}

static void gen_for(Generator *g, Stmt *s) {
    assert(s->kind == STMT_FOR);
    ForStmt forStmt = s->as.forS;

    String condLbl = qbe_label("cond", s->loc);
    String bodyLbl = qbe_label("body", s->loc);
    String incLbl = qbe_label("inc", s->loc);
    String endLbl = qbe_label("end", s->loc);

    push_labels(g, endLbl, incLbl);

    if (forStmt.initializer) gen_stmt(g, forStmt.initializer);

    gprint_lbl(g, condLbl);
    if (forStmt.condition) {
        String condVal = gen_expr(g, forStmt.condition);
        gprintfln(g, "jnz %.*s, %.*s, %.*s", strf(condVal), strf(bodyLbl), strf(endLbl));
    }

    gprint_lbl(g, bodyLbl);
    gen_stmt(g, forStmt.body);

    gprint_lbl(g, incLbl);
    if (forStmt.increment) gen_expr(g, forStmt.increment);
    gprintfln(g, "jmp %.*s", strf(condLbl));

    gprint_lbl(g, endLbl);

    pop_labels(g);
}

static void gen_break(Generator *g, Stmt *s) {
    assert(s->kind == STMT_BREAK);
    String breakLbl = get_break_lbl(g);
    gprintfln(g, "jmp %.*s", strf(breakLbl));
    gprint_lbl(g, qbe_label("next", s->loc));
}

static void gen_continue(Generator *g, Stmt *s) {
    assert(s->kind == STMT_CONTINUE);
    String continueLbl = get_cont_lbl(g);
    gprintfln(g, "jmp %.*s", strf(continueLbl));
    gprint_lbl(g, qbe_label("next", s->loc));
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

void gen_data_var(Generator *g, Stmt *s) {
    assert(s->kind == STMT_VAR);
    VarStmt varStmt = s->as.var;

    if (varStmt.specifier == SPEC_PUB) gprintf(g, "export ");
    if (varStmt.specifier != SPEC_EXTERN)
        gprintfln(g, "data %.*s = { %c %ld }", strf(varStmt.qbe_var), qbe_type(varStmt.type), varStmt.initVal);
    return;
}

void codegen(TranslationUnit *unit, FILE *outf) {
    if (!outf) outf = stdout;

    Generator g = (Generator){
        .outf = outf,
        .unit = unit,
        .labels = {0},
        .data = {0},
        .inFunc = false,
    };

    FOR_EACH(&unit->ast, Stmt *, s) {
        if ((*s)->kind == STMT_VAR) {
            accumulate_data(&g, *s, false);
        }
    }

    FOR_EACH(&unit->ast, Stmt *, s) {
         gen_stmt(&g, *s);
    }

    FOR_EACH(&g.data, Stmt *, s) {
        gen_data_var(&g, *s);
    }
}
