#include "hashmap.h"

struct _HM_Entry {
    String key;
    Val val;
};

#define MIN_CAP 8
#define EMPTY_ENTRY(e) (e).key.data == NULL

static uint64_t fnv_hash(uint8_t *buf, size_t len) {
    uint64_t hash = 0xcbf29ce484222325;
    for (size_t i = 0; i < len; i++) {
        hash *= 0x100000001b3;
        hash ^= (uint8_t)buf[i];
    }
    return hash;
}

static size_t hash_index(HashMap *hm, String key) {
    assert(hm && "Invalid input");

    return fnv_hash((uint8_t *)key.data, key.len) % hm->cap;
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
    size_t index = hash_index(hm, key);
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
    assert(hm);

    if (hm->count * 4 >= hm->cap * 3) {
        HashMap newMap = {0};
        newMap.cap = hm->cap == 0 ? MIN_CAP : hm->cap * 2;
        newMap.data = calloc(newMap.cap, sizeof(HM_Entry));
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

// --- test harness ---
static int passed = 0;
static int failed = 0;

#define EXPECT(cond, msg)                                              \
    do {                                                               \
        if (cond) { passed++; }                                        \
        else { fprintf(stderr, "FAIL [%s:%d]: %s\n", __FILE__, __LINE__, msg); failed++; } \
    } while (0)

// --- tests ---

void test_basic_insert_find(void) {
    HashMap hm = {0};
    int v = 42;

    bool inserted = hm_insert(&hm, str_from_cstr("hello"), &v);
    EXPECT(inserted, "insert new key returns true");

    Val found = hm_find_val(&hm, str_from_cstr("hello"));
    EXPECT(found == &v, "find returns correct value");
    EXPECT(*(int *)found == 42, "found value is correct");
}

static void test_duplicate_insert(void) {
    HashMap hm = {0};
    int a = 1, b = 2;

    hm_insert(&hm, str_from_cstr("key"), &a);
    bool second = hm_insert(&hm, str_from_cstr("key"), &b);
    EXPECT(!second, "inserting duplicate key returns false");

    Val found = hm_find_val(&hm, str_from_cstr("key"));
    EXPECT(found == &a, "original value unchanged after duplicate insert");
}

static void test_find_missing(void) {
    HashMap hm = {0};
    int v = 1;
    hm_insert(&hm, str_from_cstr("present"), &v);

    Val found = hm_find_val(&hm, str_from_cstr("absent"));
    EXPECT(found == NULL, "find missing key returns NULL");
}

static void test_find_empty_map(void) {
    HashMap hm = {0};
    Val found = hm_find_val(&hm, str_from_cstr("anything"));
    EXPECT(found == NULL, "find on empty map returns NULL");
}

static void test_multiple_keys(void) {
    HashMap hm = {0};
    int vals[5] = {10, 20, 30, 40, 50};
    const char *keys[5] = {"alpha", "beta", "gamma", "delta", "epsilon"};

    for (int i = 0; i < 5; i++)
        hm_insert(&hm, str_from_cstr(keys[i]), &vals[i]);

    for (int i = 0; i < 5; i++) {
        Val found = hm_find_val(&hm, str_from_cstr(keys[i]));
        EXPECT(found == &vals[i], "each key maps to correct value");
    }
}

static void test_resize(void) {
    // Insert enough entries to trigger resize (load factor > 0.75)
    HashMap hm = {0};
    int vals[20];
    char keys[20][16];

    for (int i = 0; i < 20; i++) {
        vals[i] = i * 7;
        snprintf(keys[i], sizeof(keys[i]), "key_%d", i);
        hm_insert(&hm, str_from_cstr(keys[i]), &vals[i]);
    }

    // Verify all entries still findable after resize(s)
    for (int i = 0; i < 20; i++) {
        Val found = hm_find_val(&hm, str_from_cstr(keys[i]));
        EXPECT(found == &vals[i], "entry survives resize");
    }

    EXPECT(hm.count == 20, "count correct after resize");
}

static void test_similar_keys(void) {
    // Keys that are similar but distinct - exercises collision handling
    HashMap hm = {0};
    int a = 1, b = 2, c = 3;

    hm_insert(&hm, str_from_cstr("abc"), &a);
    hm_insert(&hm, str_from_cstr("abd"), &b);
    hm_insert(&hm, str_from_cstr("abe"), &c);

    EXPECT(hm_find_val(&hm, str_from_cstr("abc")) == &a, "similar key abc");
    EXPECT(hm_find_val(&hm, str_from_cstr("abd")) == &b, "similar key abd");
    EXPECT(hm_find_val(&hm, str_from_cstr("abe")) == &c, "similar key abe");
}

static void test_count(void) {
    HashMap hm = {0};
    int v = 0;

    EXPECT(hm.count == 0, "initial count is 0");
    hm_insert(&hm, str_from_cstr("a"), &v);
    EXPECT(hm.count == 1, "count after one insert");
    hm_insert(&hm, str_from_cstr("b"), &v);
    EXPECT(hm.count == 2, "count after two inserts");
    hm_insert(&hm, str_from_cstr("a"), &v); // duplicate
    EXPECT(hm.count == 2, "count unchanged after duplicate");
}

static void test_empty_value(void) {
    HashMap hm = {0};
    hm_insert(&hm, str_from_cstr("nullval"), NULL);
    Val found = hm_find_val(&hm, str_from_cstr("nullval"));
    EXPECT(found == NULL, "NULL value stored and retrieved");
}

int hm_test_main(void) {
    test_basic_insert_find();
    test_duplicate_insert();
    test_find_missing();
    test_find_empty_map();
    test_multiple_keys();
    test_resize();
    test_similar_keys();
    test_count();
    test_empty_value();

    printf("\n%d passed, %d failed\n", passed, failed);
    return failed > 0 ? 1 : 0;
}
