#ifndef UTILS_H
#define UTILS_H

#include <assert.h>
#include <execinfo.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TODO(fmt, ...)                                                                \
    do {                                                                              \
        fprintf(stderr, "%s:%d: TODO: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
        abort();                                                                      \
    } while (0)

#define UNREACHABLE(fmt, ...)                                                                \
    do {                                                                                     \
        fprintf(stderr, "%s:%d: UNREACHABLE: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
        abort();                                                                             \
    } while (0)

#define ERROR(fmt, ...)                                                                \
    do {                                                                               \
        fprintf(stderr, "%s:%d: ERROR: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
        abort();                                                                       \
    } while (0)

#define BT_SIZE 20
#define BACKTRACE()                                                      \
    do {                                                                 \
        void *__bt_array[BT_SIZE];                                       \
        size_t __bt_size;                                                \
        __bt_size = backtrace(__bt_array, BT_SIZE);                      \
        char **__bt__strings = backtrace_symbols(__bt_array, __bt_size); \
        printf("Backtrace (%zd frames):\n", __bt_size);                  \
        for (size_t i = 0; i < __bt_size; i++) {                         \
            printf("%s\n", __bt__strings[i]);                            \
            \                                                            \
        }                                                                \
        free(__bt__strings);                                             \
    } while (0);

#define LIST_FIELDS(type) \
    type *arr;            \
    size_t cap;           \
    size_t len

#define MIN_CAP 8

#define list_append(l, e)                                               \
    do {                                                                \
        if ((l)->len >= (l)->cap) {                                     \
            (l)->cap = (l)->cap < MIN_CAP ? MIN_CAP : (l)->cap * 2;     \
            (l)->arr = realloc((l)->arr, (l)->cap * sizeof(*(l)->arr)); \
        }                                                               \
        (l)->arr[(l)->len++] = e;                                       \
    } while (0)

#define list_append_many(l, arr, arrLen)                                \
    do {                                                                \
        while ((l)->len + (arrLen) >= (l)->cap) {                       \
            (l)->cap = (l)->cap < MIN_CAP ? MIN_CAP : (l)->cap * 2;     \
            (l)->arr = realloc((l)->arr, (l)->cap * sizeof(*(l)->arr)); \
        }                                                               \
        for (size_t __i = 0; __i < (arrLen); __i++) {                   \
            (l)->arr[(l)->len++] = (arr)[__i];                          \
        }                                                               \
    } while (0)

#define FOR_EACH(l, T, v) \
    for (T *v = (l)->arr; v < (l)->arr + (l)->len; v++)

#endif // UTILS_H
