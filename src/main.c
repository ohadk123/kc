#include "codegen.h"
#include "lexer.h"
#include "parser.h"
#include "sema.h"
#include <unistd.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: kc <file>\n");
        exit(1);
    };

    TranslationUnit unit = (TranslationUnit){
        .fileName = str_from_cstr(argv[1]),
        .input = str_from_file(argv[1]),
    };

    scan_file(&unit);
    parse(&unit);
    semantic_analysis(&unit);
    codegen(&unit, stdout);
    return 0;
}

int main1(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: kc <file>\n");
        exit(1);
    };

    TranslationUnit unit = (TranslationUnit){
        .fileName = str_from_cstr(argv[1]),
        .input = str_from_file(argv[1]),
    };

    if (!scan_file(&unit)) {
        printf("LEXER ERROR");
    }

    // for (size_t i = 0; i < unit.tokens.len; i++) {
    //     print_tok(unit.tokens.arr[i]);
    // }

    parse(&unit);

    // for (size_t i = 0; i < unit.ast.len; i++) {
    //     print_stmt(unit.ast.arr[i], 1);
    //     printf("\n");
    // }
    semantic_analysis(&unit);

    if (argc > 2 && !strcmp(argv[2], "-q")) {
        codegen(&unit, stdout);
        return 0;
    }

    const char *fileNameNoExt = strtok(argv[1], ".");
    String qbeFile = str_printf("%s.ssa", fileNameNoExt);
    String asmFile = str_printf("%s.s", fileNameNoExt);

    FILE *outf = fopen(qbeFile.data, "w");
    codegen(&unit, outf);
    fflush(outf);
    fclose(outf);

    String qbeCommand = str_printf("qbe %.*s -o %.*s", strf(qbeFile), strf(asmFile));
    int ret = system(qbeCommand.data);
    printf("qbe done with code %d\n", ret);
    remove(qbeFile.data);
    if (ret != 0) exit(1);

    String asmCommand = str_printf("gcc %.*s -o %s", strf(asmFile), fileNameNoExt);
    ret = system(asmCommand.data);
    printf("gcc done with code %d\n", ret);
    remove(asmFile.data);
    if (ret != 0) exit(2);

    // printf("compiling done!\n");
    return 0;
}

int main2(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: kc <file> [flags]\n\n");
        fprintf(stderr, "FLAGS:\n");
        fprintf(stderr, "    -q        dump qbe output\n");
        fprintf(stderr, "    -g        compile into executable\n");
        exit(1);
    }

    bool qbe = false;
    bool gcc = false;
    if (argc > 2) {
        if (!strcmp(argv[2], "-q")) qbe = true;
        if (!strcmp(argv[2], "-g")) gcc = qbe = true;
    }

    TranslationUnit unit = (TranslationUnit){
        .fileName = str_from_cstr(argv[1]),
        .input = str_from_file(argv[1]),
    };

    if (!scan_file(&unit)) {
        printf("LEXER ERROR");
    }

    // for (size_t i = 0; i < unit.tokens.len; i++) {
    //     print_tok(unit.tokens.arr[i]);
    // }

    parse(&unit);

    // for (size_t i = 0; i < unit.ast.len; i++) {
    //     print_stmt(unit.ast.arr[i], 1);
    //     printf("\n");
    // }
    semantic_analysis(&unit);

    const char *fileNameNoExt = strtok(argv[1], ".");
    String qbeFile = str_printf("%s.ssa", fileNameNoExt);
    String asmFile = str_printf("%s.s", fileNameNoExt);

    FILE *outf = stdout;
    if (qbe) outf = fopen(qbeFile.data, "w");
    codegen(&unit, outf);
    fflush(outf);
    if (qbe) fclose(outf);

    if (qbe) {
        String qbeCommand = str_printf("qbe %.*s", strf(qbeFile));
        if (gcc) qbeCommand = str_printf("qbe %.*s -o %.*s", strf(qbeFile), strf(asmFile));
        int ret = system(qbeCommand.data);
        printf("qbe done with code %d\n", ret);
        // remove(qbeFile.data);
    }

    if (gcc) {
        String asmCommand = str_printf("gcc %.*s -o %s", strf(asmFile), fileNameNoExt);
        int ret = system(asmCommand.data);
        printf("gcc done with code %d\n", ret);
        // remove(asmFile.data);
    }

    // printf("compiling done!\n");
    return 0;
}
