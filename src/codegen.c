#include "codegen.h"
#include "compiler.h"

void gen_stmt(Scope *currScope, Stmt *s) {
    assert(currScope && s);
    TODO("%s", __func__);
}

const char *ktype_to_qbetype(TokenKind ktype) {
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

void gen_func(TranslationUnit *context, FuncStmt *func) {
    String funcName = func->name.as.identifier;
    printf("function %s $%.*s(", ktype_to_qbetype(func->retType), (int)funcName.len, funcName.data);

    Scope funcScope = {0};
    funcScope.above = &context->globalSymbolTable;
    for (size_t i = 0; i < func->params.len; i++) {
        Stmt *param = func->params.arr[i];
        String name = param->as.var.name.as.identifier;
        Stmt *found = get_or_insert_symbol(&funcScope, name, param);
        if (!found) {
            compile_error(context->fileName, param->loc,
                          "Multiple declarations of variable \"%.*s\", already declared at [%.*s:%zu:%zu]",
                          (int)name.len, name.data, (int)context->fileName.len, context->fileName.data, found->loc.line,
                          found->loc.col);
        }

        printf("%s %.*s, ", ktype_to_qbetype(param->as.var.type), (int) name.len, name.data);
    }
    printf (") {\n");
    printf("@start\n");

    for (size_t i = 0; i < func->block.len; i++) {
        gen_stmt(&funcScope, func->block.arr[i]);
    }

    printf("}\n");
}
