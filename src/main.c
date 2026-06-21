#include "codegen.h"
#include "lexer.h"
#include "parser.h"
#include "sema.h"
#include <unistd.h>

int main(int argc, char *argv[]) {
    const char *source = NULL;
    bool object = false;
    bool dump_qbe = false;

    if (argc < 2) {
        fprintf(stderr, "Usage: kc [-qc] <file>\n");
        exit(1);
    }

    for (int i = 1; i < argc; i++) {
        const char *arg = argv[i];

        if (arg[0] != '-') {
            if (source) {
                fprintf(stderr, "Cannot compile multiple files");
                return 1;
            } else {
                source = arg;
            }
        } else {
            if (!strcmp(arg, "-c")) {
                object = true;
            } else if (!strcmp(arg, "-q")) {
                dump_qbe = true;
            } else {
                fprintf(stderr, "Unkown flag '%s'", arg);
                return 1;
            }
        }
    }

    TranslationUnit unit = (TranslationUnit){
        .fileName = str_from_cstr(source),
        .input = str_from_file(source),
    };

    if (!scan_file(&unit)) return 1;
    parse(&unit);
    if (!semantic_analysis(&unit)) return 1;

    if (dump_qbe) {
        codegen(&unit, NULL);
        return 0;
    }

    const char *fileNameNoExt = strtok((char *)source, ".");
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

    String asmCommand;
    if (object)
        asmCommand = str_printf("gcc -c %.*s", strf(asmFile), fileNameNoExt);
    else
        asmCommand = str_printf("gcc %.*s -o %s", strf(asmFile), fileNameNoExt);

    ret = system(asmCommand.data);
    printf("gcc done with code %d\n", ret);
    remove(asmFile.data);
    if (ret != 0) exit(2);

    printf("Compilation finished successfully!\n");
    return 0;
}
