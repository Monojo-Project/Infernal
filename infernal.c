/*
 * infernal.c: el código fuente de Infernal
 * Infernal 0.2 Aplha
 * Infernal es un lenguaje de programación inspirado en Bash + Lua
 * GPL v3 License, Lynds Corp., Aros Legendarios, David Baña Szymaniak
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <setjmp.h>
#include <errno.h>
#include <stdarg.h>

/* ========================================================================== */
/* Forward declarations */
void error(int line, const char *fmt, ...);

/* Token types */
typedef enum {
    TOK_EOF, TOK_NEWLINE, TOK_IDENT, TOK_NUMBER, TOK_STRING_LITERAL,
    TOK_EQ, TOK_PLUS, TOK_MINUS, TOK_STAR, TOK_SLASH, TOK_PERCENT,
    TOK_EEQ, TOK_NEQ, TOK_LT, TOK_GT, TOK_LE, TOK_GE,
    TOK_LPAREN, TOK_RPAREN, TOK_LBRACKET, TOK_RBRACKET,
    TOK_SEMI, TOK_COMMA, TOK_PIPE, TOK_GT_OP, TOK_LT_OP, TOK_GGT,
    TOK_IF, TOK_THEN, TOK_FI, TOK_ELSE, TOK_ELIF,
    TOK_WHILE, TOK_FOR,
    TOK_FUNCTION, TOK_RETURN, TOK_BREAK, TOK_REPEAT,
    TOK_IMPORT, TOK_TRY, TOK_CATCH,
    TOK_INT, TOK_FLOAT, TOK_BOOL, TOK_STRING, TOK_LIST,
    TOK_TRUE, TOK_FALSE
} TokenType;

typedef struct {
    TokenType type;
    char *lexeme;
    int line;
} Token;

/* ========================================================================== */
/* AST nodes */
typedef struct ASTNode ASTNode;
typedef struct {
    ASTNode **stmts;
    int count, cap;
} NodeList;

struct ASTNode {
    int line;
    enum {
        NODE_PROGRAM, NODE_EXPR_STMT, NODE_CMD_STMT, NODE_ASSIGN,
        NODE_IF, NODE_WHILE, NODE_FOR, NODE_FUNC_DEF, NODE_RETURN,
        NODE_BREAK, NODE_REPEAT, NODE_IMPORT, NODE_TRY,
        NODE_VAR, NODE_LITERAL, NODE_BINOP, NODE_CALL, NODE_INDEX
    } kind;
    union {
        struct { NodeList stmts; } prog;
        struct { ASTNode *expr; } expr_stmt;
        struct { char *cmd; } cmd_stmt;
        struct { char *name; ASTNode *value; bool is_cmd; char *cmd_str; int vtype; } assign;
        struct { ASTNode *cond; NodeList then_block, else_block; } if_stmt;
        struct { ASTNode *cond; NodeList body; } while_stmt;
        struct { char *var; int vtype; ASTNode *init, *cond, *incr; NodeList body; } for_stmt;
        struct { char *name; char **params; int *ptypes; int param_count; NodeList body; } func;
        struct { ASTNode *expr; } ret;
        struct { char *path; NodeList module_block; } import;
        struct { NodeList try_block, catch_block; } try_stmt;
        struct { char *name; } var;
        struct { int type; int ival; double fval; int bval; char *sval; } lit;
        struct { int op; ASTNode *left, *right; } binop;
        struct { char *name; ASTNode **args; int argc; } call;
        struct { ASTNode *list, *index; } idx;
    } data;
};

/* ========================================================================== */
/* Value types and values */
typedef struct Value Value;
struct Value {
    int type;
    union {
        int ival;
        double fval;
        bool bval;
        char *sval;
        struct {
            Value *items;
            int count, cap;
        } list;
    } data;
};

#define VAL_INT     1
#define VAL_FLOAT   2
#define VAL_BOOL    3
#define VAL_STRING  4
#define VAL_LIST    5
#define VAL_NULL    0

Value val_make_null() { Value v; v.type = VAL_NULL; return v; }
Value val_int(int x) { Value v; v.type = VAL_INT; v.data.ival = x; return v; }
Value val_float(double x) { Value v; v.type = VAL_FLOAT; v.data.fval = x; return v; }
Value val_bool(bool x) { Value v; v.type = VAL_BOOL; v.data.bval = x; return v; }
Value val_string(const char *s) {
    Value v; v.type = VAL_STRING;
    v.data.sval = strdup(s ? s : "");
    return v;
}
Value val_list_empty() {
    Value v; v.type = VAL_LIST;
    v.data.list.items = NULL;
    v.data.list.count = v.data.list.cap = 0;
    return v;
}
void val_list_append(Value *list, Value item) {
    if (list->data.list.count >= list->data.list.cap) {
        list->data.list.cap = list->data.list.cap == 0 ? 4 : list->data.list.cap * 2;
        list->data.list.items = realloc(list->data.list.items, list->data.list.cap * sizeof(Value));
    }
    list->data.list.items[list->data.list.count++] = item;
}

int valtype_to_tokentype(int vtype) {
    switch (vtype) {
        case VAL_INT:    return TOK_INT;
        case VAL_FLOAT:  return TOK_FLOAT;
        case VAL_BOOL:   return TOK_BOOL;
        case VAL_STRING: return TOK_STRING;
        case VAL_LIST:   return TOK_LIST;
        default:         return 0;
    }
}

/* ========================================================================== */
/* Symbol table */
typedef struct VarEntry {
    char *name;
    int vtype;
    Value value;
    struct VarEntry *next;
} VarEntry;

typedef struct Scope {
    VarEntry *vars;
    struct Scope *parent;
} Scope;

Scope *global_scope = NULL;
Scope *current_scope = NULL;

Scope *scope_new(Scope *parent) {
    Scope *s = malloc(sizeof(Scope));
    s->vars = NULL;
    s->parent = parent;
    return s;
}

VarEntry *scope_find(Scope *scope, const char *name) {
    while (scope) {
        for (VarEntry *e = scope->vars; e; e = e->next)
            if (strcmp(e->name, name) == 0) return e;
            scope = scope->parent;
    }
    return NULL;
}

void scope_define(Scope *scope, const char *name, int vtype, Value val) {
    VarEntry *e = malloc(sizeof(VarEntry));
    e->name = strdup(name);
    e->vtype = vtype ? vtype : valtype_to_tokentype(val.type);
    e->value = val;
    e->next = scope->vars;
    scope->vars = e;
}

void scope_assign(Scope *scope, const char *name, Value val, int line) {
    VarEntry *e = scope_find(scope, name);
    if (e) {
        int expected = e->vtype;
        int new_type = valtype_to_tokentype(val.type);
        if (expected != 0 && new_type != expected) {
            error(line, "Tipado fuerte: la variable '%s' es de tipo %s, no se puede asignar un valor de tipo %s",
                  name,
                  expected == TOK_INT ? "int" : expected == TOK_FLOAT ? "float" :
                  expected == TOK_BOOL ? "bool" : expected == TOK_STRING ? "string" :
                  expected == TOK_LIST ? "list" : "?",
                  new_type == TOK_INT ? "int" : new_type == TOK_FLOAT ? "float" :
                  new_type == TOK_BOOL ? "bool" : new_type == TOK_STRING ? "string" :
                  new_type == TOK_LIST ? "list" : "?");
        }
        e->value = val;
    } else {
        scope_define(scope, name, 0, val);
    }
}

/* ========================================================================== */
/* Function table */
typedef struct FuncEntry {
    char *name;
    ASTNode *def;
    struct FuncEntry *next;
} FuncEntry;

FuncEntry *func_table = NULL;

void func_register(const char *name, ASTNode *def) {
    FuncEntry *e = malloc(sizeof(FuncEntry));
    e->name = strdup(name);
    e->def = def;
    e->next = func_table;
    func_table = e;
}

ASTNode *func_lookup(const char *name) {
    for (FuncEntry *e = func_table; e; e = e->next)
        if (strcmp(e->name, name) == 0) return e->def;
        return NULL;
}

/* ========================================================================== */
/* Loop iteration limit */
int max_loop_iterations = 10000;

/* ========================================================================== */
/* Error handling & control flow */
jmp_buf exception_env;
int exception_raised = 0;
char exception_msg[256];

void error(int line, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int len = vsnprintf(exception_msg, sizeof(exception_msg), fmt, ap);
    va_end(ap);
    if (line > 0) {
        snprintf(exception_msg + len, sizeof(exception_msg) - len, " (línea %d)", line);
    }
    exception_raised = 1;
    longjmp(exception_env, 1);
}

enum { CF_NONE, CF_RETURN, CF_BREAK, CF_REPEAT };
int control_flow = CF_NONE;
Value return_value;

/* ========================================================================== */
/* Lexer */
typedef struct {
    Token *tokens;
    int count, cap;
    int pos;
} TokenStream;

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

/* Keyword structure */
typedef struct {
    const char *word;
    TokenType tok;
} Keyword;

Keyword keywords[] = {
    {"if", TOK_IF}, {"then", TOK_THEN}, {"fi", TOK_FI},
    {"else", TOK_ELSE}, {"elif", TOK_ELIF}, {"elseif", TOK_ELIF},
    {"while", TOK_WHILE}, {"for", TOK_FOR},
    {"function", TOK_FUNCTION}, {"return", TOK_RETURN},
    {"break", TOK_BREAK}, {"repeat", TOK_REPEAT},
    {"import", TOK_IMPORT}, {"try", TOK_TRY}, {"catch", TOK_CATCH},
    {"int", TOK_INT}, {"float", TOK_FLOAT}, {"bool", TOK_BOOL},
    {"string", TOK_STRING}, {"list", TOK_LIST},
    {"true", TOK_TRUE}, {"false", TOK_FALSE},
    {"True", TOK_TRUE}, {"False", TOK_FALSE},
    {NULL, TOK_EOF}
};

TokenType lookup_keyword(const char *s) {
    for (int i = 0; keywords[i].word; i++)
        if (strcmp(keywords[i].word, s) == 0) return keywords[i].tok;
        return TOK_IDENT;
}

/* Tokenizer with block comments support (and error on unclosed comment) */
void tokenize_file(FILE *fp) {
    char *line = NULL;
    size_t len = 0;
    int lineno = 0;
    bool in_block_comment = false;
    int block_comment_start_line = 0;

    while (getline(&line, &len, fp) != -1) {
        lineno++;
        char *p = line;

        if (in_block_comment) {
            char *close = strstr(p, "###");
            if (close) {
                in_block_comment = false;
                p = close + 3;
            } else continue;
        }

        while (*p) {
            if (*p == '#' && *(p+1) == '#' && *(p+2) == '#') {
                block_comment_start_line = lineno;
                char *close = strstr(p + 3, "###");
                if (close) {
                    p = close + 3;
                    continue;
                } else {
                    in_block_comment = true;
                    break;
                }
            }
            if (*p == '#') break;
            if (isspace(*p)) { p++; continue; }

            if (*p == '=' && *(p+1) == '=') { ts_add((Token){TOK_EEQ, "==", lineno}); p+=2; continue; }
            if (*p == '!' && *(p+1) == '=') { ts_add((Token){TOK_NEQ, "!=", lineno}); p+=2; continue; }
            if (*p == '<' && *(p+1) == '=') { ts_add((Token){TOK_LE, "<=", lineno}); p+=2; continue; }
            if (*p == '=' && *(p+1) == '<') { ts_add((Token){TOK_LE, "=<", lineno}); p+=2; continue; }
            if (*p == '>' && *(p+1) == '=') { ts_add((Token){TOK_GE, ">=", lineno}); p+=2; continue; }
            if (*p == '>' && *(p+1) == '>') { ts_add((Token){TOK_GGT, ">>", lineno}); p+=2; continue; }
            if (*p == '|') { ts_add((Token){TOK_PIPE, "|", lineno}); p++; continue; }
            if (*p == '>') { ts_add((Token){TOK_GT_OP, ">", lineno}); p++; continue; }
            if (*p == '<') { ts_add((Token){TOK_LT_OP, "<", lineno}); p++; continue; }
            if (*p == '(') { ts_add((Token){TOK_LPAREN, "(", lineno}); p++; continue; }
            if (*p == ')') { ts_add((Token){TOK_RPAREN, ")", lineno}); p++; continue; }
            if (*p == '[') { ts_add((Token){TOK_LBRACKET, "[", lineno}); p++; continue; }
            if (*p == ']') { ts_add((Token){TOK_RBRACKET, "]", lineno}); p++; continue; }
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

            if (*p == '\'' || *p == '"') {
                char quote = *p++;
                char buf[4096]; int bi = 0;
                while (*p && *p != quote) {
                    if (*p == '\\' && *(p+1)) p++;
                    buf[bi++] = *p++;
                }
                buf[bi] = '\0';
                if (*p == quote) p++;
                ts_add((Token){TOK_STRING_LITERAL, strdup(buf), lineno});
                continue;
            }

            if (isdigit(*p) || (*p == '.' && isdigit(*(p+1)))) {
                char *start = p;
                bool is_float = false;
                while (isdigit(*p)) p++;
                if (*p == '.' && isdigit(*(p+1))) { is_float = true; p++; while (isdigit(*p)) p++; }
                char buf[128]; int len = p - start;
                memcpy(buf, start, len); buf[len] = '\0';
                ts_add((Token){TOK_NUMBER, strdup(buf), lineno});
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

    if (in_block_comment) {
        error(block_comment_start_line, "Comentario de bloque sin cerrar");
    }
}

/* ========================================================================== */
/* Parser */
ASTNode *parse_expression(int);

char *current_import_prefix = NULL;

ASTNode *node_create(int kind, int line) {
    ASTNode *n = calloc(1, sizeof(ASTNode));
    n->kind = kind;
    n->line = line;
    return n;
}

void nodelist_add(NodeList *list, ASTNode *node) {
    if (list->count >= list->cap) {
        list->cap = list->cap == 0 ? 8 : list->cap * 2;
        list->stmts = realloc(list->stmts, list->cap * sizeof(ASTNode*));
    }
    list->stmts[list->count++] = node;
}

ASTNode *parse_primary() {
    Token t = ts_peek();
    if (t.type == TOK_NUMBER) {
        ts_advance();
        ASTNode *n = node_create(NODE_LITERAL, t.line);
        if (strchr(t.lexeme, '.') || strchr(t.lexeme, 'e') || strchr(t.lexeme, 'E')) {
            n->data.lit.type = TOK_FLOAT;
            n->data.lit.fval = atof(t.lexeme);
        } else {
            n->data.lit.type = TOK_INT;
            n->data.lit.ival = atoi(t.lexeme);
        }
        return n;
    }
    if (t.type == TOK_STRING_LITERAL) {
        ts_advance();
        ASTNode *n = node_create(NODE_LITERAL, t.line);
        n->data.lit.type = TOK_STRING;
        n->data.lit.sval = strdup(t.lexeme);
        return n;
    }
    if (t.type == TOK_TRUE || t.type == TOK_FALSE) {
        ts_advance();
        ASTNode *n = node_create(NODE_LITERAL, t.line);
        n->data.lit.type = TOK_BOOL;
        n->data.lit.bval = (t.type == TOK_TRUE) ? 1 : 0;
        return n;
    }
    if (t.type == TOK_IDENT) {
        ts_advance();
        if (func_lookup(t.lexeme)) {
            ASTNode *n = node_create(NODE_CALL, t.line);
            n->data.call.name = strdup(t.lexeme);
            n->data.call.argc = 0;
            n->data.call.args = NULL;
            if (ts_match(TOK_LPAREN)) {
                if (!ts_match(TOK_RPAREN)) {
                    do {
                        n->data.call.args = realloc(n->data.call.args, (n->data.call.argc+1)*sizeof(ASTNode*));
                        n->data.call.args[n->data.call.argc++] = parse_expression(0);
                    } while (ts_match(TOK_COMMA));
                    if (!ts_match(TOK_RPAREN)) error(t.line, "Se esperaba ')' en la llamada a función '%s'", t.lexeme);
                }
            } else {
                if (ts_peek().type != TOK_NEWLINE && ts_peek().type != TOK_FI && ts_peek().type != TOK_EOF) {
                    n->data.call.args = malloc(sizeof(ASTNode*));
                    n->data.call.args[0] = parse_expression(0);
                    n->data.call.argc = 1;
                }
            }
            return n;
        }
        ASTNode *n = node_create(NODE_VAR, t.line);
        n->data.var.name = strdup(t.lexeme);
        while (ts_match(TOK_LBRACKET)) {
            ASTNode *idx = parse_expression(0);
            if (!ts_match(TOK_RBRACKET)) error(t.line, "Se esperaba ']'");
            ASTNode *ni = node_create(NODE_INDEX, t.line);
            ni->data.idx.list = n;
            ni->data.idx.index = idx;
            n = ni;
        }
        return n;
    }
    if (t.type == TOK_LPAREN) {
        ts_advance();
        ASTNode *n = parse_expression(0);
        if (!ts_match(TOK_RPAREN)) error(t.line, "Se esperaba ')'");
        return n;
    }
    error(t.line, "Token inesperado '%s'", t.lexeme);
    return NULL;
}

ASTNode *parse_unary() {
    Token t = ts_peek();
    if (ts_match(TOK_MINUS)) {
        ASTNode *right = parse_unary();
        ASTNode *n = node_create(NODE_BINOP, t.line);
        n->data.binop.op = TOK_MINUS;
        n->data.binop.left = node_create(NODE_LITERAL, t.line);
        n->data.binop.left->data.lit.type = TOK_INT;
        n->data.binop.left->data.lit.ival = 0;
        n->data.binop.right = right;
        return n;
    }
    return parse_primary();
}

ASTNode *parse_term() {
    ASTNode *left = parse_unary();
    while (ts_peek().type == TOK_STAR || ts_peek().type == TOK_SLASH || ts_peek().type == TOK_PERCENT) {
        Token op = ts_advance();
        ASTNode *right = parse_unary();
        ASTNode *n = node_create(NODE_BINOP, op.line);
        n->data.binop.op = op.type;
        n->data.binop.left = left;
        n->data.binop.right = right;
        left = n;
    }
    return left;
}

ASTNode *parse_expr() {
    ASTNode *left = parse_term();
    while (ts_peek().type == TOK_PLUS || ts_peek().type == TOK_MINUS) {
        Token op = ts_advance();
        ASTNode *right = parse_term();
        ASTNode *n = node_create(NODE_BINOP, op.line);
        n->data.binop.op = op.type;
        n->data.binop.left = left;
        n->data.binop.right = right;
        left = n;
    }
    return left;
}

ASTNode *parse_comparison() {
    ASTNode *left = parse_expr();
    TokenType op = ts_peek().type;
    if (op == TOK_EEQ || op == TOK_NEQ || op == TOK_LT || op == TOK_GT || op == TOK_LE || op == TOK_GE) {
        Token t = ts_advance();
        ASTNode *right = parse_expr();
        ASTNode *n = node_create(NODE_BINOP, t.line);
        n->data.binop.op = t.type;
        n->data.binop.left = left;
        n->data.binop.right = right;
        return n;
    }
    return left;
}

ASTNode *parse_expression(int _) { return parse_comparison(); }

NodeList parse_block(const char *terminator) {
    NodeList block = {NULL, 0, 0};
    while (1) {
        ts_skip_newlines();
        Token t = ts_peek();
        if (t.type == TOK_EOF) break;
        if (terminator && lookup_keyword(terminator) == t.type) break;
        // Para bloques terminados con "fi", también salir con else/elif
        if (terminator && strcmp(terminator, "fi") == 0) {
            if (t.type == TOK_ELSE || t.type == TOK_ELIF) break;
            if (t.type == TOK_IDENT && (strcmp(t.lexeme, "elif") == 0 || strcmp(t.lexeme, "elseif") == 0)) break;
        }

        ASTNode *stmt = NULL;

        if (t.type == TOK_IF) {
            ts_advance(); // consume 'if'
            ASTNode *cond = parse_expression(0);
            if (!ts_match(TOK_THEN)) error(t.line, "Se esperaba 'then'");
            NodeList then_block = parse_block("fi"); // se detiene en else/elif/fi

            // Nodo principal del if
            ASTNode *if_node = node_create(NODE_IF, t.line);
            if_node->data.if_stmt.cond = cond;
            if_node->data.if_stmt.then_block = then_block;
            if_node->data.if_stmt.else_block = (NodeList){NULL,0,0};

            ASTNode *current_if = if_node;

            // Procesar cadena de else / else if / elif
            while (1) {
                if (ts_match(TOK_ELIF)) {                     // 'elif' o 'elseif'
                    ASTNode *elif_cond = parse_expression(0);
                    if (!ts_match(TOK_THEN)) error(t.line, "Se esperaba 'then'");
                    NodeList elif_then = parse_block("fi");
                    ASTNode *elif_node = node_create(NODE_IF, t.line);
                    elif_node->data.if_stmt.cond = elif_cond;
                    elif_node->data.if_stmt.then_block = elif_then;
                    elif_node->data.if_stmt.else_block = (NodeList){NULL,0,0};
                    nodelist_add(&current_if->data.if_stmt.else_block, elif_node);
                    current_if = elif_node;
                } else if (ts_match(TOK_ELSE)) {
                    if (ts_peek().type == TOK_IF) {           // 'else if'
                        ts_advance();
                        ASTNode *elseif_cond = parse_expression(0);
                        if (!ts_match(TOK_THEN)) error(t.line, "Se esperaba 'then'");
                        NodeList elseif_then = parse_block("fi");
                        ASTNode *elseif_node = node_create(NODE_IF, t.line);
                        elseif_node->data.if_stmt.cond = elseif_cond;
                        elseif_node->data.if_stmt.then_block = elseif_then;
                        elseif_node->data.if_stmt.else_block = (NodeList){NULL,0,0};
                        nodelist_add(&current_if->data.if_stmt.else_block, elseif_node);
                        current_if = elseif_node;
                    } else {                                   // 'else' simple
                        NodeList plain_else = parse_block("fi");
                        current_if->data.if_stmt.else_block = plain_else;
                        break; // después de else simple no puede haber más
                    }
                } else {
                    break;
                }
            }

            if (!ts_match(TOK_FI)) error(t.line, "Se esperaba 'fi'");
            stmt = if_node;

        } else if (t.type == TOK_WHILE) {
            ts_advance();
            ASTNode *cond = parse_expression(0);
            if (!ts_match(TOK_THEN)) error(t.line, "Se esperaba 'then'");
            NodeList body = parse_block("fi");
            if (!ts_match(TOK_FI)) error(t.line, "Se esperaba 'fi'");
            stmt = node_create(NODE_WHILE, t.line);
            stmt->data.while_stmt.cond = cond;
            stmt->data.while_stmt.body = body;
        } else if (t.type == TOK_FOR) {
            ts_advance();
            int vtype = 0;
            if (ts_peek().type == TOK_INT || ts_peek().type == TOK_FLOAT || ts_peek().type == TOK_BOOL || ts_peek().type == TOK_STRING || ts_peek().type == TOK_LIST)
                vtype = ts_advance().type;
            if (ts_peek().type != TOK_IDENT) error(t.line, "Se esperaba variable en for");
            char *varname = strdup(ts_advance().lexeme);
            if (!ts_match(TOK_EQ)) error(t.line, "Se esperaba '=' en for");
            ASTNode *init = parse_expression(0);
            if (!ts_match(TOK_SEMI)) error(t.line, "Se esperaba ';' después de inicialización");
            ASTNode *cond = parse_expression(0);
            if (!ts_match(TOK_SEMI)) error(t.line, "Se esperaba ';' después de condición");
            ASTNode *incr = parse_expression(0);
            if (!ts_match(TOK_THEN)) error(t.line, "Se esperaba 'then'");
            NodeList body = parse_block("fi");
            if (!ts_match(TOK_FI)) error(t.line, "Se esperaba 'fi'");
            stmt = node_create(NODE_FOR, t.line);
            stmt->data.for_stmt.var = varname;
            stmt->data.for_stmt.vtype = vtype;
            stmt->data.for_stmt.init = init;
            stmt->data.for_stmt.cond = cond;
            stmt->data.for_stmt.incr = incr;
            stmt->data.for_stmt.body = body;
        } else if (t.type == TOK_FUNCTION) {
            ts_advance();
            if (ts_peek().type != TOK_IDENT) error(t.line, "Se esperaba nombre de función");
            char *fname = strdup(ts_advance().lexeme);
            if (!ts_match(TOK_LPAREN)) error(t.line, "Se esperaba '('");
            char **params = NULL; int *ptypes = NULL; int pcount = 0;
            if (!ts_match(TOK_RPAREN)) {
                do {
                    int ptype = 0;
                    TokenType tt = ts_peek().type;
                    if (tt == TOK_INT || tt == TOK_FLOAT || tt == TOK_BOOL || tt == TOK_STRING || tt == TOK_LIST)
                        ptype = ts_advance().type;
                    if (ts_peek().type != TOK_IDENT) error(t.line, "Se esperaba nombre de parámetro");
                    char *pname = strdup(ts_advance().lexeme);
                    params = realloc(params, (pcount+1)*sizeof(char*));
                    ptypes = realloc(ptypes, (pcount+1)*sizeof(int));
                    params[pcount] = pname;
                    ptypes[pcount] = ptype;
                    pcount++;
                } while (ts_match(TOK_COMMA));
                if (!ts_match(TOK_RPAREN)) error(t.line, "Se esperaba ')'");
            }
            NodeList body = parse_block("fi");
            if (!ts_match(TOK_FI)) error(t.line, "Se esperaba 'fi'");
            stmt = node_create(NODE_FUNC_DEF, t.line);
            stmt->data.func.name = fname;
            stmt->data.func.params = params;
            stmt->data.func.ptypes = ptypes;
            stmt->data.func.param_count = pcount;
            stmt->data.func.body = body;
            func_register(fname, stmt);
            if (current_import_prefix) {
                char prefixed[512];
                snprintf(prefixed, sizeof(prefixed), "%s.%s", current_import_prefix, fname);
                func_register(strdup(prefixed), stmt);
            }
        } else if (t.type == TOK_RETURN) {
            ts_advance();
            ASTNode *expr = NULL;
            if (ts_peek().type != TOK_NEWLINE && ts_peek().type != TOK_FI && ts_peek().type != TOK_EOF)
                expr = parse_expression(0);
            stmt = node_create(NODE_RETURN, t.line);
            stmt->data.ret.expr = expr;
        } else if (t.type == TOK_BREAK) {
            ts_advance(); stmt = node_create(NODE_BREAK, t.line);
        } else if (t.type == TOK_REPEAT) {
            ts_advance(); stmt = node_create(NODE_REPEAT, t.line);
        } else if (t.type == TOK_IMPORT) {
            ts_advance();
            Token nt = ts_peek();
            char *path = NULL;
            if (nt.type == TOK_STRING_LITERAL) {
                ts_advance(); path = strdup(nt.lexeme);
            } else if (nt.type == TOK_IDENT) {
                ts_advance();
                char buf[512];
                snprintf(buf, sizeof(buf), "/usr/share/infernal/fire/%s.fire", nt.lexeme);
                path = strdup(buf);
            } else error(t.line, "Se esperaba nombre o ruta en import");

            FILE *fp = fopen(path, "r");
            if (!fp) error(t.line, "No se pudo abrir módulo: %s", path);
            TokenStream old_ts = ts; ts_init(); tokenize_file(fp); fclose(fp);
            char *module_name = strdup(path);
            char *base = strrchr(module_name, '/');
            if (base) base++; else base = module_name;
            char *dot = strrchr(base, '.');
            if (dot) *dot = '\0';
            char *old_prefix = current_import_prefix;
            current_import_prefix = base;
            NodeList module_block = parse_block(NULL);
            current_import_prefix = old_prefix;
            ts = old_ts;
            free(module_name);
            free(path);

            stmt = node_create(NODE_IMPORT, t.line);
            stmt->data.import.path = NULL;
            stmt->data.import.module_block = module_block;
        } else if (t.type == TOK_TRY) {
            ts_advance();
            NodeList try_block = parse_block("catch");
            if (!ts_match(TOK_CATCH)) error(t.line, "Se esperaba 'catch'");
            NodeList catch_block = parse_block("fi");
            if (!ts_match(TOK_FI)) error(t.line, "Se esperaba 'fi'");
            stmt = node_create(NODE_TRY, t.line);
            stmt->data.try_stmt.try_block = try_block;
            stmt->data.try_stmt.catch_block = catch_block;
        } else if (t.type == TOK_IDENT) {
            ts_advance();
            if (ts_match(TOK_EQ)) {
                if (ts_peek().type == TOK_IDENT &&
                    !func_lookup(ts_peek().lexeme) &&
                    !scope_find(current_scope, ts_peek().lexeme) &&
                    (ts.pos + 1 >= ts.count || ts.tokens[ts.pos + 1].type == TOK_NEWLINE || ts.tokens[ts.pos + 1].type == TOK_EOF)) {
                    char *cmd_name = strdup(ts_peek().lexeme);
                ts_advance();
                stmt = node_create(NODE_ASSIGN, t.line);
                stmt->data.assign.name = strdup(t.lexeme);
                stmt->data.assign.is_cmd = true;
                stmt->data.assign.cmd_str = cmd_name;
                stmt->data.assign.vtype = 0;
                    } else {
                        int saved = ts.pos;
                        ASTNode *value_expr = NULL;
                        bool is_cmd = false;
                        jmp_buf saved_env; memcpy(&saved_env, &exception_env, sizeof(jmp_buf));
                        int saved_raised = exception_raised;
                        exception_raised = 0;
                        if (!setjmp(exception_env)) {
                            value_expr = parse_expression(0);
                            if (ts_peek().type == TOK_NEWLINE || ts_peek().type == TOK_EOF || ts_peek().type == TOK_FI)
                                is_cmd = false;
                            else
                                is_cmd = true;
                        } else {
                            is_cmd = true;
                            exception_raised = 0;
                        }
                        memcpy(&exception_env, &saved_env, sizeof(jmp_buf));
                        exception_raised = saved_raised;

                        stmt = node_create(NODE_ASSIGN, t.line);
                        stmt->data.assign.name = strdup(t.lexeme);
                        stmt->data.assign.vtype = 0;
                        if (is_cmd) {
                            ts.pos = saved;
                            char cmd[4096] = {0};
                            while (ts_peek().type != TOK_NEWLINE && ts_peek().type != TOK_EOF) {
                                Token ct = ts_advance();
                                if (cmd[0]) strcat(cmd, " ");
                                if (ct.type == TOK_STRING_LITERAL) {
                                    strcat(cmd, "\""); strcat(cmd, ct.lexeme); strcat(cmd, "\"");
                                } else strcat(cmd, ct.lexeme);
                            }
                            stmt->data.assign.is_cmd = true;
                            stmt->data.assign.cmd_str = strdup(cmd);
                        } else {
                            stmt->data.assign.is_cmd = false;
                            stmt->data.assign.value = value_expr;
                        }
                    }
            } else {
                ts.pos--;
                if (ts_peek().type == TOK_IDENT) {
                    TokenType next = (ts.pos + 1 < ts.count) ? ts.tokens[ts.pos + 1].type : TOK_EOF;
                    if (next != TOK_EQ && next != TOK_LPAREN && next != TOK_LBRACKET &&
                        next != TOK_NEWLINE && next != TOK_EOF && next != TOK_FI) {
                        char cmd[4096] = {0};
                    while (ts_peek().type != TOK_NEWLINE && ts_peek().type != TOK_EOF) {
                        Token ct = ts_advance();
                        if (cmd[0]) strcat(cmd, " ");
                        if (ct.type == TOK_STRING_LITERAL) {
                            strcat(cmd, "\""); strcat(cmd, ct.lexeme); strcat(cmd, "\"");
                        } else strcat(cmd, ct.lexeme);
                    }
                    stmt = node_create(NODE_CMD_STMT, t.line);
                    stmt->data.cmd_stmt.cmd = strdup(cmd);
                    goto stmt_done;
                        }
                }

                if (ts_peek().type == TOK_IDENT) {
                    char *name = ts_peek().lexeme;
                    if (ts.pos + 1 < ts.count && ts.tokens[ts.pos + 1].type == TOK_LPAREN) {
                        if (!func_lookup(name)) {
                            error(t.line, "Función no definida: %s", name);
                        }
                    }
                }

                if (ts_peek().type == TOK_IDENT &&
                    !func_lookup(ts_peek().lexeme) &&
                    !scope_find(current_scope, ts_peek().lexeme) &&
                    (ts.pos + 1 >= ts.count || ts.tokens[ts.pos + 1].type == TOK_NEWLINE || ts.tokens[ts.pos + 1].type == TOK_EOF)) {
                    char cmd_name[256];
                strcpy(cmd_name, ts_peek().lexeme);
                ts_advance();
                stmt = node_create(NODE_CMD_STMT, t.line);
                stmt->data.cmd_stmt.cmd = strdup(cmd_name);
                goto stmt_done;
                    }

                    int saved = ts.pos;
                    jmp_buf saved_env; memcpy(&saved_env, &exception_env, sizeof(jmp_buf));
                    int saved_raised = exception_raised;
                    exception_raised = 0;
                    if (!setjmp(exception_env)) {
                        ASTNode *expr = parse_expression(0);
                        if (ts_peek().type == TOK_NEWLINE || ts_peek().type == TOK_EOF || ts_peek().type == TOK_FI) {
                            stmt = node_create(NODE_EXPR_STMT, t.line);
                            stmt->data.expr_stmt.expr = expr;
                            memcpy(&exception_env, &saved_env, sizeof(jmp_buf));
                            exception_raised = saved_raised;
                            goto stmt_done;
                        }
                    }
                    ts.pos = saved;
                    exception_raised = 0;
                    char cmd[4096] = {0};
                    Token first = ts_advance();
                    strcpy(cmd, first.lexeme);
                    while (ts_peek().type != TOK_NEWLINE && ts_peek().type != TOK_EOF) {
                        Token ct = ts_advance();
                        strcat(cmd, " ");
                        if (ct.type == TOK_STRING_LITERAL) {
                            strcat(cmd, "\""); strcat(cmd, ct.lexeme); strcat(cmd, "\"");
                        } else strcat(cmd, ct.lexeme);
                    }
                    stmt = node_create(NODE_CMD_STMT, t.line);
                    stmt->data.cmd_stmt.cmd = strdup(cmd);
                    memcpy(&exception_env, &saved_env, sizeof(jmp_buf));
                    exception_raised = saved_raised;
            }
        } else if (t.type == TOK_INT || t.type == TOK_FLOAT || t.type == TOK_BOOL || t.type == TOK_STRING || t.type == TOK_LIST) {
            int vtype = ts_advance().type;
            if (ts_peek().type != TOK_IDENT) error(t.line, "Se esperaba nombre de variable");
            char *vname = strdup(ts_advance().lexeme);
            if (!ts_match(TOK_EQ)) error(t.line, "Se esperaba '='");
            ASTNode *value = parse_expression(0);
            stmt = node_create(NODE_ASSIGN, t.line);
            stmt->data.assign.name = vname;
            stmt->data.assign.is_cmd = false;
            stmt->data.assign.value = value;
            stmt->data.assign.vtype = vtype;
        } else {
            error(t.line, "Sentencia no reconocida '%s'", t.lexeme);
        }
        stmt_done:
        if (stmt) nodelist_add(&block, stmt);
        ts_skip_newlines();
        if (terminator && ts_peek().type == lookup_keyword(terminator)) break;
        // Para bloques "fi", también salir si encontramos else/elif (por si no se consumió antes)
        if (terminator && strcmp(terminator, "fi") == 0) {
            TokenType nt = ts_peek().type;
            if (nt == TOK_ELSE || nt == TOK_ELIF) break;
            if (nt == TOK_IDENT && (strcmp(ts_peek().lexeme, "elif") == 0 || strcmp(ts_peek().lexeme, "elseif") == 0)) break;
        }
    }
    return block;
}

/* ========================================================================== */
/* Evaluator */
Value eval_expr(ASTNode *expr);

void exec_block(NodeList *block) {
    for (int i = 0; i < block->count; i++) {
        ASTNode *stmt = block->stmts[i];
        switch (stmt->kind) {
            case NODE_EXPR_STMT:
                eval_expr(stmt->data.expr_stmt.expr);
                break;
            case NODE_CMD_STMT: {
                int ret = system(stmt->data.cmd_stmt.cmd);
                if (ret != 0) error(stmt->line, "Comando falló: %s", stmt->data.cmd_stmt.cmd);
                break;
            }
            case NODE_ASSIGN: {
                Value val;
                if (stmt->data.assign.is_cmd) {
                    FILE *fp = popen(stmt->data.assign.cmd_str, "r");
                    if (!fp) error(stmt->line, "Error al ejecutar comando: %s", stmt->data.assign.cmd_str);
                    char buf[4096]; char *out = strdup("");
                    while (fgets(buf, sizeof(buf), fp)) {
                        out = realloc(out, strlen(out) + strlen(buf) + 1);
                        strcat(out, buf);
                    }
                    int status = pclose(fp);
                    if (status != 0) error(stmt->line, "Comando falló: %s", stmt->data.assign.cmd_str);
                    size_t len = strlen(out);
                    if (len > 0 && out[len-1] == '\n') out[len-1] = '\0';
                    val = val_string(out);
                    free(out);
                } else {
                    val = eval_expr(stmt->data.assign.value);
                }
                VarEntry *var = scope_find(current_scope, stmt->data.assign.name);
                if (var) {
                    scope_assign(current_scope, stmt->data.assign.name, val, stmt->line);
                } else {
                    scope_define(current_scope, stmt->data.assign.name, stmt->data.assign.vtype, val);
                }
                break;
            }
            case NODE_IF: {
                Value cond = eval_expr(stmt->data.if_stmt.cond);
                if (cond.type == VAL_BOOL ? cond.data.bval : (cond.type == VAL_INT && cond.data.ival))
                    exec_block(&stmt->data.if_stmt.then_block);
                else
                    exec_block(&stmt->data.if_stmt.else_block);
                break;
            }
            case NODE_WHILE: {
                int iter_count = 0;
                while (1) {
                    if (iter_count >= max_loop_iterations)
                        error(stmt->line, "Límite de iteraciones (%d) alcanzado en bucle while", max_loop_iterations);
                    iter_count++;
                    Value cond = eval_expr(stmt->data.while_stmt.cond);
                    if (!(cond.type == VAL_BOOL ? cond.data.bval : (cond.type == VAL_INT && cond.data.ival))) break;
                    exec_block(&stmt->data.while_stmt.body);
                    if (control_flow == CF_BREAK) { control_flow = CF_NONE; break; }
                    if (control_flow == CF_REPEAT) { control_flow = CF_NONE; continue; }
                    if (control_flow == CF_RETURN) return;
                }
                break;
            }
            case NODE_FOR: {
                Value init_val = eval_expr(stmt->data.for_stmt.init);
                scope_define(current_scope, stmt->data.for_stmt.var, stmt->data.for_stmt.vtype, init_val);
                int iter_count = 0;
                while (1) {
                    if (iter_count >= max_loop_iterations)
                        error(stmt->line, "Límite de iteraciones (%d) alcanzado en bucle for", max_loop_iterations);
                    iter_count++;
                    Value cond = eval_expr(stmt->data.for_stmt.cond);
                    if (!(cond.type == VAL_BOOL ? cond.data.bval : (cond.type == VAL_INT && cond.data.ival))) break;
                    exec_block(&stmt->data.for_stmt.body);
                    if (control_flow == CF_BREAK) { control_flow = CF_NONE; break; }
                    if (control_flow == CF_REPEAT) { control_flow = CF_NONE; }
                    if (control_flow == CF_RETURN) return;
                    eval_expr(stmt->data.for_stmt.incr);
                }
                break;
            }
            case NODE_FUNC_DEF: break;
            case NODE_RETURN:
                return_value = stmt->data.ret.expr ? eval_expr(stmt->data.ret.expr) : val_make_null();
                control_flow = CF_RETURN;
                return;
            case NODE_BREAK: control_flow = CF_BREAK; return;
            case NODE_REPEAT: control_flow = CF_REPEAT; return;
            case NODE_IMPORT: {
                Scope *old_scope = current_scope; current_scope = global_scope;
                exec_block(&stmt->data.import.module_block);
                current_scope = old_scope;
                break;
            }
            case NODE_TRY: {
                jmp_buf saved_env; memcpy(&saved_env, &exception_env, sizeof(jmp_buf));
                int saved_raised = exception_raised; exception_raised = 0;
                if (!setjmp(exception_env)) {
                    exec_block(&stmt->data.try_stmt.try_block);
                } else {
                    exception_raised = 0;
                    exec_block(&stmt->data.try_stmt.catch_block);
                }
                memcpy(&exception_env, &saved_env, sizeof(jmp_buf));
                exception_raised = saved_raised;
                break;
            }
            default: error(stmt->line, "Sentencia no implementada");
        }
        if (control_flow != CF_NONE) break;
    }
}

Value eval_expr(ASTNode *expr) {
    switch (expr->kind) {
        case NODE_LITERAL: {
            if (expr->data.lit.type == TOK_INT) return val_int(expr->data.lit.ival);
            if (expr->data.lit.type == TOK_FLOAT) return val_float(expr->data.lit.fval);
            if (expr->data.lit.type == TOK_BOOL) return val_bool(expr->data.lit.bval);
            if (expr->data.lit.type == TOK_STRING) return val_string(expr->data.lit.sval);
            return val_make_null();
        }
        case NODE_VAR: {
            VarEntry *e = scope_find(current_scope, expr->data.var.name);
            if (!e) error(expr->line, "Variable no definida: %s", expr->data.var.name);
            return e->value;
        }
        case NODE_BINOP: {
            Value left = eval_expr(expr->data.binop.left);
            Value right = eval_expr(expr->data.binop.right);

            if (left.type == VAL_STRING || right.type == VAL_STRING) {
                char buf[1024] = {0};
                if (left.type == VAL_STRING) strcat(buf, left.data.sval);
                else sprintf(buf + strlen(buf), "%d", left.type == VAL_INT ? left.data.ival : left.type == VAL_FLOAT ? (int)left.data.fval : left.data.bval ? 1 : 0);
                if (right.type == VAL_STRING) strcat(buf, right.data.sval);
                else sprintf(buf + strlen(buf), "%d", right.type == VAL_INT ? right.data.ival : right.type == VAL_FLOAT ? (int)right.data.fval : right.data.bval ? 1 : 0);
                return val_string(buf);
            }

            if (left.type == VAL_INT && right.type == VAL_INT) {
                int lv = left.data.ival, rv = right.data.ival;
                switch (expr->data.binop.op) {
                    case TOK_PLUS:  return val_int(lv + rv);
                    case TOK_MINUS: return val_int(lv - rv);
                    case TOK_STAR:  return val_int(lv * rv);
                    case TOK_SLASH: if (rv == 0) error(expr->line, "División por cero"); return val_float((double)lv / rv);
                    case TOK_PERCENT: if (rv == 0) error(expr->line, "Módulo por cero"); return val_int(lv % rv);
                    case TOK_EEQ: return val_bool(lv == rv);
                    case TOK_NEQ: return val_bool(lv != rv);
                    case TOK_LT:  return val_bool(lv < rv);
                    case TOK_GT:  return val_bool(lv > rv);
                    case TOK_LE:  return val_bool(lv <= rv);
                    case TOK_GE:  return val_bool(lv >= rv);
                    default: error(expr->line, "Operador no soportado");
                }
            }

            double lv = left.type == VAL_INT ? left.data.ival : left.type == VAL_FLOAT ? left.data.fval : left.data.bval ? 1.0 : 0.0;
            double rv = right.type == VAL_INT ? right.data.ival : right.type == VAL_FLOAT ? right.data.fval : right.data.bval ? 1.0 : 0.0;
            switch (expr->data.binop.op) {
                case TOK_PLUS: return val_float(lv + rv);
                case TOK_MINUS: return val_float(lv - rv);
                case TOK_STAR: return val_float(lv * rv);
                case TOK_SLASH: if (rv == 0) error(expr->line, "División por cero"); return val_float(lv / rv);
                case TOK_PERCENT: if (rv == 0) error(expr->line, "Módulo por cero"); return val_float((int)lv % (int)rv);
                case TOK_EEQ: return val_bool(lv == rv);
                case TOK_NEQ: return val_bool(lv != rv);
                case TOK_LT: return val_bool(lv < rv);
                case TOK_GT: return val_bool(lv > rv);
                case TOK_LE: return val_bool(lv <= rv);
                case TOK_GE: return val_bool(lv >= rv);
                default: error(expr->line, "Operador no soportado");
            }
            break;
        }
                case NODE_CALL: {
                    ASTNode *func = func_lookup(expr->data.call.name);
                    if (!func) error(expr->line, "Función no definida: %s", expr->data.call.name);
                    if (func == (ASTNode*)1) {
                        const char *name = expr->data.call.name;
                        if (strcmp(name, "exit") == 0) {
                            int code = 0;
                            if (expr->data.call.argc > 0) {
                                Value v = eval_expr(expr->data.call.args[0]);
                                code = (v.type == VAL_INT) ? v.data.ival : 0;
                            }
                            exit(code);
                        }
                        if (strcmp(name, "setlooplimit") == 0) {
                            if (expr->data.call.argc < 1) error(expr->line, "setlooplimit requiere un argumento");
                            Value v = eval_expr(expr->data.call.args[0]);
                            if (v.type == VAL_INT) max_loop_iterations = v.data.ival;
                            else error(expr->line, "setlooplimit espera un entero");
                            return val_make_null();
                        }
                        if (strcmp(name, "getlooplimit") == 0) return val_int(max_loop_iterations);
                        if (strcmp(name, "vartype") == 0) {
                            if (expr->data.call.argc < 1) error(expr->line, "vartype requiere un argumento");
                            ASTNode *arg = expr->data.call.args[0];
                            const char *type_str = NULL;
                            if (arg->kind == NODE_VAR) {
                                VarEntry *e = scope_find(current_scope, arg->data.var.name);
                                if (e) {
                                    switch (e->vtype) {
                                        case TOK_INT: type_str = "int"; break;
                                        case TOK_FLOAT: type_str = "float"; break;
                                        case TOK_BOOL: type_str = "bool"; break;
                                        case TOK_STRING: type_str = "string"; break;
                                        case TOK_LIST: type_str = "list"; break;
                                        default: type_str = "unknown"; break;
                                    }
                                } else error(expr->line, "Variable no definida '%s'", arg->data.var.name);
                            } else {
                                Value v = eval_expr(arg);
                                switch (v.type) {
                                    case VAL_INT: type_str = "int"; break;
                                    case VAL_FLOAT: type_str = "float"; break;
                                    case VAL_BOOL: type_str = "bool"; break;
                                    case VAL_STRING: type_str = "string"; break;
                                    case VAL_LIST: type_str = "list"; break;
                                    default: type_str = "null"; break;
                                }
                            }
                            printf("%s\n", type_str);
                            return val_make_null();
                        }
                        const char *color = "";
                        if (strcmp(name, "warn") == 0) color = "\033[33m";
                        else if (strcmp(name, "error") == 0) color = "\033[31m";
                        else if (strcmp(name, "success") == 0) color = "\033[32m";
                        printf("%s", color);
                        for (int i = 0; i < expr->data.call.argc; i++) {
                            Value v = eval_expr(expr->data.call.args[i]);
                            switch (v.type) {
                                case VAL_INT: printf("%d", v.data.ival); break;
                                case VAL_FLOAT: printf("%g", v.data.fval); break;
                                case VAL_BOOL: printf("%s", v.data.bval ? "true" : "false"); break;
                                case VAL_STRING: printf("%s", v.data.sval); break;
                                default: printf("[?]");
                            }
                        }
                        printf("\033[0m\n");
                        return val_make_null();
                    }
                    Scope *new_scope = scope_new(current_scope);
                    Scope *prev_scope = current_scope; current_scope = new_scope;
                    for (int i = 0; i < func->data.func.param_count; i++) {
                        Value arg = i < expr->data.call.argc ? eval_expr(expr->data.call.args[i]) : val_make_null();
                        scope_define(new_scope, func->data.func.params[i], func->data.func.ptypes[i], arg);
                    }
                    int saved_cf = control_flow; Value saved_ret = return_value;
                    control_flow = CF_NONE;
                    exec_block(&func->data.func.body);
                    Value ret = (control_flow == CF_RETURN) ? return_value : val_make_null();
                    control_flow = saved_cf; return_value = saved_ret;
                    current_scope = prev_scope;
                    return ret;
                }
                                case NODE_INDEX: {
                                    Value list = eval_expr(expr->data.idx.list);
                                    Value idx = eval_expr(expr->data.idx.index);
                                    if (list.type != VAL_LIST) error(expr->line, "No es una lista");
                                    int i = (idx.type == VAL_INT) ? idx.data.ival : 0;
                                    if (i < 0 || i >= list.data.list.count) error(expr->line, "Índice fuera de rango");
                                    return list.data.list.items[i];
                                }
                                default: error(expr->line, "Expresión no implementada");
    }
    return val_make_null();
}

int main(int argc, char **argv) {
    if (argc < 2) { fprintf(stderr, "Uso: infernal <script.infernal>\n"); return 1; }
    FILE *fp = fopen(argv[1], "r");
    if (!fp) { perror("Error al abrir script"); return 1; }

    global_scope = scope_new(NULL); current_scope = global_scope;
    func_register("print", (ASTNode*)1);
    func_register("warn", (ASTNode*)1);
    func_register("error", (ASTNode*)1);
    func_register("success", (ASTNode*)1);
    func_register("exit", (ASTNode*)1);
    func_register("setlooplimit", (ASTNode*)1);
    func_register("getlooplimit", (ASTNode*)1);
    func_register("vartype", (ASTNode*)1);

    if (!setjmp(exception_env)) {
        ts_init();
        tokenize_file(fp);
        fclose(fp);

        NodeList program = parse_block(NULL);
        exec_block(&program);
        return 0;
    } else {
        fprintf(stderr, "Error: %s\n", exception_msg);
        return 1;
    }
}
