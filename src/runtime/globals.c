/*
 * Infernal: el lenguaje de programación. Copyright (C) 2026, GPL v3+ License, Lynds Corp., Aros Legendarios, David Baña Szymaniak.
 * Código fuente de Infernal: runtime/globals.c
 */
#include <stdlib.h>
#include <string.h>
#include "globals.h"

Scope *global_scope = NULL;
Scope *super_global_scope = NULL;
Scope *current_scope = NULL;

FuncEntry *func_table = NULL;
char *current_import_prefix = NULL;
int max_loop_iterations = 10000; // MODIFICA ESTO si quieres cambiar el límite de iteraciones en bucles

jmp_buf exception_env;
int exception_raised = 0;
char exception_msg[512];
char **source_lines = NULL;
int source_line_count = 0;
int control_flow = CF_NONE;
Value return_value;

int script_argc;
char **script_argv;

int repeat_line_target = 0;

char *script_dir = NULL;

char *current_source_file = NULL;   // <-- nuevo

void func_register(const char *name, ASTNode *def) {
    FuncObject *obj = malloc(sizeof(FuncObject));
    obj->kind = FUNC_USER;
    obj->def = def;
    FuncEntry *e = malloc(sizeof(FuncEntry));
    e->name = strdup(name);
    e->obj = obj;
    e->next = func_table;
    func_table = e;
}

void func_register_builtin(const char *name, BuiltinFunc fn) {
    FuncObject *obj = malloc(sizeof(FuncObject));
    obj->kind = FUNC_BUILTIN;
    obj->builtin = fn;
    FuncEntry *e = malloc(sizeof(FuncEntry));
    e->name = strdup(name);
    e->obj = obj;
    e->next = func_table;
    func_table = e;
}

FuncObject *func_lookup(const char *name) {
    for (FuncEntry *e = func_table; e; e = e->next)
        if (strcmp(e->name, name) == 0)
            return e->obj;
    return NULL;
}
