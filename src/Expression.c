#include "Expression.h"

#include <libk/Errors.h>

Expr *makePrimaryExpr(Token value) {
    Expr *e = malloc(sizeof(Expr));
    e->type = EXPR_LITERAL;
    e->as.primary.value = value;
    return e;
}

Expr *makeGroupingExpr(Expr *inner) {
    Expr *e = malloc(sizeof(Expr));
    e->type = EXPR_GROUPING;
    e->as.grouping.inner = inner;
    return e;
}

Expr *makeBinaryExpr(TokenType op, Expr *lhs, Expr *rhs) {
    Expr *e = malloc(sizeof(Expr));
    e->type = EXPR_BINARY;
    e->as.binary.op = op;
    e->as.binary.lhs = lhs;
    e->as.binary.rhs = rhs;
    return e;
}

Expr *makeUnaryExpr(TokenType op, Expr *inner) {
    Expr *e = malloc(sizeof(Expr));
    e->type = EXPR_UNARY;
    e->as.unary.op = op;
    e->as.unary.inner = inner;
    return e;
}

Expr *makeConditionalExpr(Expr *condition, Expr *thenBranch, Expr *elseBranch) {
    Expr *e = malloc(sizeof(Expr));
    e->type = EXPR_CONDITIONAL;
    e->as.conditional.condition = condition;
    e->as.conditional.thenBranch = thenBranch;
    e->as.conditional.elseBranch = elseBranch;
    return e;
}

Expr *makeIndexExpr(Expr *name, Expr *index) {
    Expr *e = malloc(sizeof(Expr));
    e->type = EXPR_INDEX;
    e->as.index.name = name;
    e->as.index.index = index;
    return e;
}

Expr *makeFuncCallExpr(Expr *name, ArgsList args) {
    Expr *e = malloc(sizeof(Expr));
    e->type = EXPR_FUNC_CALL;
    e->as.funcCall.callee = name;
    e->as.funcCall.args = args;
    return e;
}

Expr *makeMemberExpr(TokenType op, Expr *object, Token member) {
    Expr *e = malloc(sizeof(Expr));
    e->type = EXPR_MEMBER;
    e->as.member.op = op;
    e->as.member.object = object;
    e->as.member.member = member;
    return e;
}

#define EVAL_BINARY(TOK, op)                                                                                           \
    case TOK:                                                                                                          \
        lhs = evalExpr(root->as.binary.lhs);                                                                           \
        rhs = evalExpr(root->as.binary.rhs);                                                                           \
        return lhs op rhs

#define EVAL_UNARY(TOK, op)                                                                                            \
    case TOK:                                                                                                          \
        return op evalExpr(root->as.unary.inner)

int evalExpr(Expr *root) {
    int lhs, rhs;

    switch (root->type) {
        case EXPR_LITERAL:
            switch (root->as.primary.value.type) {
                case TOK_INTEGER_LITERAL:
                    return root->as.primary.value.as.integerLiteral;
                default:
                    TODO("Primary Expressions");
            }
        case EXPR_BINARY:
            switch (root->as.binary.op) {
                EVAL_BINARY(TOK_PLUS, +);
                EVAL_BINARY(TOK_MINUS, -);
                EVAL_BINARY(TOK_STAR, *);
                EVAL_BINARY(TOK_SLASH, /);
                EVAL_BINARY(TOK_PERCENT, %);
                EVAL_BINARY(TOK_AMPERSAND, &);
                EVAL_BINARY(TOK_CARET, ^);
                EVAL_BINARY(TOK_PIPE, |);
                EVAL_BINARY(TOK_LESS_LESS, <<);
                EVAL_BINARY(TOK_GREATER_GREATER, >>);
                EVAL_BINARY(TOK_PIPE_PIPE, ||);
                EVAL_BINARY(TOK_AMPERSAND_AMPERSAND, &&);
                EVAL_BINARY(TOK_EQUALS_EQUALS, ==);
                EVAL_BINARY(TOK_BANG_EQUALS, !=);
                EVAL_BINARY(TOK_LESS, <);
                EVAL_BINARY(TOK_LESS_EQUALS, <=);
                EVAL_BINARY(TOK_GREATER, >);
                EVAL_BINARY(TOK_GREATER_EQUALS, >=);
                case TOK_EQUALS:
                    return evalExpr(root->as.binary.rhs);
                default:
                    TODO("Binary Operators");
            }
        case EXPR_GROUPING:
            return evalExpr(root->as.grouping.inner);
        case EXPR_UNARY:
            switch (root->as.unary.op) {
                EVAL_UNARY(TOK_PLUS, +);
                EVAL_UNARY(TOK_MINUS, -);
                EVAL_UNARY(TOK_TILDE, ~);
                EVAL_UNARY(TOK_BANG, !);
                case TOK_PLUS_PLUS:
                    UNIMPLEMENTED("++");
                case TOK_MINUS_MINUS:
                    UNIMPLEMENTED("--");
                case TOK_AMPERSAND:
                    UNIMPLEMENTED("Unary Ampersand");
                case TOK_STAR:
                    UNIMPLEMENTED("Unary Star");
                default:
                    fprintf(stderr, "Not a valid unary operator: %d(%c)", root->as.unary.op, root->as.unary.op);
                    abort();
            }
        case EXPR_CONDITIONAL:
            return evalExpr(root->as.conditional.condition) ? evalExpr(root->as.conditional.thenBranch)
                                                            : evalExpr(root->as.conditional.elseBranch);
        case EXPR_INDEX:
            UNIMPLEMENTED("Index Expressions");
        case EXPR_FUNC_CALL:
            UNIMPLEMENTED("Function Call Expressions");
        case EXPR_MEMBER:
            UNIMPLEMENTED("Member Expressions");
    }
    printf("Unknown expression type: %d\n", root->type);
    UNIMPLEMENTED("Don't come here");
}

Expr *cloneExpr(Expr *src) {
    switch (src->type) {
        case EXPR_LITERAL:
            return makePrimaryExpr(src->as.primary.value);
        case EXPR_GROUPING:
            return makeGroupingExpr(cloneExpr(src->as.grouping.inner));
        case EXPR_BINARY:
            return makeBinaryExpr(src->as.binary.op, cloneExpr(src->as.binary.lhs), cloneExpr(src->as.binary.rhs));
        case EXPR_UNARY:
            return makeUnaryExpr(src->as.unary.op, cloneExpr(src->as.unary.inner));
        case EXPR_CONDITIONAL:
            return makeConditionalExpr(cloneExpr(src->as.conditional.condition),
                                       cloneExpr(src->as.conditional.thenBranch),
                                       cloneExpr(src->as.conditional.elseBranch));
        case EXPR_INDEX:
            return makeIndexExpr(cloneExpr(src->as.index.name), cloneExpr(src->as.index.index));
        case EXPR_FUNC_CALL: {
            ArgsList args = {0};
            for (usize i = 0; i < src->as.funcCall.args.len; i++) {
                appendSingle(&args, cloneExpr(src->as.funcCall.args.arr[i]));
            }
            return makeFuncCallExpr(cloneExpr(src->as.funcCall.callee), args);
        }
        case EXPR_MEMBER:
            return makeMemberExpr(src->as.member.op, cloneExpr(src->as.member.object), src->as.member.member);
    }
    UNREACHABLE("Unkown expression type");
}

static void printIndent(int indent) {
    for (int i = 0; i < indent; i++) printf("  ");
}

static void printExprImpl(Expr *root, int indent) {
    printf("{\n");
    switch (root->type) {
        case EXPR_LITERAL:
            printIndent(indent + 1);
            printf("\"type\": \"literal\",\n");
            printIndent(indent + 1);
            switch (root->as.primary.value.type) {
                case TOK_INTEGER_LITERAL:
                    printf("\"value\": %zu\n", root->as.primary.value.as.integerLiteral);
                    break;
                case TOK_FLOAT_LITERAL:
                    printf("\"value\": %f\n", root->as.primary.value.as.floatLiteral);
                    break;
                case TOK_IDENTIFIER:
                    printf("\"name\": \"%.*s\"\n", (int)root->as.primary.value.as.identifier.len,
                           root->as.primary.value.as.identifier.data);
                    break;
                case TOK_STRING_LITERAL:
                    printf("\"value\": \"%.*s\"\n", (int)root->as.primary.value.as.stringLiteral.len,
                           root->as.primary.value.as.stringLiteral.data);
                    break;
                case TOK_CHAR_LITERAL:
                    printf("\"value\": \"%c\"\n", root->as.primary.value.as.charLiteral);
                    break;
                default:
                    printf("\"value\": \"<unknown>\"\n");
                    break;
            }
            break;
        case EXPR_GROUPING:
            printIndent(indent + 1);
            printf("\"type\": \"grouping\",\n");
            printIndent(indent + 1);
            printf("\"inner\": ");
            printExprImpl(root->as.grouping.inner, indent + 1);
            break;
        case EXPR_BINARY:
            printIndent(indent + 1);
            printf("\"type\": \"binary\",\n");
            printIndent(indent + 1);
            printf("\"op\": \"%s\",\n", tokenTypesStrings[root->as.binary.op]);
            printIndent(indent + 1);
            printf("\"lhs\": ");
            printExprImpl(root->as.binary.lhs, indent + 1);
            printIndent(indent + 1);
            printf("\"rhs\": ");
            printExprImpl(root->as.binary.rhs, indent + 1);
            break;
        case EXPR_UNARY:
            printIndent(indent + 1);
            printf("\"type\": \"unary\",\n");
            printIndent(indent + 1);
            printf("\"op\": \"%s\",\n", tokenTypesStrings[root->as.unary.op]);
            printIndent(indent + 1);
            printf("\"inner\": ");
            printExprImpl(root->as.unary.inner, indent + 1);
            break;
        case EXPR_CONDITIONAL:
            printIndent(indent + 1);
            printf("\"type\": \"conditional\",\n");
            printIndent(indent + 1);
            printf("\"condition\": ");
            printExprImpl(root->as.conditional.condition, indent + 1);
            printIndent(indent + 1);
            printf("\"then\": ");
            printExprImpl(root->as.conditional.thenBranch, indent + 1);
            printIndent(indent + 1);
            printf("\"else\": ");
            printExprImpl(root->as.conditional.elseBranch, indent + 1);
            break;
        case EXPR_INDEX:
            printIndent(indent + 1);
            printf("\"type\": \"index\",\n");
            printIndent(indent + 1);
            printf("\"name\": ");
            printExprImpl(root->as.index.name, indent + 1);
            printIndent(indent + 1);
            printf("\"index\": ");
            printExprImpl(root->as.index.index, indent + 1);
            break;
        case EXPR_FUNC_CALL:
            printIndent(indent + 1);
            printf("\"type\": \"func_call\",\n");
            printIndent(indent + 1);
            printf("\"callee\": ");
            printExprImpl(root->as.funcCall.callee, indent + 1);
            printIndent(indent + 1);
            printf("\"args\": [\n");
            for (usize i = 0; i < root->as.funcCall.args.len; i++) {
                printIndent(indent + 2);
                printExprImpl(root->as.funcCall.args.arr[i], indent + 2);
            }
            printIndent(indent + 1);
            printf("]\n");
            break;
        case EXPR_MEMBER:
            printIndent(indent + 1);
            printf("\"type\": \"member\",\n");
            printIndent(indent + 1);
            printf("\"op\": \"%s\",\n", tokenTypesStrings[root->as.member.op]);
            printIndent(indent + 1);
            printf("\"object\": ");
            printExprImpl(root->as.member.object, indent + 1);
            printIndent(indent + 1);
            printf("\"member\": \"%.*s\"\n", (int)root->as.member.member.as.identifier.len,
                   root->as.member.member.as.identifier.data);
            break;
    }
    printIndent(indent);
    printf("}\n");
}

void printExpr(Expr *root) {
    printExprImpl(root, 0);
}
