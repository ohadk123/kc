#include "type.h"

static Type *make_type(TypeKind kind) {
    Type *t = malloc(sizeof(Type));
    t->kind = kind;
    t->size = sizeof(void *);
    t->isUnsigned = false;
    return t;
}

Type *type_make_primitive(PrimitiveTypeKind primitive) {
    size_t size;
    bool isUnsigned = true;
    switch (primitive) {
        case TYPE_VOID:  size = 0; break;
        case TYPE_BOOL:
        case TYPE_I8:    isUnsigned = false;
        case TYPE_U8:    size = 1; break;
        case TYPE_I16:   isUnsigned = false;
        case TYPE_U16:   size = 2; break;
        case TYPE_I32:   isUnsigned = false;
        case TYPE_U32:   size = 4; break;
        case TYPE_ISIZE: isUnsigned = false;
        case TYPE_USIZE: size = sizeof(size_t); break;
        case TYPE_I64:   isUnsigned = false;
        case TYPE_U64:   size = 8; break;
        case TYPE_F32:   size = 4; break;
        case TYPE_F64:   size = 8; break;
    }

    Type *t = make_type(TYPE_PRIMITIVE);
    t->as.primitive = primitive;
    t->size = size;
    t->isUnsigned = isUnsigned;
    return t;
}

Type *type_make_pointer(Type *pointer) {
    Type *t = make_type(TYPE_POINTER);
    t->as.pointer = pointer;
    return t;
}

Type *type_make_array(Type *elementType, Expr *len) {
    Type *t = make_type(TYPE_ARRAY);
    t->as.array.elementType = elementType;
    t->as.array.len = len;
    return t;
}

Type *type_make_func(Type *retType, StmtList params) {
    Type *t = make_type(TYPE_FUNC);
    t->as.func.retType = retType;
    t->as.func.params = params;
    return t;
}

PrimitiveTypeKind TokenToPrimitive[] = {
    [TOK_VOID]  = TYPE_VOID,
    [TOK_BOOL]  = TYPE_BOOL,
    [TOK_U8]    = TYPE_U8,
    [TOK_U16]   = TYPE_U16,
    [TOK_U32]   = TYPE_U32,
    [TOK_U64]   = TYPE_U64,
    [TOK_USIZE] = TYPE_USIZE,
    [TOK_I8]    = TYPE_I8,
    [TOK_I16]   = TYPE_I16,
    [TOK_I32]   = TYPE_I32,
    [TOK_I64]   = TYPE_I64,
    [TOK_ISIZE] = TYPE_ISIZE,
    [TOK_F32]   = TYPE_F32,
    [TOK_F64]   = TYPE_F64,
};
