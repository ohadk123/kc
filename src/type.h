#ifndef TYPE_H
#define TYPE_H

#include "token.h"

typedef struct Type Type;

extern Type *type_err;
extern Type *type_i32;
extern Type *type_i64;
extern Type *type_u32;
extern Type *type_u64;

typedef enum {
    TYPE_ERR = 0,

    TYPE_I32 = TOK_I32,
    TYPE_I64 = TOK_I64,
    TYPE_U32 = TOK_U32,
    TYPE_U64 = TOK_U64,
} TypeKind;

typedef struct {
    LIST_FIELDS(Type *);
} TypeList;

struct Type {
    TypeKind kind;
    uint32_t size;
    uint32_t align;
};

Type *type_make_primitive(TokenKind kind);

Type *type_common(Type *a, Type *b);

bool type_equal(const Type *a, const Type *b);
bool type_is_integer(const Type *t);
bool type_is_signed(const Type *t);

const char *type_name(const Type *t);

#endif // TYPE_H
