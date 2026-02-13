#include "Type.h"

Type *makePrimitiveType(Token simpleKind, Bool isConst) {
    Type *t = malloc(sizeof(Type));
    t->isConst = isConst;
    t->kind = TYPE_SIMPLE;
    t->as.simple = simpleKind;
    return t;
}

Type *makePointerType(Type *pointerType, Bool isConst) {
    Type *t = malloc(sizeof(Type));
    t->isConst = isConst;
    t->kind = TYPE_POINTER;
    t->as.pointer = pointerType;
    return t;
}

Type *makeArrayType(Type *innerType, Expr *sizeExpr, Bool isConst) {
    Type *t = malloc(sizeof(Type));
    t->isConst = isConst;
    t->kind = TYPE_ARRAY;
    t->as.array.inner = innerType;
    t->as.array.size = sizeExpr;
    return t;
}
