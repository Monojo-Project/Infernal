/*
 * Infernal: el lenguaje de programación. Copyright (C) 2026, GPL v3+ License, Lynds Corp., Aros Legendarios, David Baña Szymaniak.
 * Código fuente de Infernal: core/memory.h
*/

#ifndef CORE_MEMORY_H
#define CORE_MEMORY_H

#include <stddef.h>

void *infernal_malloc(size_t size);
void *infernal_calloc(size_t count, size_t size);
void *infernal_realloc(void *ptr, size_t size);
char *infernal_strdup(const char *value);
#define infernal_free(ptr) free(ptr)

#endif
