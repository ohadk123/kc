#include "type.h"

/******************************************************************************
 * Primitive Types List
 *****************************************************************************/

// Must be defined in order!
// u64 >= i64 >= u32 >= i32
#define TYPES_LIST(X)      \
    X(u64, TYPE_U64, 8, 8) \
    X(i64, TYPE_I64, 8, 8) \
    X(u32, TYPE_U32, 4, 4) \
    X(i32, TYPE_I32, 4, 4) \
    X(err, TYPE_ERR, 0, 0) \

/******************************************************************************
 * Preprocessor generated definitions
 *****************************************************************************/

#define X(name, k, s, a) static Type name##_type = {.kind = k, .size = s, .align = a};
TYPES_LIST(X)
#undef X

#define X(name, k, s, a) Type *type_##name = &name##_type;
TYPES_LIST(X)
#undef X

#define X(name, k, s, a) case k: return type_##name;
Type *type_make_primitive(TokenKind kind) {
    switch ((TypeKind)kind) { TYPES_LIST(X) }
    UNREACHABLE("Invalid TokenKind for TypeKind (%s)", tokenTypesStrings[kind]);
}
#undef X

#define X(name, k, s, a) case k: return #name;
const char *type_name(const Type *t) {
    switch (t->kind) { TYPES_LIST(X) }
    UNREACHABLE("Invalid type kind (%d)", t->kind);
}
#undef X

#define X(name, k, s, al) if (a->kind == k || b->kind == k) return type_##name;
Type *type_common(Type *a, Type *b) {
    assert(type_is_integer(a) && type_is_integer(b));
    TYPES_LIST(X)
    UNREACHABLE("Invalid TypeKind (%s, %s)", tokenTypesStrings[a->kind], tokenTypesStrings[b->kind]);
}

/******************************************************************************
 * Type.c functions
 *****************************************************************************/

bool type_equal(const Type *a, const Type *b) {
    if (!a || !b) return false;
    if (a == b) return true;
    if (a->kind != b->kind) return false;

    return true;
}

bool type_is_integer(const Type *t) {
    assert(t);

    switch (t->kind) {
        case TYPE_ERR: return false;
        case TYPE_I32:
        case TYPE_I64:
        case TYPE_U32:
        case TYPE_U64: return true;
    }

    UNREACHABLE("Invalid TypeKind (%s)", tokenTypesStrings[t->kind]);
}
