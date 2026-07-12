/*
 * Infernal: el lenguaje de programación. Copyright (C) 2026, GPL v3+ License, Lynds Corp., Aros Legendarios, David Baña Szymaniak.
 * Código fuente de Infernal: runtime/command.c
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <dirent.h>

#include "command.h"
#include "runtime/scope.h"
#include "runtime/globals.h"
#include "stdlib/embedded.h"

static char *embedded_tmp_dir = NULL;

void set_embedded_tmp_dir(const char *dir) {
    if (embedded_tmp_dir) free(embedded_tmp_dir);
    embedded_tmp_dir = dir ? strdup(dir) : NULL;
}

char *get_var_string(const char *name) {
    VarEntry *e = scope_find(current_scope, name);
    if (!e) return NULL;
    Value *v = &e->value;
    char buf[256];
    switch (v->type) {
        case VAL_INT: snprintf(buf, sizeof(buf), "%d", v->data.ival); break;
        case VAL_FLOAT: snprintf(buf, sizeof(buf), "%g", v->data.fval); break;
        case VAL_BOOL: return strdup(v->data.bval ? "true" : "false");
        case VAL_STRING: return strdup(v->data.sval);
        default: return NULL;
    }
    return strdup(buf);
}

char *expand_command(const char *cmd) {
    if (!cmd) return NULL;
    char *result = strdup(cmd);
    char *p = result;
    char *new_result = NULL;
    size_t new_cap = 0;

    while (*p) {
        if (*p == '$' && (isalpha(*(p+1)) || *(p+1) == '_')) {
            char *start = p + 1;
            char *end = start;
            while (isalnum(*end) || *end == '_') end++;
            char name[128];
            int len = end - start;
            strncpy(name, start, len);
            name[len] = '\0';
            char *val = get_var_string(name);
            if (val) {
                size_t prefix_len = p - result;
                size_t suffix_len = strlen(end);
                size_t val_len = strlen(val);
                size_t total = prefix_len + val_len + suffix_len;
                if (new_cap < total + 1) {
                    new_cap = total + 256;
                    new_result = realloc(new_result, new_cap);
                }
                memcpy(new_result, result, prefix_len);
                memcpy(new_result + prefix_len, val, val_len);
                memcpy(new_result + prefix_len + val_len, end, suffix_len + 1);
                free(result);
                result = new_result;
                p = result + prefix_len + val_len;
                free(val);
                continue;
            }
            p = end;
        } else if (*p == '{') {
            char *start = p + 1;
            if (isalpha(*start) || *start == '_') {
                char *end = strchr(start, '}');
                if (end) {
                    char name[128];
                    int len = end - start;
                    strncpy(name, start, len);
                    name[len] = '\0';
                    char *val = get_var_string(name);
                    if (val) {
                        size_t prefix_len = p - result;
                        size_t suffix_len = strlen(end + 1);
                        size_t val_len = strlen(val);
                        size_t total = prefix_len + val_len + suffix_len;
                        if (new_cap < total + 1) {
                            new_cap = total + 256;
                            new_result = realloc(new_result, new_cap);
                        }
                        memcpy(new_result, result, prefix_len);
                        memcpy(new_result + prefix_len, val, val_len);
                        memcpy(new_result + prefix_len + val_len, end + 1, suffix_len + 1);
                        free(result);
                        result = new_result;
                        p = result + prefix_len + val_len;
                        free(val);
                        continue;
                    }
                }
                p = end + 1;
                continue;
            }
        }
        p++;
    }
    return result;
}

/* ─── Funciones auxiliares para manejar comandos embebidos ─── */

static char *prepare_embedded_binary(const char *cmd_name) {
    const unsigned char *data = NULL;
    size_t size = 0;
    if (!embedded_find(cmd_name, &data, &size))
        return NULL;

    char tmp_path[PATH_MAX];
    int fd = -1;

    const char *tmpdir = getenv("TMPDIR");
    if (tmpdir && tmpdir[0] != '\0') {
        snprintf(tmp_path, sizeof(tmp_path), "%s/infernal_XXXXXX", tmpdir);
        fd = mkstemp(tmp_path);
    }
    if (fd == -1) {
        snprintf(tmp_path, sizeof(tmp_path), "/tmp/infernal_XXXXXX");
        fd = mkstemp(tmp_path);
    }
    if (fd == -1) {
        mkdir("./.infernal_tmp", 0700);
        snprintf(tmp_path, sizeof(tmp_path), "./.infernal_tmp/infernal_XXXXXX");
        fd = mkstemp(tmp_path);
    }
    if (fd == -1) {
        perror("mkstemp");
        return NULL;
    }

    if (write(fd, data, size) != (ssize_t)size) {
        perror("write");
        close(fd);
        unlink(tmp_path);
        return NULL;
    }
    fchmod(fd, 0700);
    close(fd);
    return strdup(tmp_path);
}

int execute_embedded(const char *full_cmd) {
    if (!full_cmd) return -1;

    char *cmd_copy = strdup(full_cmd);
    char *saveptr;
    char *cmd_name = strtok_r(cmd_copy, " \t", &saveptr);
    if (!cmd_name) {
        free(cmd_copy);
        return -1;
    }

    char *binary_path = prepare_embedded_binary(cmd_name);
    if (!binary_path) {
        free(cmd_copy);
        return -1;
    }

    size_t len = strlen(binary_path) + 1;
    char *rest = saveptr;
    if (rest && *rest) len += strlen(rest) + 1;
    char *exec_cmd = malloc(len);
    if (!exec_cmd) {
        free(binary_path);
        free(cmd_copy);
        return -1;
    }
    strcpy(exec_cmd, binary_path);
    if (rest && *rest) {
        strcat(exec_cmd, " ");
        strcat(exec_cmd, rest);
    }

    int ret = system(exec_cmd);

    unlink(binary_path);
    free(binary_path);
    free(exec_cmd);
    free(cmd_copy);
    return ret;
}

FILE *popen_embedded_with_path(const char *full_cmd, const char *mode, char **temp_path) {
    if (!full_cmd || !temp_path) return NULL;

    char *cmd_copy = strdup(full_cmd);
    char *saveptr;
    char *cmd_name = strtok_r(cmd_copy, " \t", &saveptr);
    if (!cmd_name) {
        free(cmd_copy);
        return NULL;
    }

    char *binary_path = prepare_embedded_binary(cmd_name);
    if (!binary_path) {
        free(cmd_copy);
        return NULL;
    }

    size_t len = strlen(binary_path) + 1;
    char *rest = saveptr;
    if (rest && *rest) len += strlen(rest) + 1;
    char *exec_cmd = malloc(len);
    if (!exec_cmd) {
        free(binary_path);
        free(cmd_copy);
        return NULL;
    }
    strcpy(exec_cmd, binary_path);
    if (rest && *rest) {
        strcat(exec_cmd, " ");
        strcat(exec_cmd, rest);
    }

    FILE *fp = popen(exec_cmd, mode);
    if (fp) {
        *temp_path = binary_path;
    } else {
        unlink(binary_path);
        free(binary_path);
    }
    free(exec_cmd);
    free(cmd_copy);
    return fp;
}

void cleanup_embedded_temp_dir(void) {
    DIR *d = opendir("./.infernal_tmp");
    if (!d) return;

    struct dirent *entry;
    while ((entry = readdir(d)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        char full_path[PATH_MAX];
        snprintf(full_path, sizeof(full_path), "./.infernal_tmp/%s", entry->d_name);
        unlink(full_path);
    }
    closedir(d);
    rmdir("./.infernal_tmp");
}
