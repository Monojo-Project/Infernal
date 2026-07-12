/*
 * Infernal: el lenguaje de programación. Copyright (C) 2026, GPL v3+ License, Lynds Corp., Aros Legendarios, David Baña Szymaniak.
 * Código fuente de Infernal: runtime/scope.c
*/
#include <stdlib.h>
#include <string.h>
#include "scope.h"
#include "core/value.h"
#include "runtime/error.h"

Scope *scope_new(Scope *parent) {
    Scope *s = malloc(sizeof(Scope));
    s->vars = NULL;
    s->portals = NULL;
    s->parent = parent;
    return s;
}

VarEntry *scope_find(Scope *scope, const char *name) {
    while (scope) {
        for (VarEntry *e = scope->vars; e; e = e->next)
            if (strcmp(e->name, name) == 0) return e;
        scope = scope->parent;
    }
    return NULL;
}

VarEntry *scope_find_script(Scope *scope, const char *name) {
    extern Scope *super_global_scope;
    while (scope && scope != super_global_scope) {
        for (VarEntry *e = scope->vars; e; e = e->next)
            if (strcmp(e->name, name) == 0) return e;
        scope = scope->parent;
    }
    return NULL;
}

void scope_define(Scope *scope, const char *name, int vtype, Value val) {
    VarEntry *e = malloc(sizeof(VarEntry));
    e->name = strdup(name);
    e->vtype = vtype ? vtype : valtype_to_tokentype(val.type);
    e->value = val;
    e->next = scope->vars;
    scope->vars = e;
}

void scope_assign(Scope *scope, const char *name, Value val, int line) {
    VarEntry *e = scope_find(scope, name);
    if (e) {
        int expected = e->vtype;
        int new_type = valtype_to_tokentype(val.type);
        if (expected != 0 && new_type != expected) {
            error(line, "Tipado fijo: la variable '%s' es de tipo %s, no se puede asignar un valor de tipo %s",
                  name,
                  expected == TOK_INT ? "int" : expected == TOK_FLOAT ? "float" :
                  expected == TOK_BOOL ? "bool" : expected == TOK_STRING ? "string" :
                  expected == TOK_LIST ? "list" : "?",
                  new_type == TOK_INT ? "int" : new_type == TOK_FLOAT ? "float" :
                  new_type == TOK_BOOL ? "bool" : new_type == TOK_STRING ? "string" :
                  new_type == TOK_LIST ? "list" : "?");
        }
        e->value = val;
    } else {
        scope_define(scope, name, 0, val);
    }
}

PortalEntry *portal_find(Scope *scope, const char *name) {
    while (scope) {
        for (PortalEntry *p = scope->portals; p; p = p->next)
            if (strcmp(p->name, name) == 0) return p;
        scope = scope->parent;
    }
    return NULL;
}

PortalEntry *portal_find_in_scope(Scope *scope, const char *name) {
    for (PortalEntry *p = scope->portals; p; p = p->next)
        if (strcmp(p->name, name) == 0) return p;
    return NULL;
}

void portal_define(Scope *scope, const char *name, int line) {
    PortalEntry *p = malloc(sizeof(PortalEntry));
    p->name = strdup(name);
    p->line = line;
    p->next = scope->portals;
    scope->portals = p;
}
