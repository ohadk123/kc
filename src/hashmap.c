#include "hashmap.h"

#define MIN_CAP 8
#define EMPTY_ENTRY(e) (e).key.data == NULL

static uint64_t fnv_hash(uint8_t *buf, size_t len) {
    uint64_t hash = 0xcbf29ce484222325;
    for (size_t i = 0; i < len; i++) {
        hash *= 0x100000001b3;
        hash ^= (uint8_t) buf[i];
    }
    return hash;
}

static size_t hash_index(HashMap *hm, String key) {
    assert(hm && "Invalid input");

    return fnv_hash((uint8_t *) key.data, key.len) % hm->cap;
}

Val hm_find_val(HashMap *hm, String key) {
    if (!hm || !key.data || key.len == 0) return NULL;
    if (hm->count == 0) return NULL;

    size_t index = hash_index(hm, key);
    if (hm->data[index].key.data == NULL) return NULL;

    size_t original = index;
    while (!cmp_str(hm->data[index].key, key)) {
        index = (index + 1) % hm->cap;
        if (index == original) return NULL;                // Made entire scan of the hashmap
        if (hm->data[index].key.data == NULL) return NULL; // Found empty spot before finding key, not in hashmap
    }

    return hm->data[index].val;
}

// returns true when inserted, false if it exists already
static bool force_insert(HashMap *hm, String key, Val val) {
    size_t index    = hash_index(hm, key);
    size_t original = index;
    while (hm->data[index].key.data != NULL) {               // Find empty spot
        if (cmp_str(hm->data[index].key, key)) return false; // Already in map

        index = (index + 1) % hm->cap;
        if (index == original) UNREACHABLE("force_insert called on full hashmap");
    }

    hm->data[index] = (HM_Entry){.key = key, .val = val};
    hm->count++;
    return true;
}

bool hm_insert(HashMap *hm, String key, Val val) {
    assert(val); // Don't allow NULL values
    assert(hm);

    if (hm->count * 4 >= hm->cap * 3) {
        HashMap newMap = {0};
        newMap.cap     = hm->cap == 0 ? MIN_CAP : hm->cap * 2;
        newMap.data    = calloc(newMap.cap, sizeof(HM_Entry));
        assert(newMap.data);

        for (size_t i = 0; i < hm->cap; i++) {
            HM_Entry e = hm->data[i];
            if (!(EMPTY_ENTRY(e))) force_insert(&newMap, e.key, e.val);
        }

        free(hm->data);
        *hm = newMap;
    }
    return force_insert(hm, key, val);
}

Val hm_get_or_insert(HashMap *hm, String key, Val val) {
    if (hm_insert(hm, key, val)) return NULL;
    return hm_find_val(hm, key);
}
