#ifndef VM_VM_H
#define VM_VM_H

#include "bytecode.h"

// Ejecuta un chunk (programa principal o función) y devuelve el valor de retorno.
Value vm_run(Chunk *chunk);

// Tablas globales indexadas para acceso rápido
#define MAX_GLOBALS 256
extern Value vm_globals[MAX_GLOBALS];
extern int vm_global_count;

// Tabla de builtins indexada
typedef Value (*VmBuiltin)(int argc, Value *args);
extern VmBuiltin vm_builtins[256];
extern int vm_builtin_count;

// Registro de global y builtin
int vm_register_global(const char *name, Value val);
int vm_register_builtin(const char *name, VmBuiltin func);
int vm_find_global_index(const char *name);   // devuelve -1 si no existe
int vm_find_builtin_index(const char *name);

#endif
