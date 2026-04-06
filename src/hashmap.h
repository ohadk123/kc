#ifndef HASHMAP_H
#define HASHMAP_H

#include "string.h"
#include <stddef.h>

typedef void *Val;

typedef struct {
    String key;
    Val val;
} HM_Entry;

typedef struct {
    HM_Entry *data;
    size_t count;
    size_t cap;
} HashMap;

// returns false if key already in hm, true if inserted correctly
bool hm_insert(HashMap *hm, String key, Val val);

// returns the val stored by key, NULL if key not in hm
Val hm_find_val(HashMap *hm, String key);

// returns val pointed to by key, NULL if key not in hm
Val hm_get_or_insert(HashMap *hm, String key, Val val);

#endif // !HASHMAP_H
