/*
 * Infernal: el lenguaje de programación. Copyright (C) 2026, GPL v3+ License, Lynds Corp., Aros Legendarios, David Baña Szymaniak.
 * Código fuente de Infernal: core/ast.c
*/

#include <stdlib.h>
#include "ast.h"

ASTNode *node_create(int kind, int line) {
    ASTNode *n = calloc(1, sizeof(ASTNode));
    n->kind = kind;
    n->line = line;
    return n;
}

void nodelist_add(NodeList *list, ASTNode *node) {
    if (list->count >= list->cap) {
        list->cap = list->cap == 0 ? 8 : list->cap * 2;
        list->stmts = realloc(list->stmts, list->cap * sizeof(ASTNode*));
    }
    list->stmts[list->count++] = node;
}
