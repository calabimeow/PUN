#ifndef PUN_H
#define PUN_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

extern FILE *output_file;

typedef enum
{
    TOKEN_IDENT,
    TOKEN_NUM,
    TOKEN_PUNCT,
    TOKEN_EOF
}token_kind;

typedef struct
{
    token_kind kind;
    char *loc;
    int len;
    int val;
}token;

typedef struct
{
    token *tokens;
    size_t size;
    size_t capacity;
}token_arr;

bool token_equal(token t, char *c);
token token_peek(token_arr *ta, size_t pos);
token_arr tokenize_file(const char *path);

typedef enum
{
    NODE_ASSIGN,
    NODE_ADD,
    NODE_SUB,
    NODE_MUL,
    NODE_DIV,
    NODE_NUM,
    NODE_VAR,
    NODE_RETURN,
    NODE_IF,
    NODE_ELSE,
    NODE_EQ,
    NODE_NE,
    NODE_LT,
    NODE_LE,
    NODE_FOR,
    NODE_BLOCK,
    NODE_PRINT,
    NODE_JUMP,
    NODE_LABEL,
    NODE_FUNC,
    NODE_FUNCALL
}node_kind;

typedef struct
{
    char *name;
    int offset;
}symbol;

typedef struct scope scope;
struct scope
{
    symbol **symbols;
    scope *parent;
    size_t size;
    size_t capacity;
};

typedef struct node_arr node_arr;
typedef struct node node;
struct node
{
    node_kind kind;
    int val;
    node *lhs;
    node *rhs;
    node *cond;
    node *then;
    node *els;
    node *init;
    node *inc;
    node_arr *body;
    symbol *var;
    char *label;
    node *func;
    char *funcname;
    node_arr *args;
    node_arr *params;
};

struct node_arr
{
    node *nodes;
    size_t size;
    size_t capacity;
};

node_arr parse(token_arr *ta);
void codegen(node_arr *na);

#endif
