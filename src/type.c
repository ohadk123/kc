#include "type.h"

// Primitive singletons: one shared object per primitive, statically allocated,
// never freed. Because every i32 is the same pointer, type_equal()'s identity
// fast path resolves them in one comparison.
static Type i32_type  = {.kind = TYPE_I32, .size = 4, .align = 4};
static Type i64_type  = {.kind = TYPE_I64, .size = 8, .align = 8};

Type *type_i32  = &i32_type;
Type *type_i64  = &i64_type;

static Type *make_type(TypeKind kind, size_t size, size_t align) {
    Type *t = calloc(1, sizeof(Type));
    t->kind = kind;
    t->size = size;
    t->align = align;

    return t;
}

Type *type_make_primitive(TokenKind kind) {
    switch (kind) {
        case TOK_I32:  return type_i32;
        case TOK_I64:  return type_i64;
        default:       return NULL; // not a supported type (yet)
    }
}

Type *type_make_func(Type *ret, TypeList params) {
    Type *t = make_type(TYPE_FUNC, 8, 8);
    t->kind = TYPE_FUNC;
    t->as.func.ret = ret;
    t->as.func.params = params;

    return t;
}
