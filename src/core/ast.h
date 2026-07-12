/*
 * Infernal: el lenguaje de programación. Copyright (C) 2026, GPL v3+ License, Lynds Corp., Aros Legendarios, David Baña Szymaniak.
 * Código fuente de Infernal: core/ast.h
*/

#ifndef CORE_AST_H
#define CORE_AST_H

#include "types.h"

ASTNode *node_create(int kind, int line);
void     nodelist_add(NodeList *list, ASTNode *node);

#endif
