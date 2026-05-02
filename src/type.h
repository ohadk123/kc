#ifndef TYPE_H
#define TYPE_H

#include "expression.h"
#include "statement.h"
#include "token.h"

typedef struct _Type Type;

typedef enum {
    TYPE_PRIMITIVE,
    TYPE_POINTER,
    TYPE_ARRAY,
    TYPE_FUNC,
} TypeKind;

typedef enum {
    TYPE_VOID,
    TYPE_BOOL,
    TYPE_U8,
    TYPE_U16,
    TYPE_U32,
    TYPE_U64,
    TYPE_USIZE,
    TYPE_I8,
    TYPE_I16,
    TYPE_I32,
    TYPE_I64,
    TYPE_ISIZE,
    TYPE_F32,
    TYPE_F64,
} PrimitiveTypeKind;

typedef struct {
    Type *elementType;
    Expr *len;
} ArrayType;

typedef struct {
    Type *retType;
    StmtList params;
} FuncType;

struct _Type {
    TypeKind kind;
    size_t size; // size in bytes
    bool isUnsigned;
    union {
        PrimitiveTypeKind primitive;
        Type *pointer;
        ArrayType array;
        FuncType func;
    } as;
};

Type *type_make_primitive(PrimitiveTypeKind primitive);
Type *type_make_primitive_from_token(TokenKind primitive);
Type *type_make_pointer(Type *pointer);
Type *type_make_array(Type *elementType, Expr *len);
Type *type_make_func(Type *retType, StmtList params);

extern PrimitiveTypeKind TokenToPrimitive[];

Type *compare_types(Type *a, Type *b);
String type_to_string(Type *t);

#endif // TYPE_H
