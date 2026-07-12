/*
 * Infernal: el lenguaje de programación. Copyright (C) 2026, GPL v3+ License, Lynds Corp., Aros Legendarios, David Baña Szymaniak.
 * Código fuente de Infernal: stdlib/system.c
*/

#include <stdlib.h>
#include "system.h"
#include "core/value.h"
#include "runtime/globals.h"
#include "runtime/error.h"

static Value builtin_exit(int argc, Value *args) {
    int code = 0;
    if (argc > 0) {
        Value v = args[0];
        code = (v.type == VAL_INT) ? v.data.ival : 0;
    }
    exit(code);
}

static Value builtin_setlooplimit(int argc, Value *args) {
    if (argc < 1) error(0, "setlooplimit requiere un argumento");
    Value v = args[0];
    if (v.type == VAL_INT) max_loop_iterations = v.data.ival;
    else error(0, "setlooplimit espera un entero");
    return val_make_null();
}

static Value builtin_getlooplimit(int argc, Value *args) {
    (void)argc; (void)args;
    return val_int(max_loop_iterations);
}

void register_system_builtins(void) {
    func_register_builtin("exit", builtin_exit);
    func_register_builtin("setlooplimit", builtin_setlooplimit);
    func_register_builtin("getlooplimit", builtin_getlooplimit);
}
