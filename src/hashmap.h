#ifndef HASHMAP_H
#define HASHMAP_H

#include "string.h"
#include <stddef.h>

typedef void * Val;

typedef struct {
    String key;
    Val val;
} HM_Entry;

typedef struct {
    HM_Entry *data;
    size_t count;
    size_t cap;
} HashMap;

typedef void *Val;

bool hm_insert(HashMap *hm, String key, Val val);
Val hm_find_val(HashMap *hm, String key);

#endif // !HASHMAP_H
