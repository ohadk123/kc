#include "lexer.h"
#include "parser.h"
#include "sema.h"
#include "codegen.h"
#include <unistd.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: kc <file>\n");
        exit(1);
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

    FILE *outf = fopen(qbeFile.data, "w");
    codegen(&unit, outf);
    fflush(outf);

    String qbeCommand = str_printf("qbe %.*s -o %.*s", strf(qbeFile), strf(asmFile));
    system(qbeCommand.data);
    remove(qbeFile.data);

    String asmCommand = str_printf("gcc %.*s -o %s", strf(asmFile), fileNameNoExt);
    system(asmCommand.data);
    remove(asmFile.data);

    // printf("compiling done!\n");
}
