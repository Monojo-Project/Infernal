#ifndef VM_BYTECODE_H
#define VM_BYTECODE_H

#include "core/types.h"
#include "core/value.h"

typedef enum {
    OP_NOP,
    OP_PUSH_INT, OP_PUSH_FLOAT, OP_PUSH_STRING, OP_PUSH_BOOL, OP_PUSH_NULL,
    OP_LOAD_VAR,        // push local by index
    OP_STORE_VAR,       // pop into local
    OP_LOAD_GLOBAL,     // push global by index
    OP_STORE_GLOBAL,    // pop into global
    OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_MOD,
    OP_NEG,
    OP_EQ, OP_NEQ, OP_LT, OP_GT, OP_LE, OP_GE,
    OP_AND, OP_OR, OP_NOT,
    OP_CALL_BUILTIN,    // builtin indexed by operand
    OP_CALL_USER,       // user function chunk
    OP_RETURN,
    OP_JUMP_IF_FALSE, OP_JUMP,
    OP_DUP, OP_POP,
    OP_NEW_LIST, OP_LIST_APPEND,
    OP_INDEX,           // list/string access
    OP_INDEX_ASSIGN,    // list[index] = value (for assign with index)
    // commands
    OP_EMBEDDED_CMD,
    OP_SHELL_CMD,
    OP_FLAGS,           // complex, will call C handler for now
    // repeat / portals (we'll keep as C callbacks)
} OpCode;

typedef struct {
    OpCode op;
    int operand;        // index into constants pool, locals, globals, or jump offset
} Instruction;

typedef struct {
    Value *constants;   // constant pool (literals, strings)
    int const_count, const_cap;

    char **local_names; // for debugging / variable mapping
    int local_count;

    Instruction *code;
    int code_count, code_cap;

    // for user functions: parameter count
    int param_count;
} Chunk;

#endif
