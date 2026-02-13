#ifndef INCLUCE_KC_TYPE_H_
#define INCLUCE_KC_TYPE_H_

#include "Expression.h"

typedef enum {
    TYPE_SIMPLE, // primitive or alias/enum/struct/union type name
    TYPE_POINTER,
    TYPE_ARRAY,
} TypeKind;

typedef struct Type Type;
struct Type {
    TypeKind kind;
    union {
        Token simple;
        Type *pointer;
        struct {
            Type *inner;
            Expr *size;
        } array;
    } as;
    Bool isConst;
};

Type *makePrimitiveType(Token primitiveType, Bool isConst);
Type *makePointerType(Type *pointerType, Bool isConst);
Type *makeArrayType(Type *innerType, Expr *sizeExpr, Bool isConst);

#endif // INCLUCE_KC_TYPE_H_
