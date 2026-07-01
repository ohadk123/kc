#ifndef TOKEN_H
#define TOKEN_H

#include "string.h"

#define TOKEN_LIST                          \
    X(TOK_UNKNOWN)                          \
    X(TOK_EOF)                              \
                                            \
    X(TOK_EQUALS)         /* =  */          \
    X(TOK_PLUS)           /* +  */          \
    X(TOK_PLUS_PLUS)      /* ++ */          \
    X(TOK_PLUS_EQUALS)    /* += */          \
    X(TOK_MINUS)          /* -  */          \
    X(TOK_MINUS_MINUS)    /* -- */          \
    X(TOK_MINUS_EQUALS)   /* -= */          \
    X(TOK_STAR)           /* *  */          \
    X(TOK_STAR_EQUALS)    /* *= */          \
    X(TOK_SLASH)          /* /  */          \
    X(TOK_SLASH_EQUALS)   /* /= */          \
    X(TOK_PERCENT)        /* %  */          \
    X(TOK_PERCENT_EQUALS) /* %= */          \
                                            \
    X(TOK_EQUALS_EQUALS)       /* == */     \
    X(TOK_BANG)                /* !  */     \
    X(TOK_BANG_EQUALS)         /* != */     \
    X(TOK_LESS)                /* <  */     \
    X(TOK_LESS_EQUALS)         /* <= */     \
    X(TOK_GREATER)             /* >  */     \
    X(TOK_GREATER_EQUALS)      /* >= */     \
    X(TOK_AMPERSAND_AMPERSAND) /* && */     \
    X(TOK_PIPE_PIPE)           /* || */     \
                                            \
    X(TOK_AMPERSAND)              /* &   */ \
    X(TOK_AMPERSAND_EQUALS)       /* &=  */ \
    X(TOK_PIPE)                   /* |   */ \
    X(TOK_PIPE_EQUALS)            /* |=  */ \
    X(TOK_CARET)                  /* ^   */ \
    X(TOK_CARET_EQUALS)           /* ^=  */ \
    X(TOK_TILDE)                  /* ~   */ \
    X(TOK_LESS_LESS)              /* <<  */ \
    X(TOK_LESS_LESS_EQUALS)       /* <<= */ \
    X(TOK_GREATER_GREATER)        /* >>  */ \
    X(TOK_GREATER_GREATER_EQUALS) /* >>= */ \
                                            \
    X(TOK_LEFT_PAREN)    /* ( */            \
    X(TOK_RIGHT_PAREN)   /* ) */            \
    X(TOK_LEFT_BRACE)    /* { */            \
    X(TOK_RIGHT_BRACE)   /* } */            \
    X(TOK_LEFT_BRACKET)  /* [ */            \
    X(TOK_RIGHT_BRACKET) /* ] */            \
                                            \
    X(TOK_COMMA)          /* ,   */         \
    X(TOK_DOT)            /* .   */         \
    X(TOK_DOT_DOT)        /* ..  */         \
    X(TOK_ELLIPSIS)       /* ... */         \
    X(TOK_SEMICOLON)      /* ;   */         \
    X(TOK_COLON)          /* :   */         \
    X(TOK_COLON_COLON)    /* ::  */         \
    X(TOK_QUESTION_MARK)  /* ?   */         \
    X(TOK_MINUS_GREATER)  /* ->  */         \
    X(TOK_EQUALS_GREATER) /* =>  */         \
    X(TOK_AT)             /* @   */         \
    X(TOK_UNDERSCOE)      /* _   */         \
                                            \
    X(TOK_IDENTIFIER)                       \
    X(TOK_STRING_LITERAL)                   \
    X(TOK_CHAR_LITERAL)                     \
    X(TOK_INTEGER_LITERAL)                  \
    X(TOK_LONG_LITERAL)                     \
    X(TOK_FLOAT_LITERAL)                    \
    X(TOK_DOUBLE_LITERAL)                   \
                                            \
    X(TOK_BREAK)    /* break    */          \
    X(TOK_CASE)     /* case     */          \
    X(TOK_CAST)     /* cast     */          \
    X(TOK_CONST)    /* const    */          \
    X(TOK_CONTINUE) /* continue */          \
    X(TOK_DO)       /* do       */          \
    X(TOK_ELSE)     /* else     */          \
    X(TOK_ENUM)     /* enum     */          \
    X(TOK_EXTERN)   /* extern   */          \
    X(TOK_FOR)      /* for      */          \
    X(TOK_FOREACH)  /* foreach  */          \
    X(TOK_IF)       /* if       */          \
    X(TOK_IMPORT)   /* import   */          \
    X(TOK_INLINE)   /* inline   */          \
    X(TOK_MATCH)    /* match    */          \
    X(TOK_MODULE)   /* module   */          \
    X(TOK_PUB)      /* pub      */          \
    X(TOK_RETURN)   /* return   */          \
    X(TOK_STATIC)   /* static   */          \
    X(TOK_STRUCT)   /* struct   */          \
    X(TOK_SWITCH)   /* switch   */          \
    X(TOK_THIS)     /* this     */          \
    X(TOK_UNION)    /* union    */          \
    X(TOK_VARIANT)  /* variant  */          \
    X(TOK_WHILE)    /* while    */          \
    X(TOK_TYPEDEF)  /* typedef  */          \
    X(TOK_ALIAS)    /* alias    */          \
                                            \
    X(TOK_VOID)  /* void  */                \
    X(TOK_U8)    /* u8    */                \
    X(TOK_U16)   /* u16   */                \
    X(TOK_U32)   /* u32   */                \
    X(TOK_U64)   /* u64   */                \
    X(TOK_USIZE) /* usize */                \
    X(TOK_ISIZE) /* isize */                \
    X(TOK_I8)    /* i8    */                \
    X(TOK_I16)   /* i16   */                \
    X(TOK_I32)   /* i32   */                \
    X(TOK_I64)   /* i64   */                \
    X(TOK_F32)   /* f32   */                \
    X(TOK_F64)   /* f64   */                \
    X(TOK_BOOL)  /* bool  */                \
                                            \
    X(TOK_TRUE)  /* true  */                \
    X(TOK_FALSE) /* false */

#define X(type) type,
typedef enum { TOKEN_LIST } TokenKind;
#undef X

typedef struct {
    size_t line;
    size_t col;
} Location;

typedef struct {
    TokenKind kind;
    Location loc;
    union {
        String identifier;
        String stringLiteral;
        uint8_t charLiteral;
        uint32_t integerLiteral;
        uint64_t longLiteral;
        float floatLiteral;
        double doubleLiteral;
        char unknown;
    } as;
} Token;

typedef struct {
    LIST_FIELDS(Token);
} TokensList;

extern const char *tokenTypesStrings[];

Token tok_make_simple(TokenKind type, size_t line, size_t col);
Token tok_make_unknown(char c, size_t line, size_t col);
Token tok_make_ident(String ident, size_t line, size_t col);
Token tok_make_string_lit(String strLit, size_t line, size_t col);
Token tok_make_int_lit(uint32_t value, size_t line, size_t col);
Token tok_make_float_lit(float value, size_t line, size_t col);
Token tok_make_long_lit(uint64_t value, size_t line, size_t col);
Token tok_make_double_lit(double value, size_t line, size_t col);
Token tok_make_char_lit(uint8_t value, size_t line, size_t col);

TokenKind match_keyword_or_ident(String keyword);

void print_tok(Token token);

#endif // TOKEN_H
