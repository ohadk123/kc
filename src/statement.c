#include "statement.h"

static Stmt *make_stmt(StmtKind kind, Location loc) {
    Stmt *s = calloc(1, sizeof(Stmt));
    s->kind = kind;
    s->loc = loc;
    return s;
}

Stmt *stmt_make_null(Location loc) {
    Stmt *s = make_stmt(STMT_NULL, loc);
    return s;
}

Stmt *stmt_make_var(String name, Expr *initalizer, TokenKind storage, Location loc) {
    Stmt *s = make_stmt(STMT_VAR, loc);
    s->as.var.name = name;
    s->as.var.init = initalizer;
    s->as.var.specifier = storage;
    return s;
}

Stmt *stmt_make_expr(Expr *inner, Location loc) {
    Stmt *s = make_stmt(STMT_EXPR, loc);
    s->as.expr.expr = inner;
    return s;
}

Stmt *stmt_make_block(StmtList block, Location loc) {
    Stmt *s = make_stmt(STMT_BLOCK, loc);
    s->as.block.block = block;
    return s;
}

Stmt *stmt_make_while(Expr *cond, Stmt *body, Location loc) {
    Stmt *s = make_stmt(STMT_WHILE, loc);
    s->as.whileS.condition = cond;
    s->as.whileS.body = body;
    return s;
}

Stmt *stmt_make_do_while(Expr *cond, Stmt *body, Location loc) {
    Stmt *s = make_stmt(STMT_DO_WHILE, loc);
    s->as.whileS.condition = cond;
    s->as.whileS.body = body;
    return s;
}

Stmt *stmt_make_if(Expr *cond, Stmt *thenBranch, Stmt *elseBranch, Location loc) {
    Stmt *s = make_stmt(STMT_IF, loc);
    s->as.ifS.condition = cond;
    s->as.ifS.thenBranch = thenBranch;
    s->as.ifS.elseBranch = elseBranch;
    return s;
}

Stmt *stmt_make_for(Stmt *init, Expr *cond, Expr *inc, Stmt *body, Location loc) {
    Stmt *s = make_stmt(STMT_FOR, loc);
    s->as.forS.initializer = init;
    s->as.forS.condition = cond;
    s->as.forS.increment = inc;
    s->as.forS.body = body;
    return s;
}

Stmt *stmt_make_break(Location loc) {
    return make_stmt(STMT_BREAK, loc);
}

Stmt *stmt_make_continue(Location loc) {
    return make_stmt(STMT_CONTINUE, loc);
}

Stmt *stmt_make_return(Expr *ret_val, Location loc) {
    Stmt *s = make_stmt(STMT_RETURN, loc);
    s->as.returnS.retVal = ret_val;
    return s;
}

Stmt *stmt_make_func(String name, StmtList params, StmtList block, TokenKind storage, Location loc) {
    Stmt *s = make_stmt(STMT_FUNC, loc);
    s->as.func.name = name;
    s->as.func.params = params;
    s->as.func.body = block;
    s->as.func.specifier = storage;
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
        case STMT_NULL: break;
        case STMT_VAR:
            printf("%*s\"kind\": \"var\",\n", i * 2, "");
            printf("%*s\"name\": \"%.*s\",\n", i * 2, "", strf(stmt->as.var.name));
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
            print_expr(stmt->as.whileS.condition, i);
            printf(",\n");
            printf("%*s\"body\": ", i * 2, "");
            print_stmt(stmt->as.whileS.body, i);
            printf("\n");
            break;
        case STMT_DO_WHILE:
            printf("%*s\"kind\": \"do/while\",\n", i * 2, "");
            printf("%*s\"cond\": ", i * 2, "");
            print_expr(stmt->as.whileS.condition, i);
            printf(",\n");
            printf("%*s\"body\": ", i * 2, "");
            print_stmt(stmt->as.whileS.body, i);
            printf("\n");
            break;
        case STMT_IF:
            printf("%*s\"kind\": \"if\",\n", i * 2, "");
            printf("%*s\"cond\": ", i * 2, "");
            print_expr(stmt->as.ifS.condition, i);
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
        case STMT_RETURN:
            printf("%*s\"kind\": \"return\",\n", i * 2, "");
            printf("%*s\"expr\": ", i * 2, "");
            print_expr(stmt->as.returnS.retVal, i);
            printf("\n");
            break;
        case STMT_FUNC: {
            FuncStmt *fn = &stmt->as.func;
            printf("%*s\"kind\": \"func\",\n", i * 2, "");
            printf("%*s\"name\": \"%.*s\",\n", i * 2, "", strf(fn->name));
            printf("%*s\"params\": [", i * 2, "");
            for (size_t j = 0; j < fn->params.len; j++) {
                VarStmt *par = &fn->params.arr[j]->as.var;
                printf("\n%*s{ \"name\": \"%.*s\" }", (i + 1) * 2, "", strf(par->name));
                if (j + 1 < fn->params.len) printf(",");
            }
            if (fn->params.len > 0) printf("\n%*s", i * 2, "");
            printf("],\n");
            printf("%*s\"body\": [\n", i * 2, "");
            for (size_t j = 0; j < fn->body.len; j++) {
                printf("%*s", (i + 1) * 2, "");
                print_stmt(fn->body.arr[j], i + 1);
                if (j + 1 < fn->body.len) printf(",");
                printf("\n");
            }
            printf("%*s]\n", i * 2, "");
            break;
        }
    }
    printf("%*s}", indent * 2, "");
}

String get_top_level_name(Stmt *s) {
    switch (s->kind) {
        case STMT_FUNC: return s->as.func.name;
        case STMT_NULL:
        case STMT_VAR: return s->as.var.name;
        case STMT_EXPR:
        case STMT_BLOCK:
        case STMT_WHILE:
        case STMT_DO_WHILE:
        case STMT_IF:
        case STMT_FOR:
        case STMT_RETURN:
        case STMT_BREAK:
        case STMT_CONTINUE: break;
    }
    UNREACHABLE("Not a top level declaration");
}
