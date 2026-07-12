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
    int len = vsnprintf(exception_msg, sizeof(exception_msg), fmt, ap);
    va_end(ap);
    if (line > 0) {
        snprintf(exception_msg + len, sizeof(exception_msg) - len, " (línea %d)", line);
    }
    // Eliminada la impresión de la línea fuente para evitar mensajes parásitos
    exception_raised = 1;
    longjmp(exception_env, 1);
}
