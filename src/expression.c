#include "expression.h"
#include "type.h"

static Expr *make_expr(ExprKind kind, Location loc) {
    Expr *e = malloc(sizeof(Expr));
    e->kind = kind;
    e->loc = loc;
    return e;
}

Expr *expr_make_primary(Token val, Location loc) {
    Expr *e = make_expr(EXPR_PRIMARY, loc);
    e->as.primary.value = val;
    return e;
}

Expr *expr_make_grouping(Expr *inner, Location loc) {
    Expr *e = make_expr(EXPR_GROUPING, loc);
    e->as.grouping.inner = inner;
    return e;
}

Expr *expr_make_binary(TokenKind op, Expr *lhs, Expr *rhs, Location loc) {
    Expr *e = make_expr(EXPR_BINARY, loc);
    e->as.binary.op = op;
    e->as.binary.lhs = lhs;
    e->as.binary.rhs = rhs;
    return e;
}

Expr *expr_make_unary(TokenKind op, Expr *inner, Location loc) {
    Expr *e = make_expr(EXPR_UNARY, loc);
    e->as.unary.op = op;
    e->as.unary.inner = inner;
    return e;
}

Expr *expr_make_unary_post(TokenKind op, Expr *inner, Location loc) {
    Expr *e = make_expr(EXPR_UNARY_POST, loc);
    e->as.unary.op = op;
    e->as.unary.inner = inner;
    return e;
}

Expr *expr_make_assign(TokenKind op, Expr *lhs, Expr *rhs, Location loc) {
    Expr *e = make_expr(EXPR_ASSIGN, loc);
    e->as.binary.op = op;
    e->as.binary.lhs = lhs;
    e->as.binary.rhs = rhs;
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

    // if (expr->type) {
    //     String ts = type_to_string(expr->type);
    //     printf("%*s\"type\": \"%.*s\",\n", i * 2, "", strf(ts));
    // }

    switch (expr->kind) {
        case EXPR_PRIMARY: {
            Token v = expr->as.primary.value;
            printf("%*s\"kind\": \"primary\",\n", i * 2, "");
            printf("%*s\"value\": ", i * 2, "");
            switch (v.kind) {
                case TOK_INTEGER_LITERAL: printf("%zu", v.as.integerLiteral); break;
                case TOK_FLOAT_LITERAL:   printf("%g", v.as.floatLiteral); break;
                case TOK_STRING_LITERAL:  printf("\"%.*s\"", (int)v.as.stringLiteral.len, v.as.stringLiteral.data); break;
                case TOK_CHAR_LITERAL:    printf("'%c'", v.as.charLiteral); break;
                case TOK_IDENTIFIER:      printf("\"%.*s\"", (int)v.as.identifier.len, v.as.identifier.data); break;
                case TOK_TRUE:            printf("true"); break;
                case TOK_FALSE:           printf("false"); break;
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
            printf("%*s\"op\": \"%s\",\n", i * 2, "", op_str(expr->as.binary.op));
            printf("%*s\"lhs\": ", i * 2, "");
            print_expr(expr->as.binary.lhs, i);
            printf(",\n");
            printf("%*s\"rhs\": ", i * 2, "");
            print_expr(expr->as.binary.rhs, i);
            printf("\n");
            break;
        case EXPR_UNARY:
            printf("%*s\"kind\": \"unary\",\n", i * 2, "");
            printf("%*s\"op\": \"%s\",\n", i * 2, "", op_str(expr->as.unary.op));
            printf("%*s\"inner\": ", i * 2, "");
            print_expr(expr->as.unary.inner, i);
            printf("\n");
            break;
        case EXPR_ASSIGN:
            printf("%*s\"kind\": \"assignment\",\n", i * 2, "");
            printf("%*s\"op\": \"%s\",\n", i * 2, "", op_str(expr->as.assignment.op));
            printf("%*s\"lhs\": ", i * 2, "");
            print_expr(expr->as.assignment.lhs, i);
            printf(",\n");
            printf("%*s\"rhs\": ", i * 2, "");
            print_expr(expr->as.assignment.rhs, i);
            printf("\n");
            break;
        case EXPR_UNARY_POST:
            printf("%*s\"kind\": \"unary_post\",\n", i * 2, "");
            printf("%*s\"op\": \"%s\",\n", i * 2, "", op_str(expr->as.unary.op));
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
    }
    printf("%*s}", indent * 2, "");
}
