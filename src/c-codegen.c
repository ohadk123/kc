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

static String gen_primary(Generator *g, Expr *e) {
    Token primary = e->as.primary.value;
    String value = {0};

    switch (primary.kind) {
        case TOK_IDENTIFIER:      return str_printf("%.*s", strf(primary.as.identifier));
        case TOK_CHAR_LITERAL:    value = str_printf("%u", primary.as.charLiteral); break;
        case TOK_INTEGER_LITERAL: value = str_printf("%lu", primary.as.integerLiteral); break;
        case TOK_FLOAT_LITERAL:   value = str_printf("%f", primary.as.floatLiteral); break;
        case TOK_TRUE:            value = str_from_cstr("1"); break;
        case TOK_FALSE:           value = str_from_cstr("0"); break;
        case TOK_STRING_LITERAL:  value = str_printf("\"%.*s\"", strf(primary.as.stringLiteral)); break;
        default:                  UNREACHABLE("Not a primary kind (%d)", primary.kind);
    }
    String temp = temp_id(e->loc);
    gfprintf(g, "%s %.*s = %.*s;\n", ktype_to_c(e->type), strf(temp), strf(value));
    return temp;
}

static String gen_expr(Generator *g, Expr *e) {
    switch (e->kind) {
        case EXPR_PRIMARY:     return gen_primary(g, e);
        case EXPR_GROUPING:
        case EXPR_BINARY:
        case EXPR_UNARY:
        case EXPR_ASSIGN:
        case EXPR_UNARY_POST:
        case EXPR_CONDITIONAL:
        case EXPR_FUNC_CALL:   TODO("Generate expression of kind (%d)", e->kind);
    }
    UNREACHABLE("Error on expr kind (%d)", e->kind);
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

static void gen_stmt(Generator *g, Stmt *s) {
    switch (s->kind) {
        case STMT_BLOCK:
            for (size_t i = 0; i < s->as.block.block.len; i++) gen_stmt(g, s->as.block.block.arr[i]);
            break;
        case STMT_FUNC:     gen_func(g, s); break;
        case STMT_RETURN:   gen_return(g, s); break;
        case STMT_VAR:
        case STMT_EXPR:
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
