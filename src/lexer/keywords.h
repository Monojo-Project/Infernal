/*
 * Infernal: el lenguaje de programación. Copyright (C) 2026, GPL v3+ License, Lynds Corp., Aros Legendarios, David Baña Szymaniak.
 * Código fuente de Infernal: lexer/keywords.h
*/

#ifndef LEXER_KEYWORDS_H
#define LEXER_KEYWORDS_H

#include "core/types.h"

typedef struct {
    const char *word;
    TokenType tok;
} Keyword;

extern Keyword keywords[];
TokenType lookup_keyword(const char *s);

#endif
