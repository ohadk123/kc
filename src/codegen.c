#include "codegen.h"
#include "compiler.h"
#include <stdarg.h>

static String get_temp_id(Location loc) {
    static size_t next = 0;
    return str_printf("_k%zu_%zu_%zu", next, loc.line, loc.col);
}

static void gen(Generator *g, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(g->output, fmt, args);
    va_end(args);
    fprintf(g->output, "\n");
}

static const char *ktype_to_qbetype(TokenKind ktype) {
    switch (ktype) {
        case TOK_VOID:  return "";
        case TOK_U8:    return "ub";
        case TOK_U16:   return "uh";
        case TOK_U32:   return "w";
        case TOK_U64:   return "l";
        case TOK_USIZE: return "l";
        case TOK_I8:    return "b";
        case TOK_I16:   return "h";
        case TOK_I32:   return "w";
        case TOK_I64:   return "l";
        case TOK_ISIZE: return "l";
        case TOK_F32:   return "s";
        case TOK_F64:   return "d";
        case TOK_BOOL:  return "ub";
        default:        UNREACHABLE("Not a valid type (%d)", ktype);
    }
}

static const char *binop_to_qbe_op(TokenKind op) {
    switch (op) {
        case TOK_STAR:                   return "mul";
        case TOK_SLASH:                  return "div";
        case TOK_PERCENT:                return "rem";
        case TOK_PLUS:                   return "add";
        case TOK_MINUS:                  return "sub";
        case TOK_LESS_LESS:              return "shr";
        case TOK_GREATER_GREATER:        return "shl";
        case TOK_LESS:                   return "cslt";
        case TOK_GREATER:                return "csgt";
        case TOK_LESS_EQUALS:            return "csle";
        case TOK_GREATER_EQUALS:         return "csge";
        case TOK_EQUALS_EQUALS:          return "ceq";
        case TOK_BANG_EQUALS:            return "cne";
        case TOK_AMPERSAND:              return "and";
        case TOK_CARET:                  return "xor";
        case TOK_PIPE:                   return "or";
        case TOK_AMPERSAND_AMPERSAND:    TODO("%s: TOK_AMPERSAND_AMPERSAND", __func__);
        case TOK_PIPE_PIPE:              TODO("%s: TOK_PIPE_PIPE", __func__);
        case TOK_EQUALS:                 TODO("%s: TOK_EQUALS_EQUALS", __func__);
        case TOK_PLUS_EQUALS:            return "add";
        case TOK_MINUS_EQUALS:           return "sub";
        case TOK_STAR_EQUALS:            return "mul";
        case TOK_SLASH_EQUALS:           return "div";
        case TOK_PERCENT_EQUALS:         return "rem";
        case TOK_AMPERSAND_EQUALS:       return "and";
        case TOK_CARET_EQUALS:           return "xor";
        case TOK_PIPE_EQUALS:            return "or";
        case TOK_LESS_LESS_EQUALS:       return "shr";
        case TOK_GREATER_GREATER_EQUALS: return "shl";
        default:                         UNREACHABLE("Unkown binary op (%d)", op);
    }
}

static void check_lvalue(Generator *g, Expr *e) {
    if (e->kind == EXPR_PRIMARY && e->as.primary.value.kind == TOK_IDENTIFIER) return;
    if (e->kind == EXPR_GROUPING) {
        check_lvalue(e->as.grouping.inner);
        return;
    }
    oif (e->kind == EXPR_UNARY && e->as.unary.op == TOK_STAR) return;

    compile_error(g->unit.fileName, e->as)
}

static String gen_expr(Generator *g, Expr *e) {
    String temp, ident, temp_lhs, temp_rhs;
    const char *op;
    switch (e->kind) {
        case EXPR_PRIMARY:
            temp = get_temp_id(e->as.primary.value.loc);
            switch (e->as.primary.value.kind) {
                case TOK_IDENTIFIER:      ident = e->as.primary.value.as.identifier;
                case TOK_STRING_LITERAL:  TODO("String literal codegen");
                case TOK_CHAR_LITERAL:    ident = str_printf("%u", e->as.primary.value.as.charLiteral);
                case TOK_INTEGER_LITERAL: ident = str_printf("%u", e->as.primary.value.as.integerLiteral);
                case TOK_FLOAT_LITERAL:   ident = str_printf("d_%u", e->as.primary.value.as.floatLiteral);
                default:                  UNREACHABLE("Not a primary token kind (%d)", e->as.primary.value.kind);
            }
            gen(g, "%.*s =w copy %%%.*s", (int)temp.len, temp.data, (int)ident.len, ident.data);
            return temp;
        case EXPR_GROUPING: return gen_expr(g, e->as.grouping.inner);
        case EXPR_BINARY:
            temp = get_temp_id((Location){0, 0});
            temp_lhs = gen_expr(g, e->as.binary.lhs);
            temp_rhs = gen_expr(g, e->as.binary.rhs);
            op = binop_to_qbe_op(e->as.binary.op);
            gen(g, "%.*s =w %s %.*s, %.*s", strf(temp), op, strf(temp_lhs), strf(temp_rhs));
            return temp;
        case EXPR_UNARY:
            temp = get_temp_id((Location){0, 0});
            String inner = gen_expr(g, e->as.unary.inner);
            switch (e->as.unary.op) {
                case TOK_PLUS_PLUS:   gen(g, "%.*s =w add %.*s, 1", strf(temp), strf(inner)); return temp;
                case TOK_MINUS_MINUS: gen(g, "%.*s =w sub %.*s, 1", strf(temp), strf(inner)); return temp;
                case TOK_AMPERSAND:   TODO("Unary ampersand codegen");
                case TOK_STAR:        gen(g, "%.*s =w loadw %.*s", strf(temp), strf(inner)); return temp;
                case TOK_PLUS:        gen(g, "%.*s =w add %.*s, 0", strf(temp), strf(inner)); return temp;
                case TOK_MINUS:       gen(g, "%.*s =w neg %.*s", strf(temp), strf(inner)); return temp;
                case TOK_TILDE:       gen(g, "%.*s =w xor %.*s, -1", strf(temp), strf(inner)); return temp;
                case TOK_BANG:        gen(g, "%.*s =w ceq %.*s, 0", strf(temp), strf(inner)); return temp;
                default:              UNREACHABLE("Unkown unary op (%d)", e->as.unary.op);
            }
        case EXPR_UNARY_POST:
        case EXPR_ASSIGN:
            check_lvalue(e);
            temp = get_temp_id((Location){0, 0});
            temp_lhs = gen_expr(g, e->as.binary.lhs);
            temp_rhs = gen_expr(g, e->as.binary.rhs);
            op = binop_to_qbe_op(e->as.binary.op);
            gen(g, "%.*s =w %s %.*s, %.*s", strf(temp), op, strf(temp_lhs), strf(temp_rhs));
            return temp;
        case EXPR_CONDITIONAL:
        case EXPR_FUNC_CALL:   break;
    }
    UNREACHABLE("Unkown expression kind (%d)", e->kind);
}

static void gen_stmt(Scope *currScope, Stmt *s) {
    assert(currScope && s);
    TODO("%s", __func__);
}

static void gen_func(Generator *g, FuncStmt func) {
    String funcName = func.name.as.identifier;
    printf("function %s $%.*s(", ktype_to_qbetype(func.retType), (int)funcName.len, funcName.data);

    Scope funcScope = {0};
    funcScope.above = &g->unit.globalSymbolTable;
    for (size_t i = 0; i < func.params.len; i++) {
        Stmt *param = func.params.arr[i];
        String name = param->as.var.name.as.identifier;
        Stmt *found = get_or_insert_symbol(&funcScope, name, param);
        if (!found) {
            compile_error(g->unit.fileName, param->loc,
                          "Multiple declarations of variable \"%.*s\", already declared at [%.*s:%zu:%zu]",
                          (int)name.len, name.data, (int)g->unit.fileName.len, g->unit.fileName.data, found->loc.line,
                          found->loc.col);
        }

        printf("%s %.*s, ", ktype_to_qbetype(param->as.var.type), (int)name.len, name.data);
    }
    printf(") {\n");
    printf("@start\n");

    for (size_t i = 0; i < func.block.len; i++) {
        gen_stmt(&funcScope, func.block.arr[i]);
    }

    printf("}\n");
}

void gen_translation_unit(TranslationUnit unit) {
    Generator g = {
        .unit = unit,
        .currScope = unit.globalSymbolTable,
        .output = stdout,
    };

    StmtList ast = unit.ast;
    for (size_t i = 0; i < ast.len; i++) {
        Stmt *top_level_decl = ast.arr[i];
        switch (top_level_decl->kind) {
            case STMT_FUNC: gen_func(&g, top_level_decl->as.func); break;
            default:        UNREACHABLE("Unkown top level decl (%d)", top_level_decl->kind);
        }
    }
}
