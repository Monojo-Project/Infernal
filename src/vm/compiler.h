/*
 * Infernal: el lenguaje de programación. Copyright (C) 2026, GPL v3+ License, Lynds Corp., Aros Legendarios, David Baña Szymaniak.
 * Código fuente de Infernal: vm/compiler.h
*/

#ifndef VM_COMPILER_H
#define VM_COMPILER_H

#include "bytecode.h"
#include "core/ast.h"

Chunk *compile_program(NodeList *program);
Chunk *compile_function(ASTNode *func_node);

#endif
