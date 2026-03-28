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
