// TODO: better error handling

#include "lexer.h"
#include <ctype.h>

static inline bool is_hex(char c) { return ('0' <= c && c <= '9') || ('a' <= c && c <= 'f') || ('A' <= c && c <= 'F'); }

static inline char hex_to_val(char c) {
    if ('0' <= c && c <= '9') return c - '0' + 0x0;
    if ('a' <= c && c <= 'f') return c - 'a' + 0xa;
    if ('A' <= c && c <= 'F') return c - 'A' + 0xA;

    return -1;
}

static inline bool is_oct(char c) { return '0' <= c && c <= '7'; }

static inline char oct_to_val(char c) {
    if (is_oct(c)) return c - '0';

    return -1;
}

typedef struct {
    TranslationUnit *unit;
    size_t line;
    size_t col;
    size_t index;
    bool hadError;
} Lexer;

#define lexer_error(l, fmt, ...) \
    ((l)->hadError = compile_err_no_abort((l)->unit->fileName, (Location){(l)->line, (l)->col}, fmt, ##__VA_ARGS__), (Token){0})

static inline void add_tok(Lexer *l, Token token) { list_append(&l->unit->tokens, token); }

static void add_simple_tok(Lexer *l, TokenKind type) {
    Token t = tok_make_simple(type, l->line, l->col);
    add_tok(l, t);
}

// Check if we've reached the end of the input
static bool is_at_end(Lexer *l) { return l->index == l->unit->input.len; }

// Advance the lexer and return the current character
static char advance(Lexer *l) {
    if (is_at_end(l)) return 0;
    l->col++;
    return l->unit->input.data[l->index++];
}

// Check if the next character matches the expected character and advance if it does
static bool match(Lexer *l, char next) {
    if (is_at_end(l)) return false;
    if (l->unit->input.data[l->index] != next) return false;

    l->index++;
    l->col++;
    return true;
}

// Peek at the current character without advancing
static char peek(Lexer *l) {
    if (is_at_end(l)) return 0;
    return l->unit->input.data[l->index];
}

static Token make_ident_keyword(Lexer *l) {
    size_t start = l->index - 1;
    size_t col = l->col;

    while (isalnum(peek(l)) || peek(l) == '_') advance(l);
    size_t end = l->index;

    String identString = (String){l->unit->input.data + start, end - start};

    TokenKind tokenKind = match_keyword_or_ident(identString);
    if (tokenKind == TOK_IDENTIFIER) return tok_make_ident(str_from_slice(l->unit->input, start, end), l->line, col);

    return tok_make_simple(tokenKind, l->line, col);
}

static Token make_number(Lexer *l) {
    size_t start = l->index - 1;
    size_t col   = l->col;
    const char *nptr = l->unit->input.data + start;

    char *end;
    uint64_t val = strtoull(nptr, &end, 0);
    if (end <= nptr) return lexer_error(l, "Invalid number");
    l->index += end - nptr - 1;
    l->col += end - nptr - 1;

    bool isLong = match(l, 'L');
    if (!isLong && val > INT32_MAX) return lexer_error(l, "Integer literal does not fit in i32");
    if (isLong && val > INT64_MAX) return lexer_error(l, "Integer literal does not fit in i64");

    return isLong ? tok_make_long_lit(val, l->line, col) : tok_make_int_lit(val, l->line, col);
}

static uint8_t consume_escape_char(Lexer *l) {
    uint8_t prev = l->unit->input.data[l->index - 1];
    if (prev != '\\') ERROR("Previous character is not '\\' (%X)", prev);
    uint64_t val = 0;

    char c = advance(l);
    switch (c) {
        case 'n':  val = '\n'; break;
        case 't':  val = '\t'; break;
        case 'v':  val = '\v'; break;
        case 'b':  val = '\b'; break;
        case 'r':  val = '\r'; break;
        case 'f':  val = '\f'; break;
        case 'a':  val = '\a'; break;
        case '\\': val = '\\'; break;
        case '?':  val = '\?'; break;
        case '\'': val = '\''; break;
        case '\"': val = '\"'; break;
        case 'x':
            while (is_hex(peek(l))) {
                char c = advance(l);
                val *= 16;
                char o = hex_to_val(c);
                if (o == -1) {
                    lexer_error(l, "'%c' is Not a hex character", c);
                    o = 0;
                }
                val += o;
            }
            break;
        case '0':
            while (is_oct(peek(l))) {
                char c = advance(l);
                val *= 8;
                char o = oct_to_val(c);
                if (o == -1) {
                    lexer_error(l, "'%c' Not an octal val", c);
                    o = 0;
                }
                val += o;
            }
            break;
    }

    if (val > UINT8_MAX) TODO("Hex escape sequence out of range");
    return (uint8_t)val;
}

static Token make_string(Lexer *l) {
    StringBuilder string = {0};
    size_t col = l->col;

    while (!is_at_end(l) && peek(l) != '\n') {
        if (peek(l) == '\"') {
            advance(l);
            goto terminated;
        }

        char c = advance(l);
        if (c == '\\') c = consume_escape_char(l);
        list_append(&string, c);
    }
    TODO("Unterminated string literal");

terminated:
    return tok_make_string_lit(finish_string(&string), l->line, col);
}

static Token make_char(Lexer *l) {
    size_t col = l->col;
    uint8_t c = advance(l);

    if (c == '\\') c = consume_escape_char(l);
    if (peek(l) != '\'') TODO("Unterminated char literal");
    advance(l); // Consume temrminating '

    return tok_make_char_lit(c, l->line, col);
}

#define ADD_SIMPLE(type) add_simple_tok(&l, type)

/**********************************************************************************************************************
 * Public Lexer API
 *********************************************************************************************************************/

bool scan_file(TranslationUnit *unit) {
    if (!unit) return false;

    Lexer l = (Lexer){
        .unit = unit,
        .index = 0,
        .line = 1,
        .col = 0,
        .hadError = false,
    };

    while (!is_at_end(&l)) {
        char c = advance(&l);

        if (isalpha(c) || c == '_') {
            add_tok(&l, make_ident_keyword(&l));
            continue;
        }

        if (isdigit(c) || (c == '.' && isdigit(peek(&l)))) {
            add_tok(&l, make_number(&l));
            continue;
        }

        if (c == '\"') {
            add_tok(&l, make_string(&l));
            continue;
        }

        if (c == '\'') {
            add_tok(&l, make_char(&l));
            continue;
        }

        switch (c) {
            case '\n': l.line++; l.col = 0;
            case ' ':
            case '\r':
            case '\t': break;
            // TOK_EQUALS,
            // TOK_EQUALS_GREATER
            // TOK_EQUAL_EQUALS,
            case '=':
                if (match(&l, '='))
                    ADD_SIMPLE(TOK_EQUALS_EQUALS);
                else if (match(&l, '>'))
                    ADD_SIMPLE(TOK_EQUALS_GREATER);
                else
                    ADD_SIMPLE(TOK_EQUALS);
                break;
            // TOK_PLUS,
            // TOK_PLUS_PLUS,
            // TOK_PLUS_EQUALS,
            case '+':
                if (match(&l, '='))
                    ADD_SIMPLE(TOK_PLUS_EQUALS);
                else if (match(&l, '+'))
                    ADD_SIMPLE(TOK_PLUS_PLUS);
                else
                    ADD_SIMPLE(TOK_PLUS);
                break;
            // TOK_MINUS,
            // TOK_MINUS_MINUS,
            // TOK_MINUS_EQUALS,
            // TOK_MINUS_GREATER,
            case '-':
                if (match(&l, '='))
                    ADD_SIMPLE(TOK_MINUS_EQUALS);
                else if (match(&l, '-'))
                    ADD_SIMPLE(TOK_MINUS_MINUS);
                else if (match(&l, '>'))
                    ADD_SIMPLE(TOK_MINUS_GREATER);
                else
                    ADD_SIMPLE(TOK_MINUS);
                break;
            // TOK_STAR,
            // TOK_STAR_EQUALS,
            case '*':
                if (match(&l, '='))
                    ADD_SIMPLE(TOK_STAR_EQUALS);
                else
                    ADD_SIMPLE(TOK_STAR);
                break;
            // TOK_SLASH,
            // TOK_SLASH_EQUALS,
            case '/':
                if (match(&l, '*')) {
                    while (true) {
                        if (match(&l, '*') && match(&l, '/')) break;
                        c = advance(&l);
                        if (c == '\n') l.line++;
                    }
                    break;
                }

                if (match(&l, '/'))
                    while (!is_at_end(&l) && peek(&l) != '\n') advance(&l);
                else if (match(&l, '='))
                    ADD_SIMPLE(TOK_SLASH_EQUALS);
                else
                    ADD_SIMPLE(TOK_SLASH);
                break;
            // TOK_PERCENT,
            // TOK_PERCENT_EQUALS,
            case '%':
                if (match(&l, '='))
                    ADD_SIMPLE(TOK_PERCENT_EQUALS);
                else
                    ADD_SIMPLE(TOK_PERCENT);
                break;
            // TOK_BANG,
            // TOK_BANG_EQUALS,
            case '!':
                if (match(&l, '='))
                    ADD_SIMPLE(TOK_BANG_EQUALS);
                else
                    ADD_SIMPLE(TOK_BANG);
                break;
            // TOK_LESS,
            // TOK_LESS_EQUALS,
            // TOK_LESS_LESS,
            // TOK_LESS_LESS_EQUALS,
            case '<':
                if (match(&l, '='))
                    ADD_SIMPLE(TOK_LESS_EQUALS);
                else if (match(&l, '<')) {
                    if (match(&l, '='))
                        ADD_SIMPLE(TOK_LESS_LESS_EQUALS);
                    else
                        ADD_SIMPLE(TOK_LESS_LESS);
                } else
                    ADD_SIMPLE(TOK_LESS);
                break;
            // TOK_GREATER,
            // TOK_GREATER_EQUALS,
            // TOK_GREATER_GREATER,
            // TOK_GREATER_GREATER_EQUALS,
            case '>':
                if (match(&l, '='))
                    ADD_SIMPLE(TOK_GREATER_EQUALS);
                else if (match(&l, '>')) {
                    if (match(&l, '='))
                        ADD_SIMPLE(TOK_GREATER_GREATER_EQUALS);
                    else
                        ADD_SIMPLE(TOK_GREATER_GREATER);
                } else
                    ADD_SIMPLE(TOK_GREATER);
                break;
            // TOK_AMPERSAND,
            // TOK_AMPERSAND_AMPERSAND,
            // TOK_AMPERSAND_EQUALS,
            case '&':
                if (match(&l, '='))
                    ADD_SIMPLE(TOK_AMPERSAND_EQUALS);
                else if (match(&l, '&'))
                    ADD_SIMPLE(TOK_AMPERSAND_AMPERSAND);
                else
                    ADD_SIMPLE(TOK_AMPERSAND);
                break;
            // TOK_PIPE,
            // TOK_PIPE_PIPE,
            // TOK_PIPE_EQUALS,
            case '|':
                if (match(&l, '='))
                    ADD_SIMPLE(TOK_PIPE_EQUALS);
                else if (match(&l, '|'))
                    ADD_SIMPLE(TOK_PIPE_PIPE);
                else
                    ADD_SIMPLE(TOK_PIPE);
                break;
            // TOK_CARET,
            // TOK_CARET_EQUALS,
            case '^':
                if (match(&l, '='))
                    ADD_SIMPLE(TOK_CARET_EQUALS);
                else
                    ADD_SIMPLE(TOK_CARET);
                break;
            // TOK_TILDE,
            case '~': ADD_SIMPLE(TOK_TILDE); break;
            // TOK_LEFT_PAREN,
            case '(': ADD_SIMPLE(TOK_LEFT_PAREN); break;
            // TOK_RIGHT_PAREN,
            case ')': ADD_SIMPLE(TOK_RIGHT_PAREN); break;
            // TOK_LEFT_BRACE,
            case '{': ADD_SIMPLE(TOK_LEFT_BRACE); break;
            // TOK_RIGHT_BRACE,
            case '}': ADD_SIMPLE(TOK_RIGHT_BRACE); break;
            // TOK_LEFT_BRACKET,
            case '[': ADD_SIMPLE(TOK_LEFT_BRACKET); break;
            // TOK_RIGHT_BRACKET,
            case ']': ADD_SIMPLE(TOK_RIGHT_BRACKET); break;
            // TOK_COMMA,
            case ',': ADD_SIMPLE(TOK_COMMA); break;
            // TOK_DOT,
            // TOK_DOT_DOT,
            // TOK_ELLIPSIS,
            case '.':
                if (match(&l, '.')) {
                    if (match(&l, '.'))
                        ADD_SIMPLE(TOK_ELLIPSIS);
                    else
                        ADD_SIMPLE(TOK_DOT_DOT);
                } else
                    ADD_SIMPLE(TOK_DOT);
                break;
            // TOK_SEMICOLON,
            case ';': ADD_SIMPLE(TOK_SEMICOLON); break;
            // TOK_COLON,
            // TOK_COLON_COLON,
            case ':':
                if (match(&l, ':'))
                    ADD_SIMPLE(TOK_COLON_COLON);
                else
                    ADD_SIMPLE(TOK_COLON);
                break;
            // TOK_QUESTION_MARK,
            case '?': ADD_SIMPLE(TOK_QUESTION_MARK); break;
            // TOK_AT
            case '@': ADD_SIMPLE(TOK_AT); break;

            default: lexer_error(&l, "Unknown character '%c'", c); break;
        }
    }

    add_simple_tok(&l, TOK_EOF);

    free((void *)l.unit->input.data);

    return !l.hadError;
}

void free_token_list(TokensList *tokens) {
    for (size_t i = 0; i < tokens->len; i++) {
        Token *tok = &tokens->arr[i];
        switch (tok->kind) {
            case TOK_IDENTIFIER:     free((void *)tok->as.identifier.data); break;
            case TOK_STRING_LITERAL: free((void *)tok->as.stringLiteral.data); break;
            default:                 break;
        }
    }
    free(tokens->arr);
    tokens->arr = NULL;
    tokens->len = 0;
    tokens->cap = 0;
}
