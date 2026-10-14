#ifndef MINICC_AST_H
#define MINICC_AST_H

#include "lexer.h"

typedef enum {
    ND_NUM,       /* integer literal: value */
    ND_VAR,       /* variable read: var */
    ND_ASSIGN,    /* var = lhs */
    ND_NEG,       /* -lhs */
    ND_NOT,       /* !lhs */
    ND_BINARY,    /* lhs op rhs */
    ND_DECL,      /* int var (= lhs)? */
    ND_RETURN,    /* return lhs; */
    ND_EXPR_STMT, /* lhs; */
    ND_IF,        /* if (cond) then (else els)? */
    ND_WHILE,     /* while (cond) then */
    ND_BLOCK,     /* { items } */
} NodeKind;

typedef struct Node Node;
struct Node {
    NodeKind kind;
    TokenKind op; /* ND_BINARY */
    long value;   /* ND_NUM */
    int var;      /* ND_VAR, ND_ASSIGN, ND_DECL: index into Locals */
    Node *lhs;
    Node *rhs;
    Node *cond;
    Node *then;
    Node *els;
    Node **items; /* ND_BLOCK */
    int nitems;
};

/* Every local variable of main, in declaration order. */
typedef struct {
    char **names;
    int count;
    int capacity;
} Locals;

typedef struct {
    Node *body; /* ND_BLOCK */
    Locals locals;
} Function;

#endif
