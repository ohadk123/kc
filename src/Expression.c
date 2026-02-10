#include "Expression.h"

#include <libk/Errors.h>

int eval(Expr *root) {
    int lhs, rhs;

    switch (root->type) {
        case EXPR_NUM:
            return root->as.num.as.integerLiteral;
        case EXPR_BINARY:
            switch (root->as.binary.op) {
                case TOK_PLUS:
                    lhs = eval(root->as.binary.lhs);
                    rhs = eval(root->as.binary.rhs);
                    return lhs + rhs;
                case TOK_STAR:
                    lhs = eval(root->as.binary.lhs);
                    rhs = eval(root->as.binary.rhs);
                    return lhs * rhs;
                default:
                    TODO("Binary Operators");
            }
        default:
            TODO("Expressions");
            break;
    }
}
