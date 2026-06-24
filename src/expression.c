#include "expression.h"

static Expr *make_expr(ExprKind kind, Location loc) {
    Expr *e = calloc(1, sizeof(Expr));
    e->kind = kind;
    e->loc = loc;
    return e;
}

static PrimaryValue token_to_primary(Token token) {
    PrimaryValue p = {0};
    p.kind = (PrimaryKind) token.kind;

    switch (p.kind) {
        case PRIM_IDENTIFIER: p.as.identifier = token.as.identifier; return p;
        case PRIM_STRING_LITERAL: p.as.stringLiteral = token.as.stringLiteral; return p;
        case PRIM_CHAR_LITERAL: p.as.charLiteral = token.as.charLiteral; return p;
        case PRIM_INTEGER_LITERAL: p.as.integerLiteral = token.as.integerLiteral; return p;
        case PRIM_LONG_LITERAL: p.as.longLiteral = token.as.longLiteral; return p;
        case PRIM_FLOAT_LITERAL: p.as.floatLiteral = token.as.floatLiteral; return p;
        case PRIM_DOUBLE_LITERAL: p.as.doubleLiteral = token.as.doubleLiteral; return p;
        case PRIM_TRUE:
        case PRIM_FALSE: return p; // Do Nothing
    }

    UNREACHABLE("Invalid Primary Kind (%s)", tokenTypesStrings[token.kind]);
}

Expr *expr_make_primary(Token val, Location loc) {
    Expr *e = make_expr(EXPR_PRIMARY, loc);
    e->as.primary.value = token_to_primary(val);
    e->as.primary.decl = NULL;
    return e;
}

Expr *expr_make_grouping(Expr *inner, Location loc) {
    Expr *e = make_expr(EXPR_GROUPING, loc);
    e->as.grouping.inner = inner;
    return e;
}

Expr *expr_make_binary(BinOp op, Expr *lhs, Expr *rhs, Location loc) {
    Expr *e = make_expr(EXPR_BINARY, loc);
    e->as.binary.op = op;
    e->as.binary.lhs = lhs;
    e->as.binary.rhs = rhs;
    return e;
}

Expr *expr_make_unary(UnaryOp op, Expr *inner, Location loc) {
    Expr *e = make_expr(EXPR_UNARY, loc);
    e->as.unary.op = op;
    e->as.unary.inner = inner;
    return e;
}

Expr *expr_make_unary_post(UnaryOp op, Expr *inner, Location loc) {
    Expr *e = make_expr(EXPR_UNARY_POST, loc);
    e->as.unary.op = op;
    e->as.unary.inner = inner;
    return e;
}

Expr *expr_make_assign(AssignOp op, Expr *lhs, Expr *rhs, Location loc) {
    Expr *e = make_expr(EXPR_ASSIGN, loc);
    e->as.assignment.op = op;
    e->as.assignment.lhs = lhs;
    e->as.assignment.rhs = rhs;
    return e;
}

Expr *expr_make_conditional(Expr *cond, Expr *thenBranch, Expr *elseBranch, Location loc) {
    Expr *e = make_expr(EXPR_CONDITIONAL, loc);
    e->as.conditional.condition = cond;
    e->as.conditional.thenBranch = thenBranch;
    e->as.conditional.elseBranch = elseBranch;
    return e;
}

Expr *expr_make_func_call(Expr *func, ExprList args, Location loc) {
    Expr *e = make_expr(EXPR_FUNC_CALL, loc);
    e->as.funcCall.func = func;
    e->as.funcCall.args = args;
    return e;
}

Expr *expr_make_index(Expr *array, Expr *index, Location loc) {
    Expr *e = make_expr(EXPR_INDEX, loc);
    e->as.index.array = array;
    e->as.index.index = index;
    return e;
}

Expr *expr_make_cast(TokenKind type, Expr *inner, Location loc) {
    Expr *e = make_expr(EXPR_CAST, loc);
    e->as.cast.type = type;
    e->as.cast.inner = inner;
    return e;
}

static const char *op_str(TokenKind op) {
    switch (op) {
        case TOK_PLUS:                   return "+";
        case TOK_MINUS:                  return "-";
        case TOK_STAR:                   return "*";
        case TOK_SLASH:                  return "/";
        case TOK_PERCENT:                return "%";
        case TOK_AMPERSAND:              return "&";
        case TOK_PIPE:                   return "|";
        case TOK_CARET:                  return "^";
        case TOK_TILDE:                  return "~";
        case TOK_LESS_LESS:              return "<<";
        case TOK_GREATER_GREATER:        return ">>";
        case TOK_EQUALS_EQUALS:          return "==";
        case TOK_BANG_EQUALS:            return "!=";
        case TOK_LESS:                   return "<";
        case TOK_LESS_EQUALS:            return "<=";
        case TOK_GREATER:                return ">";
        case TOK_GREATER_EQUALS:         return ">=";
        case TOK_AMPERSAND_AMPERSAND:    return "&&";
        case TOK_PIPE_PIPE:              return "||";
        case TOK_BANG:                   return "!";
        case TOK_PLUS_PLUS:              return "++";
        case TOK_MINUS_MINUS:            return "--";
        case TOK_EQUALS:                 return "=";
        case TOK_PLUS_EQUALS:            return "+=";
        case TOK_MINUS_EQUALS:           return "-=";
        case TOK_STAR_EQUALS:            return "*=";
        case TOK_SLASH_EQUALS:           return "/=";
        case TOK_PERCENT_EQUALS:         return "%=";
        case TOK_AMPERSAND_EQUALS:       return "&=";
        case TOK_PIPE_EQUALS:            return "|=";
        case TOK_CARET_EQUALS:           return "^=";
        case TOK_LESS_LESS_EQUALS:       return "<<=";
        case TOK_GREATER_GREATER_EQUALS: return ">>=";
        default:                         return "?";
    }
}

void print_expr(Expr *expr, int indent) {
    if (!expr) {
        printf("null");
        return;
    }
    printf("{\n");
    int i = indent + 1;

    switch (expr->kind) {
        case EXPR_PRIMARY: {
            PrimaryValue v = expr->as.primary.value;
            printf("%*s\"kind\": \"primary\",\n", i * 2, "");
            printf("%*s\"value\": ", i * 2, "");
            switch (v.kind) {
                case PRIM_INTEGER_LITERAL: printf("%u", v.as.integerLiteral); break;
                case PRIM_LONG_LITERAL:    printf("%lu", v.as.longLiteral); break;
                case PRIM_FLOAT_LITERAL:   printf("%f", v.as.floatLiteral); break;
                case PRIM_DOUBLE_LITERAL:  printf("%f", v.as.doubleLiteral); break;
                case PRIM_STRING_LITERAL:  printf("\"%.*s\"", (int)v.as.stringLiteral.len, v.as.stringLiteral.data); break;
                case PRIM_CHAR_LITERAL:    printf("'%c'", v.as.charLiteral); break;
                case PRIM_IDENTIFIER:      printf("\"%.*s\"", (int)v.as.identifier.len, v.as.identifier.data); break;
                case PRIM_TRUE:            printf("true"); break;
                case PRIM_FALSE:           printf("false"); break;
                default:                  printf("\"?\""); break;
            }
            printf("\n");
            break;
        }
        case EXPR_GROUPING:
            printf("%*s\"kind\": \"grouping\",\n", i * 2, "");
            printf("%*s\"inner\": ", i * 2, "");
            print_expr(expr->as.grouping.inner, i);
            printf("\n");
            break;
        case EXPR_BINARY:
            printf("%*s\"kind\": \"binary\",\n", i * 2, "");
            printf("%*s\"op\": \"%s\",\n", i * 2, "", op_str((TokenKind) expr->as.binary.op));
            printf("%*s\"lhs\": ", i * 2, "");
            print_expr(expr->as.binary.lhs, i);
            printf(",\n");
            printf("%*s\"rhs\": ", i * 2, "");
            print_expr(expr->as.binary.rhs, i);
            printf("\n");
            break;
        case EXPR_UNARY:
            printf("%*s\"kind\": \"unary\",\n", i * 2, "");
            printf("%*s\"op\": \"%s\",\n", i * 2, "", op_str((TokenKind) expr->as.unary.op));
            printf("%*s\"inner\": ", i * 2, "");
            print_expr(expr->as.unary.inner, i);
            printf("\n");
            break;
        case EXPR_ASSIGN:
            printf("%*s\"kind\": \"assignment\",\n", i * 2, "");
            printf("%*s\"op\": \"%s\",\n", i * 2, "", op_str((TokenKind) expr->as.assignment.op));
            printf("%*s\"lhs\": ", i * 2, "");
            print_expr(expr->as.assignment.lhs, i);
            printf(",\n");
            printf("%*s\"rhs\": ", i * 2, "");
            print_expr(expr->as.assignment.rhs, i);
            printf("\n");
            break;
        case EXPR_UNARY_POST:
            printf("%*s\"kind\": \"unary_post\",\n", i * 2, "");
            printf("%*s\"op\": \"%s\",\n", i * 2, "", op_str((TokenKind) expr->as.unary.op));
            printf("%*s\"inner\": ", i * 2, "");
            print_expr(expr->as.unary.inner, i);
            printf("\n");
            break;
        case EXPR_CONDITIONAL:
            printf("%*s\"kind\": \"conditional\",\n", i * 2, "");
            printf("%*s\"cond\": ", i * 2, "");
            print_expr(expr->as.conditional.condition, i);
            printf(",\n");
            printf("%*s\"then\": ", i * 2, "");
            print_expr(expr->as.conditional.thenBranch, i);
            printf(",\n");
            printf("%*s\"else\": ", i * 2, "");
            print_expr(expr->as.conditional.elseBranch, i);
            printf("\n");
            break;
        case EXPR_FUNC_CALL: {
            FuncCallExpr *fc = &expr->as.funcCall;
            printf("%*s\"kind\": \"func_call\",\n", i * 2, "");
            printf("%*s\"func\": ", i * 2, "");
            print_expr(fc->func, i);
            printf(",\n");
            printf("%*s\"args\": [", i * 2, "");
            for (size_t j = 0; j < fc->args.len; j++) {
                printf("\n%*s", (i + 1) * 2, "");
                print_expr(fc->args.arr[j], i + 1);
                if (j + 1 < fc->args.len) printf(",");
            }
            if (fc->args.len > 0) printf("\n%*s", i * 2, "");
            printf("]\n");
            break;
        }
        case EXPR_INDEX: {
            IndexExpr *idx = &expr->as.index;
            printf("%*s\"kind\": \"index\",\n", i * 2, "");
            printf("%*s\"array\": ", i * 2, "");
            print_expr(idx->array, i);
            printf(",\n");
            printf("%*s\"index\": ", i * 2, "");
            print_expr(idx->index, i);
            printf("\n");
            break;
        }

        case EXPR_CAST: {
            CastExpr *c = &expr->as.cast;
            printf("%*s\"kind\": \"cast\",\n", i * 2, "");
            printf("%*s\"type\": \"%s\"", i * 2, "", tokenTypesStrings[c->type]);
            printf(",\n");
            printf("%*s\"inner\": ", i * 2, "");
            print_expr(c->inner, i);
            printf("\n");
            break;
        }
    }
    printf("%*s}", indent * 2, "");
}
