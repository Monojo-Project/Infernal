#ifndef VM_COMPILER_H
#define VM_COMPILER_H

#include "bytecode.h"
#include "core/ast.h"

Chunk *compile_program(NodeList *program);
Chunk *compile_function(ASTNode *func_node);  // futura expansión

#endif
