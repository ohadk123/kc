#include "string.h"
#include <stdarg.h>

String str_from_cstr(const char *cstr) {
    size_t len = strlen(cstr);
    return (String){cstr, len};
}

String str_from_slice(const String src, size_t start, size_t end) {
    assert(end <= src.len);
    size_t len = end - start;
    char *data = calloc(len + 1, sizeof(char));
    assert(data);
    strncpy(data, src.data + start, len);
    return (String){data, len};
}

String str_from_file(const char *path) {
    if (!path) ERROR("path is NULL");

    FILE *fp;
    long start, end;

    if ((fp = fopen(path, "r")) == NULL) ERROR("Can't open file: \"%s\"", path);
    if (fseek(fp, 0, SEEK_SET) != 0) ERROR("fseek failed");
    if ((start = ftell(fp)) == -1) ERROR("ftell failed");
    if (fseek(fp, 0, SEEK_END) != 0) ERROR("fseek failed");
    if ((end = ftell(fp)) < start) ERROR("ftell failed");
    if (fseek(fp, 0, SEEK_SET) != 0) ERROR("fseek failed");

    size_t count = end - start;
    char *data = calloc(count + 1, sizeof(char));
    assert(data);

    assert(fread(data, sizeof(char), count, fp) == count);
    fclose(fp);

    return (String){data, count};
}

bool cmp_cstr(const String a, const char *b) {
    if (a.len != strlen(b)) return false;
    return strncmp(a.data, b, a.len) == 0;
}

bool cmp_str(const String a, const String b) {
    if (a.len != b.len) return false;

    return strncmp(a.data, b.data, a.len) == 0;
}

String finish_string(const StringBuilder *sb) {
    char *data = calloc(sb->len + 1, sizeof(char));
    strncpy(data, sb->arr, sb->len);
    return (String){data, sb->len};
}

String str_printf(const char *fmt, ...) {
    va_list args, args_copy;
    va_start(args, fmt);
    va_copy(args_copy, args);
    int len = vsnprintf(NULL, 0, fmt, args);
    va_end(args);
    char *buf = calloc(len + 1, sizeof(char));
    vsnprintf(buf, len + 1, fmt, args_copy);
    va_end(args_copy);
    return (String){
        .data = buf,
        .len = len,
    };
}
