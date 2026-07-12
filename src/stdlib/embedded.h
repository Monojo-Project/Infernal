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
    const unsigned int *size;   /* xxd genera unsigned int */
} EmbeddedModule;

extern EmbeddedModule embedded_modules[];

/* Busca un módulo por nombre. Devuelve 1 si lo encuentra, 0 en otro caso.
   Si lo encuentra, asigna *data y *size. */
int embedded_find(const char *name, const unsigned char **data, size_t *size);

#endif
