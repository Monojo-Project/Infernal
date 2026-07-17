/*
 * Infernal: el lenguaje de programación. Copyright (C) 2026, GPL v3+ License, Lynds Corp., Aros Legendarios, David Baña Szymaniak.
 * Código fuente de Infernal: runtime/command.h
*/
#ifndef RUNTIME_COMMAND_H
#define RUNTIME_COMMAND_H

#include <stdio.h>

char *expand_command(const char *cmd);
char *get_var_string(const char *name);

int execute_embedded(const char *full_cmd);
FILE *popen_embedded_with_path(const char *full_cmd, const char *mode, char **temp_path);

void set_embedded_tmp_dir(const char *dir);
void cleanup_embedded_temp_dir(void);

#endif
