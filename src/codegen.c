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

static String qbe_label(const char *name, Location l) {
    uint64_t i = counter();
    return str_printf("@_%s_%zu_%zu_%zu", name, i, l.line, l.col);
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
    Token val = e->as.primary.value;

    switch (val.kind) {
        case TOK_IDENTIFIER: {
            String out = qbe_var(e->loc);
            String qvar = get_lvalue(g, e);
            gprintfln(g, "%.*s =w loadw %.*s", strf(out), strf(qvar));
            return out;
        }
        case TOK_STRING_LITERAL:  TODO("%s: TOK_STRING_LITERAL", __func__);
        case TOK_CHAR_LITERAL:    return str_printf("%u", val.as.charLiteral);
        case TOK_INTEGER_LITERAL: return str_printf("%zu", val.as.integerLiteral);
        case TOK_FLOAT_LITERAL:   TODO("%s: TOK_FLOAT_LITERAL", __func__);
        case TOK_TRUE:            return str_from_cstr("1");
        case TOK_FALSE:           return str_from_cstr("0");
        default:                  UNREACHABLE("%s: Unsupported token kind: %s", __func__, tokenTypesStrings[val.kind]);
    }
}
static const char *get_bin_op(TokenKind op) {
    switch (op) {
        case TOK_PLUS:            return "add";
        case TOK_MINUS:           return "sub";
        case TOK_STAR:            return "mul";
        case TOK_SLASH:           return "div";
        case TOK_PERCENT:         return "rem";
        case TOK_CARET:           return "xor";
        case TOK_LESS_LESS:       return "shl";
        case TOK_GREATER_GREATER: return "sar";
        case TOK_AMPERSAND:       return "and";
        case TOK_PIPE:            return "or";
        case TOK_EQUALS_EQUALS:   return "ceqw";
        case TOK_BANG_EQUALS:     return "cnew";
        case TOK_GREATER:         return "csgtw";
        case TOK_GREATER_EQUALS:  return "csgew";
        case TOK_LESS:            return "csltw";
        case TOK_LESS_EQUALS:     return "cslew";
        default:                  UNREACHABLE("Not an arithmetic operand (%s)", tokenTypesStrings[op]);
    }
}

static String gen_binary(Generator *g, Expr *e) {
    assert(e->kind == EXPR_BINARY);
    BinaryExpr bin = e->as.binary;

    String out = qbe_var(e->loc);
    switch (bin.op) {
        case TOK_PLUS:
        case TOK_MINUS:
        case TOK_STAR:
        case TOK_SLASH:
        case TOK_PERCENT:
        case TOK_CARET:
        case TOK_LESS_LESS:
        case TOK_GREATER_GREATER:
        case TOK_AMPERSAND:
        case TOK_PIPE:
        case TOK_EQUALS_EQUALS:
        case TOK_BANG_EQUALS:
        case TOK_GREATER:
        case TOK_GREATER_EQUALS:
        case TOK_LESS:
        case TOK_LESS_EQUALS: {
            String lhs = gen_expr(g, bin.lhs);
            String rhs = gen_expr(g, bin.rhs);
            gprintfln(g, "%.*s =w %s %.*s, %.*s", strf(out), get_bin_op(bin.op), strf(lhs), strf(rhs));
            break;
        }

        case TOK_AMPERSAND_AMPERSAND:
        case TOK_PIPE_PIPE: {
            String one = qbe_label("one", e->loc);
            String zero = qbe_label("zero", e->loc);
            String rhsL = qbe_label("rhs", e->loc);
            String end = qbe_label("end", e->loc);
            bool isAnd = bin.op == TOK_AMPERSAND_AMPERSAND;

            String lhs = gen_expr(g, bin.lhs);
            gprintfln(g, "jnz %.*s, %.*s, %.*s", strf(lhs), strf(isAnd ? rhsL : one), strf(isAnd ? zero : rhsL));

            gprintfln(g, "%.*s", strf(rhsL));
            String rhs = gen_expr(g, bin.rhs);
            gprintfln(g, "jnz %.*s, %.*s, %.*s", strf(rhs), strf(one), strf(zero));

            gprintfln(g, "%.*s", strf(zero));
            gprintfln(g, "jmp %.*s", strf(end));

            gprintfln(g, "%.*s", strf(one));
            gprintfln(g, "jmp %.*s", strf(end));

            gprintfln(g, "%.*s", strf(end));
            gprintfln(g, "%.*s =w phi %.*s 0, %.*s 1", strf(out), strf(zero), strf(one));
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
            gprintfln(g, "%.*s =w sub 0, %.*s", strf(out), strf(inner));
            break;
        }

        case TOK_TILDE: {
            String inner = gen_expr(g, un.inner);
            gprintfln(g, "%.*s =w xor %.*s, -1", strf(out), strf(inner));
            break;
        }

        case TOK_BANG: {
            String inner = gen_expr(g, un.inner);
            gprintfln(g, "%.*s =w ceqw %.*s, 0", strf(out), strf(inner));
            break;
        }

        case TOK_PLUS_PLUS:
        case TOK_MINUS_MINUS: {
            String inner = get_lvalue(g, un.inner);
            String loadTemp = qbe_var(e->loc);
            const char *op = un.op == TOK_PLUS_PLUS ? "add" : "sub";

            gprintfln(g, "%.*s =w loadw %.*s", strf(loadTemp), strf(inner));
            gprintfln(g, "%.*s =w %s %.*s, 1", strf(out), op, strf(loadTemp));
            gprintfln(g, "storew %.*s, %.*s", strf(out), strf(inner));
            break;
        }

        default: TODO("%s: Unary operator \"%s\"", __func__, tokenTypesStrings[un.op]);
    }

    return out;
}

static const char *get_assign_op(TokenKind op) {
    switch (op) {
        case TOK_PLUS_EQUALS:            return "add";
        case TOK_MINUS_EQUALS:           return "sub";
        case TOK_STAR_EQUALS:            return "mul";
        case TOK_SLASH_EQUALS:           return "div";
        case TOK_PERCENT_EQUALS:         return "rem";
        case TOK_AMPERSAND_EQUALS:       return "and";
        case TOK_PIPE_EQUALS:            return "or";
        case TOK_CARET_EQUALS:           return "xor";
        case TOK_LESS_LESS_EQUALS:       return "shl";
        case TOK_GREATER_GREATER_EQUALS: return "sar";
        default: UNREACHABLE("Not an assignment operand (%s)", tokenTypesStrings[op]);
    }
}


static String gen_assign(Generator *g, Expr *e) {
    assert(e->kind == EXPR_ASSIGN);
    AssignExpr assign = e->as.assignment;

    String lhs = get_lvalue(g, assign.lhs);
    String rhs = gen_expr(g, assign.rhs);
    String out = qbe_var(e->loc);

    if (assign.op == TOK_EQUALS) {
        gprintfln(g, "%.*s =w copy %.*s", strf(out), strf(rhs));
    } else {
        String old = qbe_var(e->loc);
        gprintfln(g, "%.*s =w loadw %.*s", strf(old), strf(lhs));
        gprintfln(g, "%.*s =w %s %.*s, %.*s", strf(out), get_assign_op(assign.op), strf(old), strf(rhs));
    }

    gprintfln(g, "storew %.*s, %.*s", strf(out), strf(lhs));
    return out;
}

static String gen_unary_post(Generator *g, Expr *e) {
    assert(e->kind == EXPR_UNARY_POST);
    UnaryExpr un = e->as.unary;

    String out = qbe_var(e->loc);
    String temp = qbe_var(e->loc);
    String inner = get_lvalue(g, un.inner);
    const char *op = un.op == TOK_PLUS_PLUS ? "add" : "sub";

    gprintfln(g, "%.*s =w loadw %.*s", strf(out), strf(inner));
    gprintfln(g, "%.*s =w %s %.*s, 1", strf(temp), op, strf(out));
    gprintfln(g, "storew %.*s, %.*s", strf(temp), strf(inner));

    return out;
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
	assert(s->kind == STMT_BLOCK);
	StmtList block = s->as.block.block;

	for (size_t i = 0; i < block.len; i++) {
		gen_stmt(g, block.arr[i]);
	}
}

static void gen_func(Generator *g, Stmt *s) {
    gprintfln(g, "export function w $main() {");
    gprintfln(g, "@start");

    StmtList body = s->as.func.body;
    for (size_t i = 0; i < body.len; i++) gen_stmt(g, body.arr[i]);

    gprintfln(g, "@end");
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
}

static void gen_var(Generator *g, Stmt *s) {
    assert(s->kind == STMT_VAR);
    VarStmt var = s->as.var;

    String qvar = qbe_var(s->loc);
    gprintfln(g, "%.*s =l alloc4 1", strf(qvar));
    assert(var.init);
    String init = gen_expr(g, var.init);
    gprintfln(g, "storew %.*s, %.*s", strf(init), strf(qvar));
    s->as.var.qbe_var = qvar;
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
	assert(s->kind == STMT_IF);
	IfStmt ifS = s->as.ifS;

	String ift = qbe_label("then", s->loc);
	String end = qbe_label("end", s->loc);
	String iff = ifS.elseBranch ? qbe_label("else", s->loc) : end;

	String cond = gen_expr(g, ifS.condition);
	gprintfln(g, "jnz %.*s, %.*s, %.*s", strf(cond), strf(ift), strf(iff));
	gen_stmt(g, ifS.thenBranch);
	gprintfln(g, "jmp %.*s", strf(end));

	if (ifS.elseBranch) {
		gprintfln(g, "%.*s", strf(iff));
		gen_stmt(g, ifS.elseBranch);
		gprintfln(g, "jmp %.*s", strf(end));
	}

	gprintfln(g, "%.*s", strf(end));
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
