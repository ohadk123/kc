#ifndef LEXER_H
#define LEXER_H

#include "token.h"

bool scan_file(TokensList *dest, const char *path);

#endif // LEXER_H
