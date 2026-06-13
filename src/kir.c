#include "kir.h"
#include <inttypes.h>
#include <stdio.h>

static InstKir *make_inst_kir(InstKirKind kind) {
    InstKir *inst = calloc(1, sizeof(InstKir));
    inst->kind = kind;
    return inst;
}

InstKir *inst_kir_make_return(KirVal val) {
    InstKir *inst = make_inst_kir(KIR_INST_RETURN);
    inst->as.returnK.val = val;
    return inst;
}

InstKir *inst_kir_make_unary(KirUnaryOp op, String dst, KirVal src) {
    InstKir *inst = make_inst_kir(KIR_INST_UNARY);
    inst->as.unary.op = op;
    inst->as.unary.dst = dst;
    inst->as.unary.src = src;
    return inst;
}

InstKir *inst_kir_make_binary(KirBinaryOp op, String dst, KirVal src1, KirVal src2) {
    InstKir *inst = make_inst_kir(KIR_INST_BINARY);
    inst->as.binary.op = op;
    inst->as.binary.dst = dst;
    inst->as.binary.src1 = src1;
    inst->as.binary.src2 = src2;
    return inst;
}

InstKir *inst_kir_make_copy(String dst, KirVal src) {
    InstKir *inst = make_inst_kir(KIR_INST_COPY);
    inst->as.copy.dst = dst;
    inst->as.copy.src = src;
    return inst;
}

InstKir *inst_kir_make_jump(String target) {
    InstKir *inst = make_inst_kir(KIR_INST_JUMP);
    inst->as.jump.target = target;
    return inst;
}

InstKir *inst_kir_make_jz(KirVal cond, String target) {
    InstKir *inst = make_inst_kir(KIR_INST_JZ);
    inst->as.jz.cond = cond;
    inst->as.jz.target = target;
    return inst;
}

InstKir *inst_kir_make_jnz(KirVal cond, String target) {
    InstKir *inst = make_inst_kir(KIR_INST_JNZ);
    inst->as.jnz.cond = cond;
    inst->as.jnz.target = target;
    return inst;
}

InstKir *inst_kir_make_label(String name) {
    InstKir *inst = make_inst_kir(KIR_INST_LABEL);
    inst->as.label.name = name;
    return inst;
}

FuncDefKir *kir_make_func(Token name, InstKirList body) {
    FuncDefKir *func = calloc(1, sizeof(FuncDefKir));
    func->name = name;
    func->body = body;
    return func;
}

ProgramKir *kir_make_prog(FuncDefKir *funcDef) {
    ProgramKir *prog = calloc(1, sizeof(ProgramKir));
    prog->funcDef = funcDef;
    return prog;
}

static const char *unary_op_str(KirUnaryOp op) {
    switch (op) {
        case KIR_UNARY_OP_COMPLEMENT: return "~";
        case KIR_UNARY_OP_NEGATE:     return "-";
        case KIR_UNARY_OP_NOT:        return "!";
    }
    return "?";
}

static const char *binary_op_str(KirBinaryOp op) {
    switch (op) {
        case KIR_BINARY_OP_ADD:     return "+";
        case KIR_BINARY_OP_SUB:     return "-";
        case KIR_BINARY_OP_MULT:    return "*";
        case KIR_BINARY_OP_DIV:     return "/";
        case KIR_BINARY_OP_REM:     return "%";
        case KIR_BINARY_OP_BIT_AND: return "&";
        case KIR_BINARY_OP_BIT_OR:  return "|";
        case KIR_BINARY_OP_BIT_XOR: return "^";
        case KIR_BINARY_OP_SHL:     return "<<";
        case KIR_BINARY_OP_SHR:     return ">>";
        case KIR_BINARY_OP_EQ:      return "==";
        case KIR_BINARY_OP_NEQ:     return "!=";
        case KIR_BINARY_OP_LT:      return "<";
        case KIR_BINARY_OP_LE:      return "<=";
        case KIR_BINARY_OP_GT:      return ">";
        case KIR_BINARY_OP_GE:      return ">=";
    }
    return "?";
}

static void print_kir_val(KirVal v) {
    switch (v.kind) {
        case KIR_VAL_CONSTANT: printf("%" PRIu64, v.as.constant); break;
        case KIR_VAL_IDENT:    printf("%.*s", strf(v.as.ident)); break;
    }
}

static void print_inst_kir(InstKir *inst) {
    switch (inst->kind) {
        case KIR_INST_RETURN:
            printf("  ret ");
            print_kir_val(inst->as.returnK.val);
            printf("\n");
            break;
        case KIR_INST_UNARY:
            printf("  %.*s = %s", strf(inst->as.unary.dst), unary_op_str(inst->as.unary.op));
            print_kir_val(inst->as.unary.src);
            printf("\n");
            break;
        case KIR_INST_BINARY:
            printf("  %.*s = ", strf(inst->as.binary.dst));
            print_kir_val(inst->as.binary.src1);
            printf(" %s ", binary_op_str(inst->as.binary.op));
            print_kir_val(inst->as.binary.src2);
            printf("\n");
            break;
        case KIR_INST_COPY:
            printf("  %.*s = ", strf(inst->as.copy.dst));
            print_kir_val(inst->as.copy.src);
            printf("\n");
            break;
        case KIR_INST_JUMP:
            printf("  jmp %.*s\n", strf(inst->as.jump.target));
            break;
        case KIR_INST_JZ:
            printf("  jz ");
            print_kir_val(inst->as.jz.cond);
            printf(", %.*s\n", strf(inst->as.jz.target));
            break;
        case KIR_INST_JNZ:
            printf("  jnz ");
            print_kir_val(inst->as.jnz.cond);
            printf(", %.*s\n", strf(inst->as.jnz.target));
            break;
        case KIR_INST_LABEL:
            printf("%.*s:\n", strf(inst->as.label.name));
            break;
    }
}

void print_kir(ProgramKir *prog) {
    FuncDefKir *func = prog->funcDef;
    printf("function %.*s:\n", strf(func->name.as.identifier));
    for (size_t i = 0; i < func->body.len; i++) {
        print_inst_kir(func->body.arr[i]);
    }
}
