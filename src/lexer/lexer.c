/*
 * Infernal: el lenguaje de programación. Copyright (C) 2026, GPL v3+ License, Lynds Corp., Aros Legendarios, David Baña Szymaniak.
 * Código fuente de Infernal: lexer/lexer.c
*/

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include "lexer.h"
#include "keywords.h"
#include "runtime/globals.h"
#include "runtime/error.h"   // <-- añadido para error()

TokenStream ts;

void ts_init() { ts.tokens = NULL; ts.count = ts.cap = 0; ts.pos = 0; }

void ts_add(Token t) {
    if (ts.count >= ts.cap) {
        ts.cap = ts.cap == 0 ? 256 : ts.cap * 2;
        ts.tokens = realloc(ts.tokens, ts.cap * sizeof(Token));
    }
    ts.tokens[ts.count++] = t;
}

Token ts_peek() {
    if (ts.pos < ts.count) return ts.tokens[ts.pos];
    return (Token){TOK_EOF, "", 0};
}

Token ts_advance() {
    if (ts.pos < ts.count) return ts.tokens[ts.pos++];
    return (Token){TOK_EOF, "", 0};
}

bool ts_match(TokenType t) {
    if (ts_peek().type == t) { ts_advance(); return true; }
    return false;
}

void ts_skip_newlines() {
    while (ts_peek().type == TOK_NEWLINE) ts_advance();
}

/* Extrae la parte derecha de una asignación desde la línea original */
char *extract_command_string(int line) {
    if (line < 1 || line > source_line_count) return strdup("");
    char *src = source_lines[line - 1];
    char *eq = strchr(src, '=');
    if (!eq) return strdup("");
    char *start = eq + 1;
    while (*start == ' ' || *start == '\t') start++;
    char *hash = strchr(start, '#');
    if (hash) *hash = '\0';
    int end = strlen(start) - 1;
    while (end >= 0 && (start[end] == ' ' || start[end] == '\t')) end--;
    start[end + 1] = '\0';
    return strdup(start);
}

/* Tokeniza directamente desde un buffer en memoria (para módulos embebidos) */
void tokenize_buffer(const char *data, size_t len) {
    FILE *fp = fmemopen((void*)data, len, "r");
    if (!fp) return;
    tokenize_file(fp);
    fclose(fp);
}

/* Tokeniza desde un archivo normal */
void tokenize_file(FILE *fp) {
    char *line = NULL;
    size_t len = 0;
    int lineno = 0;
    bool in_block_comment = false;
    if (source_lines) {
        for (int i = 0; i < source_line_count; i++) free(source_lines[i]);
        free(source_lines);
        source_lines = NULL;
        source_line_count = 0;
    }

    while (getline(&line, &len, fp) != -1) {
        lineno++;
        source_lines = realloc(source_lines, (source_line_count + 1) * sizeof(char*));
        source_lines[source_line_count] = strdup(line);
        if (source_lines[source_line_count][strlen(source_lines[source_line_count])-1] == '\n')
            source_lines[source_line_count][strlen(source_lines[source_line_count])-1] = '\0';
        source_line_count++;

        char *p = line;
        // Si estamos dentro de un comentario de bloque, buscar el cierre
        if (in_block_comment) {
            char *close = strstr(p, "###");
            if (close) {
                in_block_comment = false;
                p = close + 3;
            } else {
                continue;   // toda la línea es parte del comentario
            }
        }

        while (*p) {
            // Detectar inicio o fin de comentario de bloque
            if (*p == '#' && *(p+1) == '#' && *(p+2) == '#') {
                if (!in_block_comment) {
                    char *close = strstr(p + 3, "###");
                    if (close) {
                        p = close + 3;
                        continue;
                    } else {
                        in_block_comment = true;
                        break;
                    }
                } else {
                    in_block_comment = false;
                    p += 3;
                    continue;
                }
            }

            // Comentario de línea
            if (*p == '#') break;
            if (isspace(*p)) { p++; continue; }

            // Operadores compuestos
            if (*p == '&' && *(p+1) == '&') { ts_add((Token){TOK_AND, "&&", lineno}); p+=2; continue; }
            if (*p == '|' && *(p+1) == '|') { ts_add((Token){TOK_OR, "||", lineno}); p+=2; continue; }
            if (*p == '=' && *(p+1) == '=') { ts_add((Token){TOK_EEQ, "==", lineno}); p+=2; continue; }
            if (*p == '!' && *(p+1) == '=') { ts_add((Token){TOK_NEQ, "!=", lineno}); p+=2; continue; }
            if (*p == '<' && *(p+1) == '=') { ts_add((Token){TOK_LE, "<=", lineno}); p+=2; continue; }
            if (*p == '>' && *(p+1) == '=') { ts_add((Token){TOK_GE, ">=", lineno}); p+=2; continue; }
            if (*p == '>' && *(p+1) == '>') { ts_add((Token){TOK_GGT, ">>", lineno}); p+=2; continue; }
            if (*p == '|') { ts_add((Token){TOK_PIPE, "|", lineno}); p++; continue; }
            if (*p == '>') { ts_add((Token){TOK_GT_OP, ">", lineno}); p++; continue; }
            if (*p == '<') { ts_add((Token){TOK_LT_OP, "<", lineno}); p++; continue; }
            if (*p == '(') { ts_add((Token){TOK_LPAREN, "(", lineno}); p++; continue; }
            if (*p == ')') { ts_add((Token){TOK_RPAREN, ")", lineno}); p++; continue; }
            if (*p == '[') { ts_add((Token){TOK_LBRACKET, "[", lineno}); p++; continue; }
            if (*p == ']') { ts_add((Token){TOK_RBRACKET, "]", lineno}); p++; continue; }
            if (*p == '{') { ts_add((Token){TOK_LBRACE, "{", lineno}); p++; continue; }
            if (*p == '}') { ts_add((Token){TOK_RBRACE, "}", lineno}); p++; continue; }
            if (*p == ';') { ts_add((Token){TOK_SEMI, ";", lineno}); p++; continue; }
            if (*p == ',') { ts_add((Token){TOK_COMMA, ",", lineno}); p++; continue; }
            if (*p == '+') { ts_add((Token){TOK_PLUS, "+", lineno}); p++; continue; }
            if (*p == '-') { ts_add((Token){TOK_MINUS, "-", lineno}); p++; continue; }
            if (*p == '*') { ts_add((Token){TOK_STAR, "*", lineno}); p++; continue; }
            if (*p == '%') { ts_add((Token){TOK_PERCENT, "%", lineno}); p++; continue; }

            if (*p == '/') {
                char *next = p + 1;
                if (*next != '\0' && (isalnum(*next) || *next == '_' || *next == '/' || *next == '.' || *next == '-' || *next == '~')) {
                    char *start = p;
                    while (*p && (isalnum(*p) || *p == '_' || *p == '/' || *p == '.' || *p == '-' || *p == '~')) p++;
                    char buf[256]; int len = p - start;
                    memcpy(buf, start, len); buf[len] = '\0';
                    TokenType k = lookup_keyword(buf);
                    ts_add((Token){k != TOK_IDENT ? k : TOK_IDENT, strdup(buf), lineno});
                    continue;
                } else {
                    ts_add((Token){TOK_SLASH, "/", lineno});
                    p++;
                    continue;
                }
            }

            if (*p == '=') { ts_add((Token){TOK_EQ, "=", lineno}); p++; continue; }

            if (*p == '!') {
                ts_add((Token){TOK_BANG, "!", lineno});
                p++;
                continue;
            }

            // NUEVO: token para @
            if (*p == '@') {
                ts_add((Token){TOK_AT, "@", lineno});
                p++;
                continue;
            }

            // ---- MODIFICACIÓN: cadena sin cerrar genera error ----
            if (*p == '\'' || *p == '"') {
                char quote = *p++;
                char buf[4096]; int bi = 0;
                while (*p && *p != quote && *p != '\n') {   // añadimos '\n' para no leer más allá de la línea
                    if (*p == '\\' && *(p+1)) p++;
                    buf[bi++] = *p++;
                }
                buf[bi] = '\0';
                if (*p == quote) {
                    p++;   // consumir la comilla de cierre
                } else {
                    // Cadena de texto sin cerrar
                    error(lineno, "Cadena de texto sin cerrar: %c%s", quote, buf);
                }
                ts_add((Token){TOK_STRING_LITERAL, strdup(buf), lineno});
                continue;
            }
            // -------------------------------------------------------

            if (isdigit(*p) || (*p == '.' && isdigit(*(p+1)))) {
                char *start = p;
                while (isdigit(*p)) p++;
                if (*p == '.' && isdigit(*(p+1))) {
                    p++;
                    while (isdigit(*p)) p++;
                }
                char buf[128]; int len = p - start;
                memcpy(buf, start, len); buf[len] = '\0';
                ts_add((Token){TOK_NUMBER, strdup(buf), lineno});
                continue;
            }

            if (*p == '$' && (isalpha(*(p+1)) || *(p+1) == '_')) {
                char *start = p;
                p++;
                while (isalnum(*p) || *p == '_') p++;
                int len = p - start;
                char buf[256];
                memcpy(buf, start, len);
                buf[len] = '\0';
                ts_add((Token){TOK_IDENT, strdup(buf), lineno});
                continue;
            }

            if (isalpha(*p) || *p == '_' || *p == '.' || *p == '~') {
                char *start = p;
                while (isalnum(*p) || *p == '_' || *p == '/' || *p == '.' || *p == '-' || *p == '~') p++;
                char buf[256]; int len = p - start;
                memcpy(buf, start, len); buf[len] = '\0';
                TokenType k = lookup_keyword(buf);
                ts_add((Token){k != TOK_IDENT ? k : TOK_IDENT, strdup(buf), lineno});
                continue;
            }
            p++;
        }
        ts_add((Token){TOK_NEWLINE, "\n", lineno});
    }
    free(line);
}
