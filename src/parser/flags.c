/*
 * Infernal: el lenguaje de programación. Copyright (C) 2026, GPL v3+ License, Lynds Corp., Aros Legendarios, David Baña Szymaniak.
 * Código fuente de Infernal: parser/flags.c
*/

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "flags.h"
#include "core/ast.h"
#include "lexer/lexer.h"
#include "runtime/error.h"
#include "expression.h"

static bool is_valid_flag_name(const char *s) {
    return s && (isalpha(s[0]) || s[0] == '_');
}

void parse_flag_body_tokens(Token **body_tokens, int *body_count) {
    int depth = 1;
    *body_tokens = NULL;
    *body_count = 0;
    int cap = 0;

    while (depth > 0 && ts.pos < ts.count) {
        Token t = ts_advance();
        if (t.type == TOK_LBRACE) depth++;
        else if (t.type == TOK_RBRACE) {
            depth--;
            if (depth == 0) break;
        }
        if (*body_count >= cap) {
            cap = cap == 0 ? 64 : cap * 2;
            *body_tokens = realloc(*body_tokens, cap * sizeof(Token));
        }
        (*body_tokens)[(*body_count)++] = t;
    }
    if (depth != 0) {
        error(ts.tokens[ts.pos-1].line, "No se encontró '}' que cierra el bloque del flag");
    }
}

ASTNode *parse_flags() {
    if (!ts_match(TOK_LPAREN)) error(ts_peek().line, "Se esperaba '(' después de 'flags'");
    ASTNode *mode_expr = parse_expression(0);
    if (!ts_match(TOK_COMMA)) error(ts_peek().line, "Se esperaba ',' después del modo");

    ASTNode *node = node_create(NODE_FLAGS, mode_expr->line);
    node->data.flags.mode = 0;
    if (mode_expr->kind == NODE_LITERAL && mode_expr->data.lit.type == TOK_INT)
        node->data.flags.mode = mode_expr->data.lit.ival;
    else
        error(mode_expr->line, "El modo de flags debe ser 0 o 1");

    node->data.flags.specs = NULL;
    node->data.flags.spec_count = 0;

    while (!ts_match(TOK_RPAREN)) {
        ts_skip_newlines();
        FlagSpec spec;
        memset(&spec, 0, sizeof(spec));

        if (ts_peek().type == TOK_STAR) {
            ts_advance();
            spec.catch_all = true;
            if (!ts_match(TOK_LBRACE)) error(ts_peek().line, "Se esperaba '{' después de '*'");
            parse_flag_body_tokens(&spec.body_tokens, &spec.body_count);
        } else {
            char name_buf[128] = "";
            if (ts_peek().type == TOK_MINUS) {
                ts_advance();
                strcat(name_buf, "-");
                if (ts_peek().type == TOK_MINUS) {
                    ts_advance();
                    strcat(name_buf, "-");
                }
                if (!is_valid_flag_name(ts_peek().lexeme))
                    error(ts_peek().line, "Se esperaba nombre del flag después de '-'/'--'");
                strcat(name_buf, ts_advance().lexeme);
            } else if (is_valid_flag_name(ts_peek().lexeme)) {
                strcat(name_buf, ts_advance().lexeme);
            } else {
                error(ts_peek().line, "Se esperaba un flag (--nombre, -n o nombre)");
            }
            spec.names = realloc(spec.names, (++spec.name_count)*sizeof(char*));
            spec.names[spec.name_count-1] = strdup(name_buf);

            while (ts_peek().type == TOK_PIPE) {
                ts_advance();
                char alias_buf[128] = "";
                if (ts_peek().type == TOK_MINUS) {
                    ts_advance();
                    strcat(alias_buf, "-");
                    if (ts_peek().type == TOK_MINUS) {
                        ts_advance();
                        strcat(alias_buf, "-");
                    }
                    if (!is_valid_flag_name(ts_peek().lexeme))
                        error(ts_peek().line, "Se esperaba identificador para el alias");
                    strcat(alias_buf, ts_advance().lexeme);
                } else if (is_valid_flag_name(ts_peek().lexeme)) {
                    strcat(alias_buf, ts_advance().lexeme);
                } else {
                    error(ts_peek().line, "Alias debe ser un identificador, con o sin guiones");
                }
                spec.names = realloc(spec.names, (++spec.name_count)*sizeof(char*));
                spec.names[spec.name_count-1] = strdup(alias_buf);
            }

            if (ts_match(TOK_EQ)) {
                TokenType t = ts_peek().type;
                if (t == TOK_INT || t == TOK_FLOAT || t == TOK_BOOL || t == TOK_STRING || t == TOK_LIST) {
                    spec.vtype = ts_advance().type;
                    if (!is_valid_flag_name(ts_peek().lexeme))
                        error(ts_peek().line, "Se esperaba nombre de variable para el flag");
                    spec.var_name = strdup(ts_advance().lexeme);
                } else {
                    error(ts_peek().line, "Se esperaba tipo después de '=' en flag (int, float, bool, string, list)");
                }
            }

            if (!ts_match(TOK_LBRACE))
                error(ts_peek().line, "Se esperaba '{' después de la especificación del flag");
            parse_flag_body_tokens(&spec.body_tokens, &spec.body_count);
        }

        node->data.flags.specs = realloc(node->data.flags.specs,
                                          (node->data.flags.spec_count+1)*sizeof(FlagSpec));
        node->data.flags.specs[node->data.flags.spec_count++] = spec;

        ts_skip_newlines();
        if (ts_peek().type == TOK_COMMA) { ts_advance(); ts_skip_newlines(); }
    }
    return node;
}
