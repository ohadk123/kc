#include "type.h"

static Type *make_type(TypeKind kind) {
    Type *t = calloc(1, sizeof(Type));
    t->kind = kind;
    t->size = sizeof(void *);
    t->isUnsigned = false;
    return t;
}

Type *type_make_primitive(PrimitiveTypeKind primitive) {
    size_t size = 0;
    bool isUnsigned = true;
    switch (primitive) {
        case TYPE_VOID: size = 0; break;
        case TYPE_BOOL: size = 1; break;
        case TYPE_I8:
            size = 1;
            isUnsigned = false;
            break;
        case TYPE_U8: size = 1; break;
        case TYPE_I16:
            size = 2;
            isUnsigned = false;
            break;
        case TYPE_U16: size = 2; break;
        case TYPE_I32:
            size = 4;
            isUnsigned = false;
            break;
        case TYPE_U32: size = 4; break;
        case TYPE_ISIZE:
            size = sizeof(size_t);
            isUnsigned = false;
            break;
        case TYPE_USIZE: size = sizeof(size_t); break;
        case TYPE_I64:
            size = 8;
            isUnsigned = false;
            break;
        case TYPE_U64: size = 8; break;
        case TYPE_F32: size = 4; break;
        case TYPE_F64: size = 8; break;
    }

    Type *t = make_type(TYPE_PRIMITIVE);
    t->as.primitive = primitive;
    t->size = size;
    t->isUnsigned = isUnsigned;
    return t;
}

Type *type_make_primitive_from_token(TokenKind primitive) { return type_make_primitive(TokenToPrimitive[primitive]); }

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
    [TOK_VOID] = TYPE_VOID, [TOK_BOOL] = TYPE_BOOL,   [TOK_U8] = TYPE_U8,   [TOK_U16] = TYPE_U16, [TOK_U32] = TYPE_U32,
    [TOK_U64] = TYPE_U64,   [TOK_USIZE] = TYPE_USIZE, [TOK_I8] = TYPE_I8,   [TOK_I16] = TYPE_I16, [TOK_I32] = TYPE_I32,
    [TOK_I64] = TYPE_I64,   [TOK_ISIZE] = TYPE_ISIZE, [TOK_F32] = TYPE_F32, [TOK_F64] = TYPE_F64,
};

static int typePriorities[] = {
    [TYPE_VOID] = 0, [TYPE_BOOL] = 1,   [TYPE_U8] = 2,   [TYPE_U16] = 3,  [TYPE_U32] = 4,
    [TYPE_U64] = 5,  [TYPE_USIZE] = 6,  [TYPE_I8] = 7,   [TYPE_I16] = 8,  [TYPE_I32] = 9,
    [TYPE_I64] = 10, [TYPE_ISIZE] = 11, [TYPE_F32] = 12, [TYPE_F64] = 13,
};

PrimitiveTypeKind choose_pritimitive(PrimitiveTypeKind a, PrimitiveTypeKind b) {
    if (typePriorities[a] > typePriorities[b])
        return a;
    else
        return b;
}

Type *compare_types(Type *a, Type *b) {
    if (a->kind != b->kind) return NULL;

    switch (a->kind) {
        case TYPE_PRIMITIVE: return type_make_primitive(choose_pritimitive(a->as.primitive, b->as.primitive));
        case TYPE_POINTER:   {
            Type *pointedType = compare_types(a->as.pointer, b->as.pointer);
            if (!pointedType) return NULL;
            return type_make_pointer(pointedType);
        }
        case TYPE_ARRAY: {
            Type *elementType = compare_types(a->as.array.elementType, b->as.array.elementType);
            if (!elementType) return NULL;
            return type_make_array(elementType, a->as.array.len);
        }
        case TYPE_FUNC: {
            if (a->as.func.params.len != b->as.func.params.len) return NULL;
            Type *retType = compare_types(a->as.func.retType, b->as.func.retType);
            if (!retType) return NULL;
            for (size_t i = 0; i < a->as.func.params.len; i++) {
                Type *paramA = a->as.func.params.arr[i]->as.var.type;
                Type *paramB = b->as.func.params.arr[i]->as.var.type;
                if (!compare_types(paramA, paramB)) return NULL;
            }
            return type_make_func(retType, a->as.func.params);
        }
    }

    UNREACHABLE("Not a type kind (%d)", a->kind);
}

static const char *primitive_to_cstr(PrimitiveTypeKind primitive) {
    switch (primitive) {
        case TYPE_VOID:  return "void";
        case TYPE_BOOL:  return "bool";
        case TYPE_U8:    return "u8";
        case TYPE_U16:   return "u16";
        case TYPE_U32:   return "u32";
        case TYPE_U64:   return "u64";
        case TYPE_USIZE: return "usize";
        case TYPE_I8:    return "i8";
        case TYPE_I16:   return "i16";
        case TYPE_I32:   return "i32";
        case TYPE_I64:   return "i64";
        case TYPE_ISIZE: return "isize";
        case TYPE_F32:   return "f32";
        case TYPE_F64:   return "f64";
    }

    UNREACHABLE("Not a primitive type kind (%d)", primitive);
}

String type_to_string(Type *t) {
    StringBuilder sb = {0};

    switch (t->kind) {
        case TYPE_PRIMITIVE: sb_appendf(&sb, "%s", primitive_to_cstr(t->as.primitive)); break;
        case TYPE_POINTER:   {
            String str = type_to_string(t->as.pointer);
            sb_appendf(&sb, "%.*s *", strf(str));
            break;
        }
        case TYPE_ARRAY: {
            String str = type_to_string(t->as.array.elementType);
            sb_appendf(&sb, "%.*s[]", strf(str));
            break;
        }
        case TYPE_FUNC: {
            String retTypeStr = type_to_string(t->as.func.retType);
            sb_appendf(&sb, "%.*s (", strf(retTypeStr));
            for (size_t i = 0; i < t->as.func.params.len; i++) {
                Stmt *param = t->as.func.params.arr[i];
                String paramTypeStr = type_to_string(param->as.var.type);
                sb_appendf(&sb, "%.*s", strf(paramTypeStr));
                if (i != t->as.func.params.len - 1) sb_appendf(&sb, ", ");
            }
            sb_appendf(&sb, ")");
            break;
        }

        default: UNREACHABLE("Not a type kind (%d)", t->kind);
    }

    return finish_string(&sb);
}
