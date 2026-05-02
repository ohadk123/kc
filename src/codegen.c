#include "codegen.h"
#include "type.h"
#include <stdarg.h>

static uint64_t counter(void) {
    static uint64_t count = 0;
    return count++;
}

String qbe_id(Location loc) {
    int id = counter();
    return str_printf("%%_ktemp_%zu_%zu_%zu", id++, loc.line, loc.col);
}

String get_label(const char *name, Location loc) {
    return str_printf("__%s.%zu.%zu.%zu", name, loc.line, loc.col, counter());
}

typedef struct {
    FILE *outf;
    TranslationUnit *unit;
} Generator;

int gprintf(Generator *g, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int ret = vfprintf(g->outf, fmt, args);
    va_end(args);
    return ret;
}

const char *ktype_to_qbe_ext(Type *type) {
    (void)type;
    TODO("%s", __func__);
}

static String gen_expr(Generator *g, Expr *e);

static String gen_primary(Generator *g, Expr *e) {
    Token val = e->as.primary.value;

    switch (val.kind) {
        case TOK_IDENTIFIER:      TODO("%s: TOK_IDENTIFIER", __func__);
        case TOK_STRING_LITERAL:  TODO("%s: TOK_STRING_LITERAL", __func__);
        case TOK_TRUE:            TODO("%s: TOK_TRUE", __func__);
        case TOK_FALSE:           TODO("%s: TOK_FALSE", __func__);
        case TOK_CHAR_LITERAL:    TODO("%s: TOK_CHAR_LITERAL", __func__);
        case TOK_INTEGER_LITERAL: return str_printf("%zu", val.as.integerLiteral);
        case TOK_FLOAT_LITERAL:   TODO("%s: TOK_FLOAT_LITERAL", __func__);
        default:                  compile_error(g->unit->fileName, e->loc, "Unsupported primary expression");
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
    BinaryExpr binary = e->as.binary;

    String out = qbe_id(e->loc);
    switch (binary.op) {
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
        case TOK_LESS_EQUALS:     {
            String lhs = gen_expr(g, binary.lhs);
            String rhs = gen_expr(g, binary.rhs);
            gprintf(g, "%.*s =w %s %.*s, %.*s\n", strf(out), get_bin_op(binary.op), strf(lhs), strf(rhs));
            break;
        }

        case TOK_AMPERSAND_AMPERSAND: {
            String endLabel = get_label("end", e->loc);
            String trueLabel = get_label("true", e->loc);
            String falseLabel = get_label("false", e->loc);
            String rhsLabel = get_label("rhs", e->loc);

            String lhs = gen_expr(g, binary.lhs);
            gprintf(g, "jnz %.*s, @%.*s, @%.*s\n", strf(lhs), strf(rhsLabel), strf(falseLabel));
            gprintf(g, "@%.*s\n", strf(rhsLabel));
            String rhs = gen_expr(g, binary.rhs);
            gprintf(g, "jnz %.*s, @%.*s, @%.*s\n", strf(rhs), strf(trueLabel), strf(falseLabel));
            gprintf(g, "@%.*s\n", strf(trueLabel));
            gprintf(g, "%.*s =w copy 1\n", strf(out));
            gprintf(g, "jmp @%.*s\n", strf(endLabel));
            gprintf(g, "@%.*s\n", strf(falseLabel));
            gprintf(g, "%.*s =w copy 0\n", strf(out));
            gprintf(g, "@%.*s\n", strf(endLabel));
            return out;
        }

        case TOK_PIPE_PIPE: {
            String endLabel = get_label("end", e->loc);
            String trueLabel = get_label("true", e->loc);
            String falseLabel = get_label("false", e->loc);
            String rhsLabel = get_label("rhs", e->loc);

            String lhs = gen_expr(g, binary.lhs);
            gprintf(g, "jnz %.*s, @%.*s, @%.*s\n", strf(lhs), strf(trueLabel), strf(rhsLabel));

            gprintf(g, "@%.*s\n", strf(rhsLabel));
            String rhs = gen_expr(g, binary.rhs);
            gprintf(g, "jnz %.*s, @%.*s, @%.*s\n", strf(rhs), strf(trueLabel), strf(falseLabel));

            gprintf(g, "@%.*s\n", strf(trueLabel));
            gprintf(g, "%.*s =w copy 1\n", strf(out));
            gprintf(g, "jmp @%.*s\n", strf(endLabel));
            gprintf(g, "@%.*s\n", strf(falseLabel));
            gprintf(g, "%.*s =w copy 0\n", strf(out));
            gprintf(g, "@%.*s\n", strf(endLabel));
            return out;
        }

        default: TODO("%s: Unsupported binary operator (%s)", __func__, tokenTypesStrings[binary.op]);
    }

    return out;
}

static String gen_unary(Generator *g, Expr *e) {
    UnaryExpr unary = e->as.unary;
    (void)g;
    String inner = gen_expr(g, unary.inner);
    String out = qbe_id(e->loc);

    switch (unary.op) {
        case TOK_TILDE: {
            gprintf(g, "%.*s =w xor %.*s, -1\n", strf(out), strf(inner));
            return out;
        }
        case TOK_MINUS: {
            gprintf(g, "%.*s =w sub 0, %.*s\n", strf(out), strf(inner));
            return out;
        }
        case TOK_BANG: {
            String endLabel = get_label("end", e->loc);
            String trueLabel = get_label("true", e->loc);
            String falseLabel = get_label("false", e->loc);

            String inner = gen_expr(g, unary.inner);
            gprintf(g, "jnz %.*s, @%.*s, @%.*s\n", strf(inner), strf(falseLabel), strf(trueLabel));

            gprintf(g, "@%.*s\n", strf(trueLabel));
            gprintf(g, "%.*s =w copy 1\n", strf(out));
            gprintf(g, "jmp @%.*s\n", strf(endLabel));
            gprintf(g, "@%.*s\n", strf(falseLabel));
            gprintf(g, "%.*s =w copy 0\n", strf(out));
            gprintf(g, "@%.*s\n", strf(endLabel));
            return out;
        }
        default: TODO("%s: Unsupported unary operator (%s)", __func__, tokenTypesStrings[unary.op]);
    }
}

static String gen_assign(Generator *g, Expr *e) {
    (void)g;
    (void)e;
    TODO("%s", __func__);
}

static String gen_unary_post(Generator *g, Expr *e) {
    (void)g;
    (void)e;
    TODO("%s", __func__);
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
        case EXPR_PRIMARY:     return gen_primary(g, e); break;
        case EXPR_GROUPING:    return gen_expr(g, e->as.grouping.inner); break;
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

static void gen_stmt(Generator *g, Stmt *s);

static void gen_func(Generator *g, Stmt *s) {
    gprintf(g, "export function w $%.*s() {\n", strf(s->as.func.name.as.identifier));
    gprintf(g, "@start\n");

    for (size_t i = 0; i < s->as.func.block.len; i++) {
        gen_stmt(g, s->as.func.block.arr[i]);
    }

    gprintf(g, "}\n");
}

static void gen_return(Generator *g, Stmt *s) {
    if (!s->as.returnS.retVal)
        gprintf(g, "ret\n");
    else
        gprintf(g, "ret %s\n", gen_expr(g, s->as.returnS.retVal));
}

static void gen_var(Generator *g, Stmt *s) {
    (void)g;
    (void)s;
    TODO("%s", __func__);
}

static void gen_while(Generator *g, Stmt *s) {
    (void)g;
    (void)s;
    TODO("%s", __func__);
}

static void gen_if(Generator *g, Stmt *s) {
    (void)g;
    (void)s;
    TODO("%s", __func__);
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
        case STMT_BLOCK: {
            StmtList block = s->as.block.block;
            for (size_t i = 0; i < block.len; i++) gen_stmt(g, block.arr[i]);
            break;
        }
        case STMT_FUNC:     gen_func(g, s); break;
        case STMT_RETURN:   gen_return(g, s); break;
        case STMT_VAR:      gen_var(g, s); break;
        case STMT_EXPR:     gen_expr(g, s->as.expr.expr); break;
        case STMT_WHILE:    gen_while(g, s); break;
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
