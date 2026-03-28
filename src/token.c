#include "token.h"
#include <assert.h>
#include <stdio.h>

#define X(type) [type] = #type,
static const char *tokenTypesStrings[] = {TOKEN_LIST};
#undef X

TokenKind match_keyword_or_ident(String keyword) {
    assert(keyword.len != 0);

    switch (keyword.data[0]) {
        case 'a':
            if (cmp_cstr(keyword, "alias"))    return TOK_ALIAS;
            break;
        case 'b':
            if (cmp_cstr(keyword, "bool"))     return TOK_BOOL;
            if (cmp_cstr(keyword, "break"))    return TOK_BREAK;
            break;
        case 'c':
            if (cmp_cstr(keyword, "cast"))     return TOK_CAST;
            if (cmp_cstr(keyword, "const"))    return TOK_CONST;
            if (cmp_cstr(keyword, "continue")) return TOK_CONTINUE;
            break;
        case 'd':
            if (cmp_cstr(keyword, "do"))       return TOK_DO;
            break;
        case 'e':
            if (cmp_cstr(keyword, "else"))     return TOK_ELSE;
            if (cmp_cstr(keyword, "enum"))     return TOK_ENUM;
            if (cmp_cstr(keyword, "extern"))   return TOK_EXTERN;
            break;
        case 'f':
            if (cmp_cstr(keyword, "f32"))      return TOK_F32;
            if (cmp_cstr(keyword, "f64"))      return TOK_F64;
            if (cmp_cstr(keyword, "false"))    return TOK_FALSE;
            if (cmp_cstr(keyword, "for"))      return TOK_FOR;
            if (cmp_cstr(keyword, "foreach"))  return TOK_FOREACH;
            break;
        case 'i':
            if (cmp_cstr(keyword, "i16"))      return TOK_I16;
            if (cmp_cstr(keyword, "i32"))      return TOK_I32;
            if (cmp_cstr(keyword, "i64"))      return TOK_I64;
            if (cmp_cstr(keyword, "i8"))       return TOK_I8;
            if (cmp_cstr(keyword, "if"))       return TOK_IF;
            if (cmp_cstr(keyword, "import"))   return TOK_IMPORT;
            if (cmp_cstr(keyword, "inline"))   return TOK_INLINE;
            if (cmp_cstr(keyword, "isize"))    return TOK_ISIZE;
            break;
        case 'm':
            if (cmp_cstr(keyword, "match"))    return TOK_MATCH;
            if (cmp_cstr(keyword, "module"))   return TOK_MODULE;
            break;
        case 'p':
            if (cmp_cstr(keyword, "pub"))      return TOK_PUB;
            break;
        case 'r':
            if (cmp_cstr(keyword, "return"))   return TOK_RETURN;
            break;
        case 's':
            if (cmp_cstr(keyword, "static"))   return TOK_STATIC;
            if (cmp_cstr(keyword, "struct"))   return TOK_STRUCT;
            break;
        case 't':
            if (cmp_cstr(keyword, "this"))     return TOK_THIS;
            if (cmp_cstr(keyword, "true"))     return TOK_TRUE;
            if (cmp_cstr(keyword, "typedef"))  return TOK_TYPEDEF;
            break;
        case 'u':
            if (cmp_cstr(keyword, "u16"))      return TOK_U16;
            if (cmp_cstr(keyword, "u32"))      return TOK_U32;
            if (cmp_cstr(keyword, "u64"))      return TOK_U64;
            if (cmp_cstr(keyword, "u8"))       return TOK_U8;
            if (cmp_cstr(keyword, "union"))    return TOK_UNION;
            if (cmp_cstr(keyword, "usize"))    return TOK_USIZE;
            break;
        case 'v':
            if (cmp_cstr(keyword, "variant"))  return TOK_VARIANT;
            if (cmp_cstr(keyword, "void"))     return TOK_VOID;
            break;
        case 'w':
            if (cmp_cstr(keyword, "while"))    return TOK_WHILE;
            break;
    }

    return TOK_IDENTIFIER;
}

Token tok_make_simple(TokenKind type, size_t line, size_t col) {
    return (Token){
        .kind = type,
        .line = line,
        .col = col,
    };
}

Token tok_make_unknown(char c, size_t line, size_t col) {
    return (Token){
        .kind = TOK_UNKNOWN,
        .as.unknown = c,
        .line = line,
        .col = col,
    };
}

Token tok_make_ident(String ident, size_t line, size_t col) {
    return (Token){
        .kind = TOK_IDENTIFIER,
        .as.identifier = ident,
        .line = line,
        .col = col,
    };
}

Token tok_make_string_lit(String strLit, size_t line, size_t col) {
    return (Token){
        .kind = TOK_STRING_LITERAL,
        .as.stringLiteral = strLit,
        .line = line,
        .col = col,
    };
}

Token tok_make_int_lit(uint64_t value, size_t line, size_t col) {
    return (Token){
        .kind = TOK_INTEGER_LITERAL,
        .as.integerLiteral = value,
        .line = line,
        .col = col,
    };
}

Token tok_make_float_lit(double value, size_t line, size_t col) {
    return (Token){
        .kind = TOK_FLOAT_LITERAL,
        .as.floatLiteral = value,
        .line = line,
        .col = col,
    };
}

Token tok_make_char_lit(uint8_t value, size_t line, size_t col) {
    return (Token){
        .kind = TOK_CHAR_LITERAL,
        .as.charLiteral = value,
        .line = line,
        .col = col,
    };
}

static bool is_printable_char(char c) { return ' ' <= c && c <= '~'; }

void print_tok(Token token) {
    printf("{ [%zu:%zu] \"type\": \"%s\"", token.line, token.col, tokenTypesStrings[token.kind]);
    switch (token.kind) {
        case TOK_IDENTIFIER:
            printf(", \"name\": \"%.*s\"", (int)token.as.identifier.len, token.as.identifier.data);
            break;
        case TOK_STRING_LITERAL:
            printf(", \"value\": \"%.*s\"", (int)token.as.stringLiteral.len, token.as.stringLiteral.data);
            break;
        case TOK_CHAR_LITERAL:
            if (is_printable_char(token.as.charLiteral))
                printf(", \"value\": \"%c\"", token.as.charLiteral);
            else
                printf(", \"value\": \"0x%02x\"", token.as.charLiteral);
            break;
        case TOK_INTEGER_LITERAL: printf(", \"value\": %zu", token.as.integerLiteral); break;
        case TOK_FLOAT_LITERAL:   printf(", \"value\": %f", token.as.floatLiteral); break;
        case TOK_UNKNOWN:
            if (is_printable_char(token.as.unknown))
                printf(", \"value\": \"%c\"", token.as.unknown);
            else
                printf(", \"value\": \"0x%02x\"", token.as.unknown);
            break;
        default: break;
    }
    printf(" }\n");
}
