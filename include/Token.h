#ifndef INCLUDE_KC_TOKEN_H_
#define INCLUDE_KC_TOKEN_H_

#include <libk/String.h>
#include <libk/List.h>

#define TOKEN_LIST                                                                                                     \
    X(TOK_UNKNOWN)                                                                                                     \
    X(TOK_EOF)                                                                                                         \
    X(TOK_ERROR)                                                                                                       \
                                                                                                                       \
    X(TOK_EQUALS)         /* =  */                                                                                     \
    X(TOK_PLUS)           /* +  */                                                                                     \
    X(TOK_PLUS_PLUS)      /* ++ */                                                                                     \
    X(TOK_PLUS_EQUALS)    /* += */                                                                                     \
    X(TOK_MINUS)          /* -  */                                                                                     \
    X(TOK_MINUS_MINUS)    /* -- */                                                                                     \
    X(TOK_MINUS_EQUALS)   /* -= */                                                                                     \
    X(TOK_STAR)           /* *  */                                                                                     \
    X(TOK_STAR_EQUALS)    /* *= */                                                                                     \
    X(TOK_SLASH)          /* /  */                                                                                     \
    X(TOK_SLASH_EQUALS)   /* /= */                                                                                     \
    X(TOK_PERCENT)        /* %  */                                                                                     \
    X(TOK_PERCENT_EQUALS) /* %= */                                                                                     \
                                                                                                                       \
    X(TOK_EQUALS_EQUALS)       /* =  */                                                                                \
    X(TOK_BANG)                /* !  */                                                                                \
    X(TOK_BANG_EQUALS)         /* != */                                                                                \
    X(TOK_LESS)                /* <  */                                                                                \
    X(TOK_LESS_EQUALS)         /* <= */                                                                                \
    X(TOK_GREATER)             /* >  */                                                                                \
    X(TOK_GREATER_EQUALS)      /* >= */                                                                                \
    X(TOK_AMPERSAND_AMPERSAND) /* && */                                                                                \
    X(TOK_PIPE_PIPE)           /* || */                                                                                \
                                                                                                                       \
    X(TOK_AMPERSAND)              /* &   */                                                                            \
    X(TOK_AMPERSAND_EQUALS)       /* &=  */                                                                            \
    X(TOK_PIPE)                   /* |   */                                                                            \
    X(TOK_PIPE_EQUALS)            /* |=  */                                                                            \
    X(TOK_CARET)                  /* ^   */                                                                            \
    X(TOK_CARET_EQUALS)           /* ^=  */                                                                            \
    X(TOK_TILDE)                  /* ~   */                                                                            \
    X(TOK_LESS_LESS)              /* <<  */                                                                            \
    X(TOK_LESS_LESS_EQUALS)       /* <<= */                                                                            \
    X(TOK_GREATER_GREATER)        /* >>  */                                                                            \
    X(TOK_GREATER_GREATER_EQUALS) /* >>= */                                                                            \
                                                                                                                       \
    X(TOK_LEFT_PAREN)    /* ( */                                                                                       \
    X(TOK_RIGHT_PAREN)   /* ) */                                                                                       \
    X(TOK_LEFT_BRACE)    /* { */                                                                                       \
    X(TOK_RIGHT_BRACE)   /* } */                                                                                       \
    X(TOK_LEFT_BRACKET)  /* [ */                                                                                       \
    X(TOK_RIGHT_BRACKET) /* ] */                                                                                       \
                                                                                                                       \
    X(TOK_COMMA)         /* ,   */                                                                                     \
    X(TOK_DOT)           /* .   */                                                                                     \
    X(TOK_ELLIPSIS)      /* ... */                                                                                     \
    X(TOK_SEMICOLON)     /* ;   */                                                                                     \
    X(TOK_COLON)         /* :   */                                                                                     \
    X(TOK_COLON_COLON)   /* ::  */                                                                                     \
    X(TOK_QUESTION_MARK) /* ?   */                                                                                     \
    X(TOK_MINUS_GREATER) /* ->  */                                                                                     \
    X(TOK_AT)            /* @   */                                                                                     \
                                                                                                                       \
    X(TOK_IDENTIFIER)                                                                                                  \
    X(TOK_STRING_LITERAL)                                                                                              \
    X(TOK_CHAR_LITERAL)                                                                                                \
    X(TOK_INTEGER_LITERAL)                                                                                             \
    X(TOK_FLOAT_LITERAL)                                                                                               \
                                                                                                                       \
    X(TOK_AUTO)     /* auto     */                                                                                     \
    X(TOK_BREAK)    /* break    */                                                                                     \
    X(TOK_CASE)     /* case     */                                                                                     \
    X(TOK_CONST)    /* const    */                                                                                     \
    X(TOK_CONTINUE) /* continue */                                                                                     \
    X(TOK_DEFAULT)  /* default  */                                                                                     \
    X(TOK_DO)       /* do       */                                                                                     \
    X(TOK_ELSE)     /* else     */                                                                                     \
    X(TOK_ENUM)     /* enum     */                                                                                     \
    X(TOK_EXTERN)   /* extern   */                                                                                     \
    X(TOK_FOR)      /* for      */                                                                                     \
    X(TOK_IF)       /* if       */                                                                                     \
    X(TOK_RETURN)   /* return   */                                                                                     \
    X(TOK_STATIC)   /* static   */                                                                                     \
    X(TOK_STRUCT)   /* struct   */                                                                                     \
    X(TOK_SWITCH)   /* switch   */                                                                                     \
    X(TOK_TYPEDEF)  /* typedef  */                                                                                     \
    X(TOK_UNION)    /* union    */                                                                                     \
    X(TOK_WHILE)    /* while    */                                                                                     \
                                                                                                                       \
    X(TOK_VOID) /* void */                                                                                             \
    X(TOK_U8)   /* u8   */                                                                                             \
    X(TOK_U16)  /* u16  */                                                                                             \
    X(TOK_U32)  /* u32  */                                                                                             \
    X(TOK_U64)  /* u64  */                                                                                             \
    X(TOK_I8)   /* i8   */                                                                                             \
    X(TOK_I16)  /* i16  */                                                                                             \
    X(TOK_I32)  /* i32  */                                                                                             \
    X(TOK_I64)  /* i64  */                                                                                             \
    X(TOK_F32)  /* f32  */                                                                                             \
    X(TOK_F64)  /* f64  */                                                                                             \
    X(TOK_BOOL) /* bool */

#define X(type) type,
typedef enum { TOKEN_LIST } TokenType;
#undef X

typedef struct {
    TokenType type;
    union {
        String identifier;
        String stringLiteral;
        u8 charLiteral;
        usize integerLiteral;
        f64 floatLiteral;
        u8 unknown;
        String error;
    } as;
    usize line;
    usize col;
} Token;

typedef struct {
    LIST_FIELDS(Token);
} TokensList;

TokenType findKeywordOrIdent(String keyword);
Token makeSimple(TokenType type, usize line, usize col);
Token makeUnknown(u8 c, usize line, usize col);
Token makeIdentifierToken(String ident, usize line, usize col);
Token makeStringLiteralToken(String strLit, usize line, usize col);
Token makeIntegerLiteralToken(u64 value, usize line, usize col);
Token makeFloatLiteralToken(f64 value, usize line, usize col);
Token makeCharLiteralToken(u8 value, usize line, usize col);
Token makeErrorToken(String errorMsg, usize line, usize col);

#endif  // INCLUDE_KC_TOKEN_H_
