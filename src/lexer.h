#ifndef LEXER_H
#define LEXER_H

#include "token.h"

void scan_file(TokensList *dest, const char *path);

#endif // LEXER_H
