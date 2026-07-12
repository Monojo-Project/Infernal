/*
 * Infernal: el lenguaje de programación. Copyright (C) 2026, GPL v3+ License, Lynds Corp., Aros Legendarios, David Baña Szymaniak.
 * Código fuente de Infernal: runtime/evaluator.h
*/
#ifndef RUNTIME_EVALUATOR_H
#define RUNTIME_EVALUATOR_H

#include "core/types.h"

Value eval_expr(ASTNode *expr);
void exec_block(NodeList *block);
void exec_block_from(NodeList *block, int start_index);
void exec_flag_spec(FlagSpec *spec);

#endif
