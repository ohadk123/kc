#ifndef SEMA_H
#define SEMA_H

#include "hashmap.h"
#include "statement.h"
typedef struct _TranslationUnit TranslationUnit;
typedef struct _Scope Scope;
struct _Scope {
    HashMap curr;
    Scope *above;
};

void fill_global_symbol_table(TranslationUnit *unit);
bool add_symbol(Scope *curr, String symbol, Val val);
Stmt *get_symbol(Scope *curr, String symbol);

// returns NULL if symbol not in scope bottom or it's parents
Val get_or_insert_symbol(Scope *bottom, String symbol, Stmt *decl);

#endif // !SEMA_H
