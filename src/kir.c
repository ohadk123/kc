#include "kir.h"

static InstKir *make_inst_kir(InstKirKind kind) {
    InstKir *inst = calloc(1, sizeof(InstKir));
    inst->kind = kind;
    return inst;
}

InstKir *kir_make_return(ValKir val) {
    InstKir *inst = make_inst_kir(KIR_INST_RETURN);
    inst->as.returnK.val = val;
    return inst;
}

InstKir *kir_make_unary(UnaryInstKirOp op, String dst, ValKir src) {
    InstKir *inst = make_inst_kir(KIR_INST_UNARY);
    inst->as.unary.op = op;
    inst->as.unary.dst = dst;
    inst->as.unary.src = src;
    return inst;
}

InstKir *kir_make_binary(BinaryInstKirOp op, String dst, ValKir src1, ValKir src2) {
    InstKir *inst = make_inst_kir(KIR_INST_BINARY);
    inst->as.binary.op = op;
    inst->as.binary.dst = dst;
    inst->as.binary.src1 = src1;
    inst->as.binary.src2 = src2;
    return inst;
}

InstKir *kir_make_copy(String dst, ValKir src) {
    InstKir *inst = make_inst_kir(KIR_INST_COPY);
    inst->as.copy.dst = dst;
    inst->as.copy.src = src;
    return inst;
}

InstKir *kir_make_jump(String target) {
    InstKir *inst = make_inst_kir(KIR_INST_JUMP);
    inst->as.jump.target = target;
    return inst;
}

InstKir *kir_make_jz(ValKir cond, String target) {
    InstKir *inst = make_inst_kir(KIR_INST_JZ);
    inst->as.jz.cond = cond;
    inst->as.jz.target = target;
    return inst;
}

InstKir *kir_make_jnz(ValKir cond, String target) {
    InstKir *inst = make_inst_kir(KIR_INST_JNZ);
    inst->as.jnz.cond = cond;
    inst->as.jnz.target = target;
    return inst;
}

InstKir *kir_make_label(String name) {
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
