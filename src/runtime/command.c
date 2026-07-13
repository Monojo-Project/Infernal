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
#include <sys/types.h>
#include <pwd.h>

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
    size_t cap = strlen(cmd) * 2 + 64;
    char *result = malloc(cap);
    size_t len = 0;
    const char *p = cmd;

    while (*p) {
        if (*p == '$' && (isalpha(*(p+1)) || *(p+1) == '_')) {
            const char *start = p + 1;
            while (isalnum(*start) || *start == '_') start++;
            size_t nlen = start - (p + 1);
            if (nlen > 127) nlen = 127;
            char name[128];
            memcpy(name, p + 1, nlen);
            name[nlen] = '\0';
            char *val = get_var_string(name);
            if (val) {
                size_t vlen = strlen(val);
                if (len + vlen >= cap) {
                    cap = (len + vlen) * 2;
                    result = realloc(result, cap);
                }
                memcpy(result + len, val, vlen);
                len += vlen;
                free(val);
                p = start;
                continue;
            }
            p = start;
            continue;
        }
        if (len + 1 >= cap) {
            cap *= 2;
            result = realloc(result, cap);
        }
        result[len++] = *p++;
    }
    result[len] = '\0';
    return realloc(result, len + 1);
}

/* ─── Extracción del binario embebido (devuelve ruta absoluta, SIN borrar) ─── */
static char *prepare_embedded_binary(const char *cmd_name) {
    const unsigned char *data = NULL;
    size_t size = 0;
    if (!embedded_find(cmd_name, &data, &size))
        return NULL;

    char tmp_path[PATH_MAX];
    int fd = -1;

    /* 1. Directorio actual */
    if (access(".", W_OK) == 0) {
        mkdir("./.infernal_tmp", 0700);
        snprintf(tmp_path, sizeof(tmp_path), "./.infernal_tmp/infernal_XXXXXX");
        fd = mkstemp(tmp_path);
    }

    /* 2. /tmp */
    if (fd == -1) {
        const char *tmpdir = getenv("TMPDIR");
        if (tmpdir && tmpdir[0]) {
            snprintf(tmp_path, sizeof(tmp_path), "%s/infernal_XXXXXX", tmpdir);
            fd = mkstemp(tmp_path);
        }
    }
    if (fd == -1) {
        snprintf(tmp_path, sizeof(tmp_path), "/tmp/infernal_XXXXXX");
        fd = mkstemp(tmp_path);
    }

    if (fd == -1) {
        perror("mkstemp");
        return NULL;
    }

    ssize_t written = write(fd, data, size);
    if (written != (ssize_t)size) {
        perror("write");
        close(fd);
        unlink(tmp_path);
        return NULL;
    }

    fdatasync(fd);
    fchmod(fd, 0700);
    close(fd);

    // Obtener ruta absoluta (el archivo sigue existiendo)
    char *abs_path = realpath(tmp_path, NULL);
    if (!abs_path) {
        perror("realpath");
        unlink(tmp_path);
        return NULL;
    }
    return abs_path;
}

/* ─── Ejecutar comando embebido (SIN fallback a /bin/sh) ─── */
int execute_embedded(const char *full_cmd) {
    if (!full_cmd) return -1;

    char *cmd_copy = strdup(full_cmd);
    char *saveptr;
    char *cmd_name = strtok_r(cmd_copy, " \t", &saveptr);
    if (!cmd_name) { free(cmd_copy); return -1; }

    char *binary_path = prepare_embedded_binary(cmd_name);
    if (!binary_path) { free(cmd_copy); return -1; }

    size_t len = strlen(binary_path) + 1;
    char *rest = saveptr;
    if (rest && *rest) len += strlen(rest) + 1;
    char *exec_cmd = malloc(len);
    if (!exec_cmd) { unlink(binary_path); free(binary_path); free(cmd_copy); return -1; }
    strcpy(exec_cmd, binary_path);
    if (rest && *rest) { strcat(exec_cmd, " "); strcat(exec_cmd, rest); }

    int ret = system(exec_cmd);

    // *** CORRECCIÓN: se elimina el fallback con /bin/sh ***
    // (anteriormente intentaba ejecutar de nuevo con /bin/sh, lo que causaba
    //  errores de sintaxis en scripts de Bash al usar dash)

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
    if (!cmd_name) { free(cmd_copy); return NULL; }
    char *binary_path = prepare_embedded_binary(cmd_name);
    if (!binary_path) { free(cmd_copy); return NULL; }
    size_t len = strlen(binary_path) + 1;
    char *rest = saveptr;
    if (rest && *rest) len += strlen(rest) + 1;
    char *exec_cmd = malloc(len);
    if (!exec_cmd) { unlink(binary_path); free(binary_path); free(cmd_copy); return NULL; }
    strcpy(exec_cmd, binary_path);
    if (rest && *rest) { strcat(exec_cmd, " "); strcat(exec_cmd, rest); }
    FILE *fp = popen(exec_cmd, mode);
    if (fp) { *temp_path = binary_path; }
    else { unlink(binary_path); free(binary_path); }
    free(exec_cmd);
    free(cmd_copy);
    return fp;
}

void cleanup_embedded_temp_dir(void) {
    DIR *d = opendir("./.infernal_tmp");
    if (!d) return;
    struct dirent *entry;
    while ((entry = readdir(d)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
        char full_path[PATH_MAX];
        snprintf(full_path, sizeof(full_path), "./.infernal_tmp/%s", entry->d_name);
        unlink(full_path);
    }
    closedir(d);
    rmdir("./.infernal_tmp");
}
