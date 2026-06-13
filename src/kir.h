#ifndef KIR_H
#define KIR_H

#include "token.h"
#include <stdint.h>

typedef enum {
    KIR_VAL_CONSTANT,
    KIR_VAL_IDENT,
} KirValKind;

typedef struct {
    KirValKind kind;
    union {
        uint64_t constant;
        String ident;
    } as;
} ValKir;

// KIR Instruction

typedef enum {
    KIR_INST_RETURN,
    KIR_INST_UNARY,
    KIR_INST_BINARY,
    KIR_INST_COPY,
    KIR_INST_JUMP,
    KIR_INST_JZ,
    KIR_INST_JNZ,
    KIR_INST_LABEL,
} InstKirKind;

typedef struct {
    ValKir val;
} ReturnInstKir;

typedef enum {
    KIR_UNARY_OP_COMPLEMENT,
    KIR_UNARY_OP_NEGATE,
    KIR_UNARY_OP_NOT,
} UnaryInstKirOp;

typedef struct {
    UnaryInstKirOp op;
    String dst;
    ValKir src;
} UnaryInstKir;

typedef enum {
    KIR_BINARY_ADD,
    KIR_BINARY_SUB,
    KIR_BINARY_MULT,
    KIR_BINARY_DIV,
    KIR_BINARY_REM,
    KIR_BINARY_BIT_AND,
    KIR_BINARY_BIT_OR,
    KIR_BINARY_BIT_XOR,
    KIR_BINARY_SHL,
    KIR_BINARY_SHR,
    KIR_BINARY_EQ,
    KIR_BINARY_NEQ,
    KIR_BINARY_LT,
    KIR_BINARY_LE,
    KIR_BINARY_GT,
    KIR_BINARY_GE,
} BinaryInstKirOp;

typedef struct {
    BinaryInstKirOp op;
    String dst;
    ValKir src1, src2;
} BinaryInstKir;

typedef struct {
    String dst;
    ValKir src;
} CopyInstKir;

typedef struct {
    String target;
} JumpInstKir;

typedef struct {
    ValKir cond;
    String target;
} JzInstKir;

typedef struct {
    ValKir cond;
    String target;
} JnzInstKir;

typedef struct {
    String name;
} LabelInstKir;

typedef struct {
    InstKirKind kind;
    union {
        ReturnInstKir returnK;
        UnaryInstKir unary;
        BinaryInstKir binary;
        CopyInstKir copy;
        JumpInstKir jump;
        JzInstKir jz;
        JnzInstKir jnz;
        LabelInstKir label;
    } as;
} InstKir;

InstKir *kir_make_return(ValKir val);
InstKir *kir_make_unary(UnaryInstKirOp op, String dst, ValKir src);
InstKir *kir_make_binary(BinaryInstKirOp op, String dst, ValKir src1, ValKir src2);
InstKir *kir_make_copy(String dst, ValKir src);
InstKir *kir_make_jump(String target);
InstKir *kir_make_jz(ValKir cond, String target);
InstKir *kir_make_jnz(ValKir cond, String target);
InstKir *kir_make_label(String name);

typedef struct {
    LIST_FIELDS(InstKir *);
} InstKirList;

// KIR itself

typedef struct {
    Token name;
    InstKirList body;
} FuncDefKir;

FuncDefKir *kir_make_func(Token name, InstKirList body);

typedef struct {
    FuncDefKir *funcDef;
} ProgramKir;

ProgramKir *kir_make_prog(FuncDefKir *funcDef);

void print_kir(ProgramKir *prog);

#endif // KIR_H
