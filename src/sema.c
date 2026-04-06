#include "sema.h"
#include "compiler.h"

void fill_global_symbol_table(TranslationUnit *unit) {
    for (size_t i = 0; i < unit->ast.len; i++) {
        Stmt *s = unit->ast.arr[i];
        Token nameTok = get_top_level_name(s);
        assert(nameTok.kind == TOK_IDENTIFIER);
        String key = nameTok.as.identifier;

        if (!hm_insert(&unit->globalSymbolTable.curr, key, s)) {
            Stmt *first = hm_find_val(&unit->globalSymbolTable.curr, key);
            compile_error(unit->fileName, s->loc, "symbol \"%.*s\" already delcared before at [%.*s:%zu:%zu]", (int) key.len, key.data, (int) unit->fileName.len, unit->fileName.data, first->loc.line, first->loc.col);
        }
    }
}

// if symbol already declared, return the scope it's declared at, NULL otherwise
Scope *symbol_exists(Scope *bottom, String symbol) {
    while (bottom && hm_find_val(&bottom->curr, symbol)) bottom = bottom->above;
    return bottom;
}

Val get_or_insert_symbol(Scope *bottom, String symbol, Stmt *decl) {
    Scope *found = symbol_exists(bottom, symbol);
    if (found) return hm_find_val(&found->curr, symbol);
    assert(hm_insert(&bottom->curr, symbol, decl));
    return NULL;
}

bool add_symbol(Scope *curr, String symbol, Val val) {
    if (symbol_exists(curr, symbol)) return false;
    return hm_insert(&curr->curr, symbol, val);
}

Stmt *get_symbol(Scope *bottom, String symbol) {
    do {
        Val val = hm_find_val(&bottom->curr, symbol);
        if (val) return val;
        bottom = bottom->above;
    } while (bottom);
    return NULL;
}
