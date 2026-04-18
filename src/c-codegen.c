#include "c-codegen.h"
#include <stdarg.h>

static const char *ktype_to_c(TokenKind ktype) {
    switch (ktype) {
        case TOK_INTEGER_LITERAL: return "__UINT64_TYPE__";
        case TOK_FLOAT_LITERAL:   return "double";
        case TOK_VOID:  return "void";
        case TOK_BOOL:
        case TOK_U8:    return "__UINT8_TYPE__";
        case TOK_U16:   return "__UINT16_TYPE__";
        case TOK_U32:   return "__UINT32_TYPE__";
        case TOK_U64:   return "__UINT64_TYPE__";
        case TOK_USIZE: return "__UINTPTR_TYPE__";
        case TOK_I8:    return "__INT8_TYPE__";
        case TOK_I16:   return "__INT16_TYPE__";
        case TOK_I32:   return "__INT32_TYPE__";
        case TOK_I64:   return "__INT64_TYPE__";
        case TOK_ISIZE: return "__PTRDIFF_TYPE__";
        case TOK_F32:   return "float";
        case TOK_F64:   return "double";
        default:        UNREACHABLE("Not a type kind (%s)", tokenTypesStrings[ktype]);
    }
}

static String temp_id(Location loc) {
    static size_t id = 0;
    return str_printf("_ktemp_%zu_%zu_%zu", id++, loc.line, loc.col);
}

static const char *op_str(TokenKind op) {
    switch (op) {
        case TOK_EQUALS: return "=";
        case TOK_PLUS: return "+";
        case TOK_PLUS_PLUS: return "++";
        case TOK_PLUS_EQUALS: return "+=";
        case TOK_MINUS: return "-";
        case TOK_MINUS_MINUS: return "--";
        case TOK_MINUS_EQUALS: return "-=";
        case TOK_STAR: return "*";
        case TOK_STAR_EQUALS: return "*=";
        case TOK_SLASH: return "/";
        case TOK_SLASH_EQUALS: return "/=";
        case TOK_PERCENT: return "%";
        case TOK_PERCENT_EQUALS: return "%=";
        case TOK_EQUALS_EQUALS: return "==";
        case TOK_BANG: return "!";
        case TOK_BANG_EQUALS: return "!=";
        case TOK_LESS: return "<";
        case TOK_LESS_EQUALS: return "<=";
        case TOK_GREATER: return ">";
        case TOK_GREATER_EQUALS: return ">=";
        case TOK_AMPERSAND_AMPERSAND: return "&&";
        case TOK_PIPE_PIPE: return "||";
        case TOK_AMPERSAND: return "&";
        case TOK_AMPERSAND_EQUALS: return "&=";
        case TOK_PIPE: return "|";
        case TOK_PIPE_EQUALS: return "|=";
        case TOK_CARET: return "^";
        case TOK_CARET_EQUALS: return "^=";
        case TOK_TILDE: return "~";
        case TOK_LESS_LESS: return "<<";
        case TOK_LESS_LESS_EQUALS: return "<<=";
        case TOK_GREATER_GREATER: return ">>";
        case TOK_GREATER_GREATER_EQUALS: return ">>=";
        default:        UNREACHABLE("Not a op kind (%s)", tokenTypesStrings[op]);
    }
}

typedef struct {
    FILE *outf;
    TranslationUnit *unit;
} Generator;

static int gfprintf(Generator *g, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int ret = vfprintf(g->outf, fmt, args);
    va_end(args);
    return ret;
}

static String gen_expr(Generator *g, Expr *e);

static String gen_primary(Expr *e) {
    Token prim = e->as.primary.value;

    switch (prim.kind) {
        case TOK_CHAR_LITERAL:    return str_printf("%u", prim.as.charLiteral);
        case TOK_INTEGER_LITERAL: return str_printf("%lu", prim.as.integerLiteral);
        case TOK_FLOAT_LITERAL:   return str_printf("%f", prim.as.floatLiteral);
        case TOK_TRUE:            return str_printf("1");
        case TOK_FALSE:           return str_printf("0");
        case TOK_STRING_LITERAL:  return str_printf("\"%.*s\"", strf(prim.as.stringLiteral));
        default:                  UNREACHABLE("Not a primary kind (%d)", prim.kind);
    }
}

static String gen_grouping(Generator *g, Expr *e) {
    GroupingExpr group = e->as.grouping;
    return str_printf("%.*s", gen_expr(g, group.inner));
}

static String gen_binary(Generator *g, Expr *e) {
    BinaryExpr bin = e->as.binary;

    String lhs = gen_expr(g, bin.lhs);
    String rhs = gen_expr(g, bin.rhs);

    return str_printf("%.*s %s %.*s", strf(lhs), op_str(bin.op), strf(rhs));
}

static String gen_unary(Generator *g, Expr *e) {
    UnaryExpr un = e->as.unary;
    String inner = gen_expr(g, un.inner);
    return str_printf("%s %.*s", op_str(un.op), strf(inner));
}

static String gen_unary_post(Generator *g, Expr *e) {
    UnaryExpr un = e->as.unary;
    String inner = gen_expr(g, un.inner);
    return str_printf("%.*s %s", strf(inner), op_str(un.op));
}

static String gen_func_call(Generator *g, Expr *e) {
    FuncCallExpr func = e->as.funcCall;

    StringBuilder call = {0};
    String funcName = gen_expr(g, func.func);
    sb_appendf(&call, "%.*s(", strf(funcName));

    int argsLen = func.args.len;
    for (int i = 0; i < argsLen; i++) {
        String argName = gen_expr(g, func.args.arr[i]);
        sb_appendf(&call, "%.*s", strf(argName));
        if (i < argsLen - 1) sb_appendf(&call, ", ");
    }
    sb_appendf(&call, ")");

    return finish_string(&call);
}

static String gen_conditional(Generator *g, Expr *e) {
    ConditionalExpr cond = e->as.conditional;

    String condName = gen_expr(g, cond.condition);
    String resultName = temp_id(e->loc);

    gfprintf(g, "%s %.*s;\n", ktype_to_c(e->type), strf(resultName));
    gfprintf(g, "if (%.*s) {\n", strf(condName));
    String trueVal = gen_expr(g, cond.thenBranch);
    gfprintf(g, "%.*s = %.*s;\n", strf(resultName), strf(trueVal));
    gfprintf(g, "} else {\n");
    String falseVal = gen_expr(g, cond.elseBranch);
    gfprintf(g, "%.*s = %.*s;\n", strf(resultName), strf(falseVal));
    gfprintf(g, "}\n");

    return resultName;
}

static String gen_expr(Generator *g, Expr *e) {
    // minor opt for identifiers
    if (e->kind == EXPR_PRIMARY && e->as.primary.value.kind == TOK_IDENTIFIER) return e->as.primary.value.as.identifier;

    String inner;
    switch (e->kind) {
        case EXPR_PRIMARY:     inner = gen_primary(e);         break;
        case EXPR_GROUPING:    inner = gen_grouping(g, e);     break;
        case EXPR_BINARY:      inner = gen_binary(g, e);       break;
        case EXPR_UNARY:       inner = gen_unary(g, e);        break;
                               // TODO: works because the inner structs are identical
        case EXPR_ASSIGN:      inner = gen_binary(g, e);       break;
        case EXPR_UNARY_POST:  inner = gen_unary_post(g, e);   break;
        case EXPR_CONDITIONAL: return gen_conditional(g, e);
        case EXPR_FUNC_CALL:   inner = gen_func_call(g, e);    break;
    }

    String temp = temp_id(e->loc);
    gfprintf(g, "%s %.*s = %.*s;\n", ktype_to_c(e->type), strf(temp), strf(inner));
    return temp;
}

static void gen_stmt(Generator *g, Stmt *s);

static void gen_func(Generator *g, Stmt *s) {
    FuncStmt func = s->as.func;
    gfprintf(g, "%s %.*s(", ktype_to_c(func.retType), strf(func.name.as.identifier));

    for (size_t i = 0; i < func.params.len; i++) {
        VarStmt param = func.params.arr[i]->as.var;
        gfprintf(g, "%s %.*s", ktype_to_c(param.type), strf(param.name.as.identifier));
        if (i < func.params.len - 1) gfprintf(g, ", ");
    }
    gfprintf(g, ") {\n");

    for (size_t i = 0; i < func.block.len; i++) gen_stmt(g, func.block.arr[i]);

    gfprintf(g, "}\n");
}

static void gen_return(Generator *g, Stmt *s) {
    ReturnStmt ret = s->as.returnS;

    String retVal = str_from_cstr("");
    if (ret.retVal)
        retVal = gen_expr(g, ret.retVal);

    gfprintf(g, "return %.*s;\n", strf(retVal));
}

static void gen_var(Generator *g, Stmt *s) {
    VarStmt var = s->as.var;

    String initVal = str_from_cstr("0");
    if (var.init)
        initVal = gen_expr(g, var.init);

    gfprintf(g, "%s %.*s = %.*s;\n", ktype_to_c(var.type), strf(var.name.as.identifier), strf(initVal));
}

static void gen_stmt(Generator *g, Stmt *s) {
    switch (s->kind) {
        case STMT_BLOCK:
            for (size_t i = 0; i < s->as.block.block.len; i++) gen_stmt(g, s->as.block.block.arr[i]);
            break;
        case STMT_FUNC:     gen_func(g, s); break;
        case STMT_RETURN:   gen_return(g, s); break;
        case STMT_VAR:      gen_var(g, s); break;
        case STMT_EXPR:     gen_expr(g, s->as.expr.expr); break;
        case STMT_WHILE:
        case STMT_IF:
        case STMT_FOR:
        case STMT_BREAK:
        case STMT_CONTINUE: TODO("Generate Statement of kind (%d)", s->kind);
    }
}

void c_codegen(TranslationUnit *unit, FILE *outf) {
    Generator g = (Generator){
        .outf = outf,
        .unit = unit,
    };

    for (size_t i = 0; i < unit->ast.len; i++) {
        gen_stmt(&g, unit->ast.arr[i]);
    }
}
