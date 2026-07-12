/*
 * Infernal: el lenguaje de programación. Copyright (C) 2026, GPL v3+ License, Lynds Corp., Aros Legendarios, David Baña Szymaniak.
 * Código fuente de Infernal: stdlib/embedded.c
*/

#include <string.h>
#include "embedded.h"

extern EmbeddedModule embedded_modules[];   // definido en embedded_table.c

int embedded_find(const char *name, const unsigned char **data, size_t *size)
{
    for (int i = 0; embedded_modules[i].name != NULL; i++) {
        if (strcmp(embedded_modules[i].name, name) == 0) {
            *data = embedded_modules[i].data;
            *size = (size_t)(*embedded_modules[i].size);
            return 1;
        }
    }
    return 0;
}
