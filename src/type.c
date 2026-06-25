#include "type.h"

static Type err_type  = {.kind = TYPE_ERR, .size = 0, .align = 0};
static Type i32_type  = {.kind = TYPE_I32, .size = 4, .align = 4};
static Type i64_type  = {.kind = TYPE_I64, .size = 8, .align = 8};

Type *type_err  = &err_type;
Type *type_i32  = &i32_type;
Type *type_i64  = &i64_type;

Type *type_make_primitive(TokenKind kind) {
    switch (kind) {
        case TOK_I32:  return type_i32;
        case TOK_I64:  return type_i64;
        default:       return NULL; // not a supported type (yet)
    }
}

bool type_equal(const Type *a, const Type *b) {
    if (!a || !b) return false;
    if (a == b) return true;
    if (a->kind != b->kind) return false;

    return true;
}

bool type_is_integer(const Type *t) {
    return t && (t->kind == TYPE_I32 || t->kind == TYPE_I64);
}

Type *type_common(Type *a, Type *b) {
    assert(type_is_integer(a) && type_is_integer(b));
    return (a->kind == TYPE_I64 || b->kind == TYPE_I64) ? type_i64 : type_i32;
}

const char *type_name(const Type *t) {
    switch (t->kind) {
        case TYPE_ERR: return "err";
        case TYPE_I32: return "i32";
        case TYPE_I64: return "i64";
    }
    UNREACHABLE("Invalid type kind (%d)", t->kind);
}
