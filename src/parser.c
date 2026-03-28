#include "token.h"

typedef struct {
    TokensList input;
    size_t index;
    String fileName;
} Parser;

static bool is_at_end(Parser *p) { return p->index == p->input.len; }

static Token previous(Parser *p) { return p->input.arr[p->index - 1]; }

static bool match_arr(Parser *p, size_t count, const TokenKind *tokens) {
    if (is_at_end(p)) return false;

    for (size_t i = 0; i < count; i++) {
        if (p->input.arr[p->index].kind == tokens[i]) {
            p->index++;
            return true;
        }
    }
    return false;
}

#define match(p, ...) match_arr(p, sizeof((TokenKind[]){__VA_ARGS__}) / sizeof(TokenKind), (TokenKind[]){__VA_ARGS__})

inline static Token peek(Parser *p) {
    if (is_at_end(p)) return (Token){0};
    return p->input.arr[p->index];
}

inline static Token peek_ahead(Parser *p, size_t offset) {
    if (p->index + offset >= p->input.len) return (Token){0};
    return p->input.arr[p->index + offset];
}

__attribute__((__noreturn__)) static void parse_error(Parser *p, const char *msg) {
    fprintf(stderr, "[%.*s:%zu:%zu]: Error: %s\n", (int)p->fileName.len, p->fileName.data, peek(p).line, peek(p).col,
            msg);
    abort();
}

static void parse_warning(Parser *p, const char *msg) {
    fprintf(stderr, "[%.*s:%zu:%zu]: Warning: %s\n", (int)p->fileName.len, p->fileName.data, peek(p).line, peek(p).col,
            msg);
}

static Token expect(Parser *p, TokenKind expected, const char *msg) {
    if (match(p, expected)) return previous(p);
    parse_error(p, msg);
}
