#include "Token.h"
#include <libk/Errors.h>
#include <stdio.h>

#define X(type) [type] = #type,
cstr tokenTypesStrings[] = {TOKEN_LIST};
#undef X

TokenType findKeywordOrIdent(String keyword) {
    ILLEGAL(keyword.len == 0, "Empty String");

    switch (keyword.data[0]) {
        case 'a':
            if (compareCString(&keyword, "auto")) return TOK_AUTO;
            break;
        case 'b':
            if (compareCString(&keyword, "bool")) return TOK_BOOL;
            if (compareCString(&keyword, "break")) return TOK_BREAK;
            break;
        case 'c':
            if (compareCString(&keyword, "case")) return TOK_CASE;
            if (compareCString(&keyword, "const")) return TOK_CONST;
            if (compareCString(&keyword, "continue")) return TOK_CONTINUE;
            break;
        case 'd':
            if (compareCString(&keyword, "default")) return TOK_DEFAULT;
            if (compareCString(&keyword, "do")) return TOK_DO;
            break;
        case 'e':
            if (compareCString(&keyword, "else")) return TOK_ELSE;
            if (compareCString(&keyword, "enum")) return TOK_ENUM;
            if (compareCString(&keyword, "extern")) return TOK_EXTERN;
            break;
        case 'f':
            if (compareCString(&keyword, "f32")) return TOK_F32;
            if (compareCString(&keyword, "f64")) return TOK_F64;
            if (compareCString(&keyword, "for")) return TOK_FOR;
            break;
        case 'i':
            if (compareCString(&keyword, "i16")) return TOK_I16;
            if (compareCString(&keyword, "i32")) return TOK_I32;
            if (compareCString(&keyword, "i64")) return TOK_I64;
            if (compareCString(&keyword, "i8")) return TOK_I8;
            if (compareCString(&keyword, "if")) return TOK_IF;
            break;
        case 'r':
            if (compareCString(&keyword, "return")) return TOK_RETURN;
            break;
        case 's':
            if (compareCString(&keyword, "static")) return TOK_STATIC;
            if (compareCString(&keyword, "struct")) return TOK_STRUCT;
            if (compareCString(&keyword, "switch")) return TOK_SWITCH;
            break;
        case 't':
            if (compareCString(&keyword, "typedef")) return TOK_TYPEDEF;
            break;
        case 'u':
            if (compareCString(&keyword, "u16")) return TOK_U16;
            if (compareCString(&keyword, "u32")) return TOK_U32;
            if (compareCString(&keyword, "u64")) return TOK_U64;
            if (compareCString(&keyword, "u8")) return TOK_U8;
            if (compareCString(&keyword, "union")) return TOK_UNION;
            break;
        case 'v':
            if (compareCString(&keyword, "void")) return TOK_VOID;
            break;
        case 'w':
            if (compareCString(&keyword, "while")) return TOK_WHILE;
    }

    return TOK_IDENTIFIER;
}

Token makeSimple(TokenType type, usize line, usize col) {
    return (Token){
        .type = type,
        .as.unknown = 0,
        .line = line,
        .col = col,
    };
}

Token makeUnknown(u8 c, usize line, usize col) {
    return (Token){
        .type = TOK_UNKNOWN,
        .as.unknown = c,
        .line = line,
        .col = col,
    };
}

Token makeIdentifierToken(String ident, usize line, usize col) {
    return (Token){
        .type = TOK_IDENTIFIER,
        .as.identifier = ident,
        .line = line,
        .col = col,
    };
}

Token makeStringLiteralToken(String strLit, usize line, usize col) {
    return (Token){
        .type = TOK_STRING_LITERAL,
        .as.stringLiteral = strLit,
        .line = line,
        .col = col,
    };
}

Token makeIntegerLiteralToken(u64 value, usize line, usize col) {
    return (Token){
        .type = TOK_INTEGER_LITERAL,
        .as.integerLiteral = value,
        .line = line,
        .col = col,
    };
}

Token makeFloatLiteralToken(f64 value, usize line, usize col) {
    return (Token){
        .type = TOK_FLOAT_LITERAL,
        .as.floatLiteral = value,
        .line = line,
        .col = col,
    };
}

Token makeCharLiteralToken(u8 value, usize line, usize col) {
    return (Token){
        .type = TOK_CHAR_LITERAL,
        .as.charLiteral = value,
        .line = line,
        .col = col,
    };
}

Token makeErrorToken(String errorMsg, usize line, usize col) {
    return (Token){
        .type = TOK_ERROR,
        .as.error = errorMsg,
        .line = line,
        .col = col,
    };
}

static Bool isPrintableChar(u8 c) { return ' ' <= c && c <= '~'; }

void printToken(Token token) {
    printf("{ \"type\": \"%s\", \"line\": %zu, \"col\": %zu", tokenTypesStrings[token.type], token.line, token.col);
    switch (token.type) {
        case TOK_IDENTIFIER:
            printf(", \"name\": \"%.*s\"", (int)token.as.identifier.len, token.as.identifier.data);
            break;
        case TOK_STRING_LITERAL:
            printf(", \"value\": \"%.*s\"", (int)token.as.stringLiteral.len, token.as.stringLiteral.data);
            break;
        case TOK_CHAR_LITERAL:
            if (isPrintableChar(token.as.charLiteral))
                printf(", \"value\": \"%c\"", token.as.charLiteral);
            else
                printf(", \"value\": \"0x%02x\"", token.as.charLiteral);
            break;
        case TOK_INTEGER_LITERAL:
            printf(", \"value\": %zu", token.as.integerLiteral);
            break;
        case TOK_FLOAT_LITERAL:
            printf(", \"value\": %f", token.as.floatLiteral);
            break;
        case TOK_UNKNOWN:
            if (isPrintableChar(token.as.unknown))
                printf(", \"value\": \"%c\"", token.as.unknown);
            else
                printf(", \"value\": \"0x%02x\"", token.as.unknown);
            break;
        case TOK_ERROR:
            printf(", \"error\": \"%.*s\"", (int)token.as.error.len, token.as.error.data);
            break;
        default:
            break;
    }
    printf(" }");
}
