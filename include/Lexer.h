#ifndef INCLUDE_INCLUDE_LEXER_H_
#define INCLUDE_INCLUDE_LEXER_H_

#include "Token.h"
#include <libk/String.h>
#include <libk/Errors.h>

Bool scanFile(TokensList *dest, cstr path);
void freeTokensList(TokensList *tokens);
void printToken(Token token);

#endif // INCLUDE_INCLUDE_LEXER_H_
