/*
 * Infernal: el lenguaje de programación. Copyright (C) 2026, GPL v3+ License, Lynds Corp., Aros Legendarios, David Baña Szymaniak.
 * Código fuente de Infernal: runtime/error.c
 */
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "error.h"
#include "runtime/globals.h"

void error(int line, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    // Primero formateamos el mensaje base
    char base[512];
    vsnprintf(base, sizeof(base), fmt, ap);
    va_end(ap);

    // Ahora construimos el mensaje final con archivo (si existe) y línea
    if (current_source_file) {
        snprintf(exception_msg, sizeof(exception_msg),
                 "Error en '%s', línea %d: %s", current_source_file, line, base);
    } else {
        snprintf(exception_msg, sizeof(exception_msg),
                 "Error, línea %d: %s", line, base);
    }

    exception_raised = 1;
    longjmp(exception_env, 1);
}
