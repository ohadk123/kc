#include "statement.h"

static Stmt *make_stmt(StmtKind kind) {
    Stmt *s = malloc(sizeof(Stmt));
    s->kind = kind;
    return s;
}

Stmt *stmt_make_var(TokenKind type, Token name, Expr *initalizer) {
    Stmt *s = make_stmt(STMT_VAR);
    s->as.var.type = type;
    s->as.var.name = name;
    s->as.var.init = initalizer;
    return s;
}

Stmt *stmt_make_expr(Expr *inner) {
    Stmt *s = make_stmt(STMT_EXPR);
    s->as.expr.expr = inner;
    return s;
}

Stmt *stmt_make_block(StmtList block) {
    Stmt *s = make_stmt(STMT_BLOCK);
    s->as.block.block = block;
    return s;
}

Stmt *stmt_make_while(Expr *cond, Stmt *body) {
    Stmt *s = make_stmt(STMT_WHILE);
    s->as.whileS.cond = cond;
    s->as.whileS.body = body;
    return s;
}

Stmt *stmt_make_if(Expr *cond, Stmt *thenBranch, Stmt *elseBranch) {
    Stmt *s = make_stmt(STMT_IF);
    s->as.ifS.cond = cond;
    s->as.ifS.thenBranch = thenBranch;
    s->as.ifS.elseBranch = elseBranch;
    return s;
}

Stmt *stmt_make_for(Stmt *init, Expr *cond, Expr *inc, Stmt *body) {
    Stmt *s = make_stmt(STMT_FOR);
    s->as.forS.initializer = init;
    s->as.forS.condition = cond;
    s->as.forS.increment = inc;
    s->as.forS.body = body;
    return s;
}

Stmt *stmt_make_break(void) {
    return make_stmt(STMT_BREAK);
}

Stmt *stmt_make_continue(void) {
    return make_stmt(STMT_CONTINUE);
}

static const char *type_str(TokenKind type) {
    switch (type) {
        case TOK_VOID:  return "void";
        case TOK_U8:    return "u8";
        case TOK_U16:   return "u16";
        case TOK_U32:   return "u32";
        case TOK_U64:   return "u64";
        case TOK_USIZE: return "usize";
        case TOK_I8:    return "i8";
        case TOK_I16:   return "i16";
        case TOK_I32:   return "i32";
        case TOK_I64:   return "i64";
        case TOK_ISIZE: return "isize";
        case TOK_F32:   return "f32";
        case TOK_F64:   return "f64";
        case TOK_BOOL:  return "bool";
        default:        return "?";
    }
}

Stmt *stmt_make_return(Expr *ret_val) {
    Stmt *s = make_stmt(STMT_RETURN);
    s->as.returnS.ret_val = ret_val;
    return s;
}

void print_stmt(Stmt *stmt, int indent) {
    if (!stmt) {
        printf("null");
        return;
    }
    printf("{\n");
    int i = indent + 1;
    switch (stmt->kind) {
        case STMT_VAR:
            printf("%*s\"kind\": \"var\",\n", i * 2, "");
            printf("%*s\"type\": \"%s\",\n", i * 2, "", type_str(stmt->as.var.type));
            printf("%*s\"name\": \"%.*s\",\n", i * 2, "", (int)stmt->as.var.name.as.identifier.len, stmt->as.var.name.as.identifier.data);
            printf("%*s\"init\": ", i * 2, "");
            print_expr(stmt->as.var.init, i);
            printf("\n");
            break;
        case STMT_EXPR:
            printf("%*s\"kind\": \"expr\",\n", i * 2, "");
            printf("%*s\"expr\": ", i * 2, "");
            print_expr(stmt->as.expr.expr, i);
            printf("\n");
            break;
        case STMT_BLOCK: {
            printf("%*s\"kind\": \"block\",\n", i * 2, "");
            printf("%*s\"body\": [\n", i * 2, "");
            StmtList block = stmt->as.block.block;
            for (size_t j = 0; j < block.len; j++) {
                printf("%*s", (i + 1) * 2, "");
                print_stmt(block.arr[j], i + 1);
                if (j + 1 < block.len) printf(",");
                printf("\n");
            }
            printf("%*s]\n", i * 2, "");
            break;
        }
        case STMT_WHILE:
            printf("%*s\"kind\": \"while\",\n", i * 2, "");
            printf("%*s\"cond\": ", i * 2, "");
            print_expr(stmt->as.whileS.cond, i);
            printf(",\n");
            printf("%*s\"body\": ", i * 2, "");
            print_stmt(stmt->as.whileS.body, i);
            printf("\n");
            break;
        case STMT_IF:
            printf("%*s\"kind\": \"if\",\n", i * 2, "");
            printf("%*s\"cond\": ", i * 2, "");
            print_expr(stmt->as.ifS.cond, i);
            printf(",\n");
            printf("%*s\"then\": ", i * 2, "");
            print_stmt(stmt->as.ifS.thenBranch, i);
            if (stmt->as.ifS.elseBranch) {
                printf(",\n");
                printf("%*s\"else\": ", i * 2, "");
                print_stmt(stmt->as.ifS.elseBranch, i);
            }
            printf("\n");
            break;
        case STMT_FOR:
            printf("%*s\"kind\": \"for\",\n", i * 2, "");
            printf("%*s\"init\": ", i * 2, "");
            print_stmt(stmt->as.forS.initializer, i);
            printf(",\n");
            printf("%*s\"cond\": ", i * 2, "");
            print_expr(stmt->as.forS.condition, i);
            printf(",\n");
            printf("%*s\"incr\": ", i * 2, "");
            print_expr(stmt->as.forS.increment, i);
            printf(",\n");
            printf("%*s\"body\": ", i * 2, "");
            print_stmt(stmt->as.forS.body, i);
            printf("\n");
            break;
        case STMT_BREAK:
            printf("%*s\"kind\": \"break\"\n", i * 2, "");
            break;
        case STMT_CONTINUE:
            printf("%*s\"kind\": \"continue\"\n", i * 2, "");
            break;
    }
    printf("%*s}", indent * 2, "");
}
