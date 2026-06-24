#ifndef TYPE_H
#define TYPE_H

#include "token.h"

typedef enum {
    TYPE_I32 = TOK_I32,
    TYPE_I64 = TOK_I64,

    TYPE_FUNC,
} TypeKind;

typedef struct Type Type;

typedef struct {
    LIST_FIELDS(Type *);
} TypeList;

typedef struct {
    Type    *ret;
    TypeList params;
} FuncType;

struct Type {
    TypeKind kind;
    size_t size;
    size_t align;
    union {
        FuncType func;
    } as;
};

Type *type_make_primitive(TokenKind kind);
Type *type_make_func(Type *ret, TypeList params);
