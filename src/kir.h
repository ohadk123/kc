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
} KirVal;

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
    KirVal val;
} ReturnInstKir;

typedef enum {
    KIR_UNARY_OP_COMPLEMENT,
    KIR_UNARY_OP_NEGATE,
    KIR_UNARY_OP_NOT,
} KirUnaryOp;

typedef struct {
    KirUnaryOp op;
    String dst;
    KirVal src;
} UnaryInstKir;

typedef enum {
    KIR_BINARY_OP_ADD,
    KIR_BINARY_OP_SUB,
    KIR_BINARY_OP_MULT,
    KIR_BINARY_OP_DIV,
    KIR_BINARY_OP_REM,
    KIR_BINARY_OP_BIT_AND,
    KIR_BINARY_OP_BIT_OR,
    KIR_BINARY_OP_BIT_XOR,
    KIR_BINARY_OP_SHL,
    KIR_BINARY_OP_SHR,
    KIR_BINARY_OP_EQ,
    KIR_BINARY_OP_NEQ,
    KIR_BINARY_OP_LT,
    KIR_BINARY_OP_LE,
    KIR_BINARY_OP_GT,
    KIR_BINARY_OP_GE,
} KirBinaryOp;

typedef struct {
    KirBinaryOp op;
    String dst;
    KirVal src1, src2;
} BinaryInstKir;

typedef struct {
    String dst;
    KirVal src;
} CopyInstKir;

typedef struct {
    String target;
} JumpInstKir;

typedef struct {
    KirVal cond;
    String target;
} JzInstKir;

typedef struct {
    KirVal cond;
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

InstKir *inst_kir_make_return(KirVal val);
InstKir *inst_kir_make_unary(KirUnaryOp op, String dst, KirVal src);
InstKir *inst_kir_make_binary(KirBinaryOp op, String dst, KirVal src1, KirVal src2);
InstKir *inst_kir_make_copy(String dst, KirVal src);
InstKir *inst_kir_make_jump(String target);
InstKir *inst_kir_make_jz(KirVal cond, String target);
InstKir *inst_kir_make_jnz(KirVal cond, String target);
InstKir *inst_kir_make_label(String name);

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
