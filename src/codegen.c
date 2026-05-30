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

    Scope *scope;
} Generator;

int gprintf(Generator *g, const char *fmt, ...) {
    static size_t line = 0;
    if (g->outf == stdout) printf("[%zu] ", line++);
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

/******************************************************************************
 * Scope Helpers
 *****************************************************************************/

static void enter_scope(Generator *g) {
    Scope *newScope = calloc(1, sizeof(Scope));
    newScope->above = g->scope;
    g->scope = newScope;
}

static inline void exit_scope(Generator *g) {
    Scope *oldScope = g->scope;
    g->scope = oldScope->above;
    free(oldScope);
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

    return 0;
}

static Stmt *expect_var(Generator *g, Token nameTok) {
    assert(nameTok.kind == TOK_IDENTIFIER);
    Stmt *found = find_symbol(g->scope, nameTok.as.identifier);
    if (!found) compile_error(g->unit->fileName, nameTok.loc, "unkown symbol '%.*s'", strf(nameTok.as.identifier));
    return found;
}

static String declare_var(Generator *g, Stmt *varStmt) {
    assert(varStmt->kind == STMT_VAR);

    String varName = varStmt->as.var.name.as.identifier;
    // Stmt *found = find_symbol(g->scope, varName);
    Stmt *found = hm_find_val(&g->scope->symbols, varName);
    if (found)
        compile_error(g->unit->fileName, varStmt->loc, "symbol '%.*s' already delcared before at [%.*s:%zu:%zu]",
                      strf(varName), strf(g->unit->fileName), varStmt->loc.line, varStmt->loc.col);

    varStmt->as.var.qbeVarAddr = qbe_id(varStmt->loc);
    hm_insert(&g->scope->symbols, varName, varStmt);
    return varStmt->as.var.qbeVarAddr;
}

/******************************************************************************
 * Expression CodeGen
 *****************************************************************************/

static String gen_expr(Generator *g, Expr *e);

static String gen_primary(Generator *g, Expr *e) {
    Token val = e->as.primary.value;

    switch (val.kind) {
        case TOK_IDENTIFIER: {
            String varAddr = expect_var(g, val)->as.var.qbeVarAddr;
            String temp = qbe_id(val.loc);
            gprintf(g, "%.*s =w loadw %.*s\n", strf(temp), strf(varAddr));
            return temp;
        }
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

            // check lhs, if it's false, no need to check rhs, jump to false label
            String lhs = gen_expr(g, binary.lhs);
            gprintf(g, "jnz %.*s, @%.*s, @%.*s\n", strf(lhs), strf(rhsLabel), strf(falseLabel));

            gprintf(g, "@%.*s\n", strf(rhsLabel));
            String rhs = gen_expr(g, binary.rhs);
            gprintf(g, "jnz %.*s, @%.*s, @%.*s\n", strf(rhs), strf(trueLabel), strf(falseLabel));

            gprintf(g, "@%.*s\n", strf(trueLabel));
            gprintf(g, "jmp @%.*s\n", strf(endLabel));

            gprintf(g, "@%.*s\n", strf(falseLabel));
            gprintf(g, "jmp @%.*s\n", strf(endLabel));

            gprintf(g, "@%.*s\n", strf(endLabel));
            gprintf(g, "%.*s =w phi @%.*s 1, @%.*s 0\n", strf(out), strf(trueLabel), strf(falseLabel));
            return out;
        }

        case TOK_PIPE_PIPE: {
            String endLabel = get_label("end", e->loc);
            String trueLabel = get_label("true", e->loc);
            String falseLabel = get_label("false", e->loc);
            String rhsLabel = get_label("rhs", e->loc);

            // check lhs, if it's true, no need to check rhs, jump to true label
            String lhs = gen_expr(g, binary.lhs);
            gprintf(g, "jnz %.*s, @%.*s, @%.*s\n", strf(lhs), strf(trueLabel), strf(rhsLabel));

            gprintf(g, "@%.*s\n", strf(rhsLabel));
            String rhs = gen_expr(g, binary.rhs);
            gprintf(g, "jnz %.*s, @%.*s, @%.*s\n", strf(rhs), strf(trueLabel), strf(falseLabel));

            gprintf(g, "@%.*s\n", strf(trueLabel));
            gprintf(g, "jmp @%.*s\n", strf(endLabel));

            gprintf(g, "@%.*s\n", strf(falseLabel));
            gprintf(g, "jmp @%.*s\n", strf(endLabel));

            gprintf(g, "@%.*s\n", strf(endLabel));
            gprintf(g, "%.*s =w phi @%.*s 1, @%.*s 0\n", strf(out), strf(trueLabel), strf(falseLabel));
            return out;
        }

        default: UNREACHABLE("%s: Unsupported binary operator (%s)", __func__, tokenTypesStrings[binary.op]);
    }

    return out;
}

static String gen_lvalue(Generator *g, Expr *e) {
    switch (e->kind) {
        case EXPR_PRIMARY: {
            if (e->as.primary.value.kind != TOK_IDENTIFIER) break;
            return expect_var(g, e->as.primary.value)->as.var.qbeVarAddr;
        }
        case EXPR_GROUPING: return gen_lvalue(g, e->as.grouping.inner);
        case EXPR_UNARY:    {
            if (e->as.unary.op != TOK_STAR) break;
            return gen_expr(g, e->as.unary.inner);
        }
        default: break;
    }

    compile_error(g->unit->fileName, e->loc, "expression is not assignable");
}

static String gen_unary(Generator *g, Expr *e) {
    UnaryExpr unary = e->as.unary;
    String out = qbe_id(e->loc);

    switch (unary.op) {
        case TOK_TILDE: {
            String inner = gen_expr(g, unary.inner);
            gprintf(g, "%.*s =w xor %.*s, -1\n", strf(out), strf(inner));
            return out;
        }

        case TOK_MINUS: {
            String inner = gen_expr(g, unary.inner);
            gprintf(g, "%.*s =w sub 0, %.*s\n", strf(out), strf(inner));
            return out;
        }

        case TOK_BANG: {
            String inner = gen_expr(g, unary.inner);
            String endLabel = get_label("end", e->loc);
            String trueLabel = get_label("true", e->loc);
            String falseLabel = get_label("false", e->loc);

            gprintf(g, "jnz %.*s, @%.*s, @%.*s\n", strf(inner), strf(falseLabel), strf(trueLabel));

            gprintf(g, "@%.*s\n", strf(trueLabel));
            gprintf(g, "%.*s =w copy 1\n", strf(out));
            gprintf(g, "jmp @%.*s\n", strf(endLabel));
            gprintf(g, "@%.*s\n", strf(falseLabel));
            gprintf(g, "%.*s =w copy 0\n", strf(out));
            gprintf(g, "@%.*s\n", strf(endLabel));
            return out;
        }

        case TOK_PLUS_PLUS: {
            String inner = gen_lvalue(g, unary.inner);
            String loadTemp = qbe_id(e->loc);
            gprintf(g, "%.*s =w loadw %.*s\n", strf(loadTemp), strf(inner));
            gprintf(g, "%.*s =w add %.*s, 1\n", strf(out), strf(loadTemp));
            gprintf(g, "storew %.*s, %.*s\n", strf(out), strf(inner));
            return out;
        }

        case TOK_MINUS_MINUS: {
            String inner = gen_lvalue(g, unary.inner);
            String loadTemp = qbe_id(e->loc);
            gprintf(g, "%.*s =w loadw %.*s\n", strf(loadTemp), strf(inner));
            gprintf(g, "%.*s =w sub %.*s, 1\n", strf(out), strf(loadTemp));
            gprintf(g, "storew %.*s, %.*s\n", strf(out), strf(inner));
            return out;
        }

        default: UNREACHABLE("%s: Unsupported unary operator (%s)", __func__, tokenTypesStrings[unary.op]);
    }
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
        default:                         UNREACHABLE("Not an assignment operand (%s)", tokenTypesStrings[op]);
    }
}

static String gen_assign(Generator *g, Expr *e) {
    AssignExpr assign = e->as.assignment;

    String lhs = gen_lvalue(g, assign.lhs);
    String rhs = gen_expr(g, assign.rhs);

    switch (assign.op) {
        case TOK_EQUALS:                 gprintf(g, "storew %.*s, %.*s\n", strf(rhs), strf(lhs)); break;
        case TOK_PLUS_EQUALS:
        case TOK_MINUS_EQUALS:
        case TOK_STAR_EQUALS:
        case TOK_SLASH_EQUALS:
        case TOK_PERCENT_EQUALS:
        case TOK_AMPERSAND_EQUALS:
        case TOK_PIPE_EQUALS:
        case TOK_CARET_EQUALS:
        case TOK_LESS_LESS_EQUALS:
        case TOK_GREATER_GREATER_EQUALS: {
            String temp = qbe_id(e->loc);
            gprintf(g, "%.*s =w loadw %.*s\n", strf(temp), strf(lhs));
            gprintf(g, "%.*s =w %s %.*s, %.*s\n", strf(temp), get_assign_op(assign.op), strf(temp), strf(rhs),
                    strf(temp));
            gprintf(g, "storew %.*s, %.*s\n", strf(temp), strf(lhs));
            break;
        }

        default: UNREACHABLE("operand %s", tokenTypesStrings[assign.op]);
    }

    String temp = qbe_id(e->loc);
    gprintf(g, "%.*s =w loadw %.*s\n", strf(temp), strf(lhs));
    return temp;
}

static String gen_unary_post(Generator *g, Expr *e) {
    UnaryExpr unary = e->as.unary;
    String out = qbe_id(e->loc);
    String temp = qbe_id(e->loc);
    String inner = gen_lvalue(g, unary.inner);

    const char *opStr;
    switch (unary.op) {
        case TOK_PLUS_PLUS:   opStr = "add"; break;
        case TOK_MINUS_MINUS: opStr = "sub"; break;
        default:              UNREACHABLE("%s: Unsupported unary operator (%s)", __func__, tokenTypesStrings[unary.op]);
    }

    gprintf(g, "%.*s =w loadw %.*s\n", strf(out), strf(inner));
    gprintf(g, "%.*s =w %s %.*s, 1\n", strf(temp), opStr, strf(out));
    gprintf(g, "storew %.*s, %.*s\n", strf(temp), strf(inner));
    return out;
}

static String gen_conditional(Generator *g, Expr *e) {
    ConditionalExpr c = e->as.conditional;
    String temp = qbe_id(e->loc);
    String thenLabel = get_label("then", e->loc);
    String thenLabelEnd = get_label("thenEnd", e->loc);
    String elseLabel = get_label("else", e->loc);
    String elseLabelEnd = get_label("elseEnd", e->loc);
    String endLabel = get_label("end", e->loc);

    String cond = gen_expr(g, c.condition);
    gprintf(g, "jnz %.*s, @%.*s, @%.*s\n", strf(cond), strf(thenLabel), strf(elseLabel));

    gprintf(g, "@%.*s\n", strf(thenLabel));
    String thenVal = gen_expr(g, c.thenBranch);
    gprintf(g, "@%.*s\n", strf(thenLabelEnd));
    gprintf(g, "jmp @%.*s\n", strf(endLabel));

    gprintf(g, "@%.*s\n", strf(elseLabel));
    String elseVal = gen_expr(g, c.elseBranch);
    gprintf(g, "@%.*s\n", strf(elseLabelEnd));
    gprintf(g, "jmp @%.*s\n", strf(endLabel));

    gprintf(g, "@%.*s\n", strf(endLabel));
    gprintf(g, "%.*s =w phi @%.*s %.*s, @%.*s %.*s\n", strf(temp), strf(thenLabelEnd), strf(thenVal),
            strf(elseLabelEnd), strf(elseVal));

    return temp;
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
    StmtList block = s->as.block.block;
    enter_scope(g);
    for (size_t i = 0; i < block.len; i++) gen_stmt(g, block.arr[i]);
    exit_scope(g);
}

static void gen_func(Generator *g, Stmt *s) {
    enter_scope(g);

    gprintf(g, "export function w $%.*s() {\n", strf(s->as.func.name.as.identifier));
    gprintf(g, "@start\n");

    for (size_t i = 0; i < s->as.func.block.len; i++) {
        gen_stmt(g, s->as.func.block.arr[i]);
    }

    // TODO: empty functions, yay or nay?
    gprintf(g, "@end\n");
    gprintf(g, "ret 0\n");
    gprintf(g, "}\n");

    exit_scope(g);
}

static void gen_return(Generator *g, Stmt *s) {
    if (!s->as.returnS.retVal)
        gprintf(g, "ret\n");
    else
        gprintf(g, "ret %s\n", gen_expr(g, s->as.returnS.retVal));

    String retLabel = get_label("ret", s->loc);
    gprintf(g, "@%.*s\n", strf(retLabel));
}

static void gen_var(Generator *g, Stmt *s) {
    assert(s->kind == STMT_VAR);
    VarStmt var = s->as.var;

    String qbeVarAddr = declare_var(g, s);
    gprintf(g, "%.*s =l alloc4 1\n", strf(qbeVarAddr));
    gprintf(g, "storew 0, %.*s\n", strf(qbeVarAddr));

    if (var.init) {
        String init = gen_expr(g, var.init);
        gprintf(g, "storew %.*s, %.*s\n", strf(init), strf(qbeVarAddr));
    }

    return;
}

static void gen_while(Generator *g, Stmt *s) {
    (void)g;
    (void)s;
    TODO("%s", __func__);
}

static void gen_if(Generator *g, Stmt *s) {
    IfStmt ifS = s->as.ifS;

    String cond = gen_expr(g, ifS.condition);
    String thenLabel = get_label("then", s->loc);
    String endLabel = get_label("end", s->loc);
    String elseLabel = ifS.elseBranch ? get_label("else", s->loc) : endLabel;

    enter_scope(g);

    gprintf(g, "jnz %.*s, @%.*s, @%.*s\n", strf(cond), strf(thenLabel), strf(elseLabel));
    gprintf(g, "@%.*s\n", strf(thenLabel));
    gen_stmt(g, ifS.thenBranch);
    gprintf(g, "jmp @%.*s\n", strf(endLabel));
    if (ifS.elseBranch) {
        gprintf(g, "@%.*s\n", strf(elseLabel));
        gen_stmt(g, ifS.elseBranch);
        gprintf(g, "jmp @%.*s\n", strf(endLabel));
    }
    gprintf(g, "@%.*s\n", strf(endLabel));

    exit_scope(g);
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
        case STMT_NULL:                                   break;
        case STMT_BLOCK:    gen_block(g, s);              break;
        case STMT_FUNC:     gen_func(g, s);               break;
        case STMT_RETURN:   gen_return(g, s);             break;
        case STMT_VAR:      gen_var(g, s);                break;
        case STMT_EXPR:     gen_expr(g, s->as.expr.expr); break;
        case STMT_WHILE:    gen_while(g, s);              break;
        case STMT_IF:       gen_if(g, s);                 break;
        case STMT_FOR:      gen_for(g, s);                break;
        case STMT_BREAK:    gen_break(g, s);              break;
        case STMT_CONTINUE: gen_continue(g, s);           break;
    }
}

void codegen(TranslationUnit *unit, FILE *outf) {
    if (!outf) outf = stdout;

    Generator g = (Generator){
        .outf = outf,
        .unit = unit,
        .scope = &unit->globalSymbols,
    };

    for (size_t i = 0; i < unit->ast.len; i++) {
        gen_stmt(&g, unit->ast.arr[i]);
    }
}
