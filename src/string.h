#ifndef STRING_H
#define STRING_H

#include "utils.h"

typedef struct {
    const char *data; // Null terminated string
    size_t len;
} String;

String str_from_cstr(const char *cstr);
String str_from_slice(const String src, size_t start, size_t end);
String str_from_file(const char *path);

bool cmp_cstr(const String a, const char *b);

typedef struct {
    LIST_FIELDS(char);
} StringBuilder;

String finish_string(const StringBuilder *sb);

#endif // STRING_H
