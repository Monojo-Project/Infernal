/*
 * Infernal: el lenguaje de programación. Copyright (C) 2026, GPL v3+ License, Lynds Corp., Aros Legendarios, David Baña Szymaniak.
 * Código fuente de Infernal: core/value.c
*/

#include <stdlib.h>
#include <string.h>
#include "value.h"

Value val_make_null(void) {
    Value v; v.type = VAL_NULL; return v;
}

Value val_int(int x) {
    Value v; v.type = VAL_INT; v.data.ival = x; return v;
}

Value val_float(double x) {
    Value v; v.type = VAL_FLOAT; v.data.fval = x; return v;
}

Value val_bool(bool x) {
    Value v; v.type = VAL_BOOL; v.data.bval = x; return v;
}

Value val_string(const char *s) {
    Value v; v.type = VAL_STRING;
    v.data.sval = strdup(s ? s : "");
    return v;
}

Value val_list_empty(void) {
    Value v; v.type = VAL_LIST;
    v.data.list.items = NULL;
    v.data.list.count = v.data.list.cap = 0;
    return v;
}

void val_list_append(Value *list, Value item) {
    if (list->data.list.count >= list->data.list.cap) {
        list->data.list.cap = list->data.list.cap == 0 ? 4 : list->data.list.cap * 2;
        list->data.list.items = realloc(list->data.list.items,
                                        list->data.list.cap * sizeof(Value));
    }
    list->data.list.items[list->data.list.count++] = item;
}

Value val_list_copy(Value *src) {
    Value v = val_list_empty();
    for (int i = 0; i < src->data.list.count; i++)
        val_list_append(&v, src->data.list.items[i]);
    return v;
}

int valtype_to_tokentype(int vtype) {
    switch (vtype) {
        case VAL_INT:    return TOK_INT;
        case VAL_FLOAT:  return TOK_FLOAT;
        case VAL_BOOL:   return TOK_BOOL;
        case VAL_STRING: return TOK_STRING;
        case VAL_LIST:   return TOK_LIST;
        default:         return 0;
    }
}

Value val_reference(const char *list_name, int index) {
    Value v;
    v.type = VAL_REFERENCE;
    v.data.ref.list_name = strdup(list_name);
    v.data.ref.index = index;
    return v;
}
