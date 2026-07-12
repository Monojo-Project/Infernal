/*
 * Infernal: el lenguaje de programación. Copyright (C) 2026, GPL v3+ License, Lynds Corp., Aros Legendarios, David Baña Szymaniak.
 * Código fuente de Infernal: stdlib/output.c
*/

#include <stdio.h>
#include "output.h"
#include "core/value.h"
#include "runtime/globals.h"

static void print_value(Value v) {
    switch (v.type) {
        case VAL_INT: printf("%d", v.data.ival); break;
        case VAL_FLOAT: printf("%g", v.data.fval); break;
        case VAL_BOOL: printf("%s", v.data.bval ? "true" : "false"); break;
        case VAL_STRING: printf("%s", v.data.sval); break;
        case VAL_LIST:
            printf("[");
            for (int j = 0; j < v.data.list.count; j++) {
                if (j > 0) printf(", ");
                print_value(v.data.list.items[j]);
            }
            printf("]");
            break;
        default: printf("?");
    }
}

static Value builtin_print(int argc, Value *args) {
    for (int i = 0; i < argc; i++) {
        if (i > 0) printf(" ");
        print_value(args[i]);
    }
    printf("\n");
    return val_make_null();
}

static Value builtin_warn(int argc, Value *args) {
    printf("\033[33m");
    builtin_print(argc, args);
    printf("\033[0m");
    return val_make_null();
}

static Value builtin_error(int argc, Value *args) {
    printf("\033[31m");
    builtin_print(argc, args);
    printf("\033[0m");
    return val_make_null();
}

static Value builtin_success(int argc, Value *args) {
    printf("\033[32m");
    builtin_print(argc, args);
    printf("\033[0m");
    return val_make_null();
}

void register_output_builtins(void) {
    func_register_builtin("print", builtin_print);
    func_register_builtin("warn", builtin_warn);
    func_register_builtin("error", builtin_error);
    func_register_builtin("success", builtin_success);
}
