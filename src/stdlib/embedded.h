/*
 * Infernal: el lenguaje de programación. Copyright (C) 2026, GPL v3+ License, Lynds Corp., Aros Legendarios, David Baña Szymaniak.
 * Código fuente de Infernal: stdlib/embedded.h
 */
#ifndef STDLIB_EMBEDDED_H
#define STDLIB_EMBEDDED_H

#include <stddef.h>

typedef struct {
    const char *name;
    const unsigned char *data;
    size_t *size_ptr;
    int compressed;   // 1 = gzip comprimido, 0 = datos crudos
} EmbeddedModule;

extern EmbeddedModule embedded_modules[];

int embedded_find(const char *name, const unsigned char **data, size_t *size, int *compressed);

#endif
