#include "sema.h"

void fill_global_symbol_table(TranslationUnit *unit) {
    for (size_t i = 0; i < unit->ast.len; i++) {
        Stmt *s = unit->ast.arr[i];
        Token nameTok = get_top_level_name(s);
        assert(nameTok.kind == TOK_IDENTIFIER);
        String key = nameTok.as.identifier;

        if (!hm_insert(&unit->globalSymbolTable, key, s)) {
            Stmt *first = hm_find_val(&unit->globalSymbolTable, key);
            compile_error(unit->fileName, s->loc, "symbol \"%.*s\" already delcared before at [%.*s:%zu:%zu]", (int) key.len, key.data, (int) unit->fileName.len, unit->fileName.data, first->loc.line, first->loc.col);
        }
    }
}
