#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "parser.h"
#include "util.h"

/*
 * Recursive-descent parser. Expressions use a Pratt parser (precedence
 * climbing): each binary operator has a binding power, and parse_expr keeps
 * consuming operators that bind at least as tightly as `min_prec`.
 */

typedef struct {
    TokenList toks;
    int pos;
    Locals locals;
} Parser;

static const Token *peek(Parser *p) {
    return &p->toks.items[p->pos];
}

static const Token *peek_ahead(Parser *p, int n) {
    int i = p->pos + n;
    if (i >= p->toks.count) i = p->toks.count - 1; /* the last token is EOF */
    return &p->toks.items[i];
}

static const Token *advance(Parser *p) {
    const Token *t = peek(p);
    if (t->kind != TK_EOF) p->pos++;
    return t;
}

static int accept(Parser *p, TokenKind kind) {
    if (peek(p)->kind != kind) return 0;
    advance(p);
    return 1;
}

static const Token *expect(Parser *p, TokenKind kind, const char *what) {
    const Token *t = peek(p);
    if (t->kind != kind) die_at(t->line, "expected %s", what);
    return advance(p);
}

static Node *new_node(NodeKind kind) {
    Node *n = xcalloc(1, sizeof(Node));
    n->kind = kind;
    return n;
}

static int find_local(const Parser *p, const char *name) {
    for (int i = 0; i < p->locals.count; i++) {
        if (strcmp(p->locals.names[i], name) == 0) return i;
    }
    return -1;
}

static int declare_local(Parser *p, const Token *name) {
    if (find_local(p, name->name) >= 0) {
        die_at(name->line, "redeclaration of '%s'", name->name);
    }
    if (p->locals.count == p->locals.capacity) {
        p->locals.capacity = p->locals.capacity ? p->locals.capacity * 2 : 8;
        p->locals.names = xrealloc(p->locals.names,
                                   (size_t)p->locals.capacity * sizeof(char *));
    }
    p->locals.names[p->locals.count] = name->name;
    return p->locals.count++;
}

static int resolve_local(const Parser *p, const Token *name) {
    int i = find_local(p, name->name);
    if (i < 0) die_at(name->line, "undefined variable '%s'", name->name);
    return i;
}

/* Binding power of a binary operator; 0 means "not a binary operator". */
static int binary_precedence(TokenKind kind) {
    switch (kind) {
    case TK_EQ:
    case TK_NE:
        return 1;
    case TK_LT:
    case TK_LE:
    case TK_GT:
    case TK_GE:
        return 2;
    case TK_PLUS:
    case TK_MINUS:
        return 3;
    case TK_STAR:
    case TK_SLASH:
    case TK_PERCENT:
        return 4;
    default:
        return 0;
    }
}

static Node *parse_assignment(Parser *p);
static Node *parse_stmt(Parser *p);

/* Primary expressions and prefix operators. */
static Node *parse_prefix(Parser *p) {
    const Token *t = peek(p);
    switch (t->kind) {
    case TK_NUM: {
        advance(p);
        Node *n = new_node(ND_NUM);
        n->value = t->value;
        return n;
    }
    case TK_IDENT: {
        advance(p);
        Node *n = new_node(ND_VAR);
        n->var = resolve_local(p, t);
        return n;
    }
    case TK_MINUS:
    case TK_BANG: {
        advance(p);
        Node *n = new_node(t->kind == TK_MINUS ? ND_NEG : ND_NOT);
        n->lhs = parse_prefix(p);
        return n;
    }
    case TK_LPAREN: {
        advance(p);
        Node *e = parse_assignment(p);
        expect(p, TK_RPAREN, "')'");
        return e;
    }
    default:
        die_at(t->line, "expected an expression");
    }
    return NULL; /* not reached */
}

/* Binary operators, using precedence climbing. */
static Node *parse_expr(Parser *p, int min_prec) {
    Node *lhs = parse_prefix(p);
    for (;;) {
        const Token *op = peek(p);
        int prec = binary_precedence(op->kind);
        if (prec == 0 || prec < min_prec) break;

        advance(p);
        /* Left-associative: the right side may only take tighter operators. */
        Node *rhs = parse_expr(p, prec + 1);

        Node *n = new_node(ND_BINARY);
        n->op = op->kind;
        n->lhs = lhs;
        n->rhs = rhs;
        lhs = n;
    }
    return lhs;
}

/* Assignment is lowest precedence and right-associative: a = b = 1. */
static Node *parse_assignment(Parser *p) {
    const Token *t = peek(p);
    if (t->kind == TK_IDENT && peek_ahead(p, 1)->kind == TK_ASSIGN) {
        advance(p);
        advance(p);
        Node *n = new_node(ND_ASSIGN);
        n->var = resolve_local(p, t);
        n->lhs = parse_assignment(p);
        return n;
    }
    return parse_expr(p, 1);
}

static Node *parse_block(Parser *p) {
    expect(p, TK_LBRACE, "'{'");
    Node *block = new_node(ND_BLOCK);
    int capacity = 0;
    while (peek(p)->kind != TK_RBRACE) {
        if (peek(p)->kind == TK_EOF) {
            die_at(peek(p)->line, "missing '}' before end of file");
        }
        if (block->nitems == capacity) {
            capacity = capacity ? capacity * 2 : 8;
            block->items = xrealloc(block->items, (size_t)capacity * sizeof(Node *));
        }
        block->items[block->nitems++] = parse_stmt(p);
    }
    advance(p);
    return block;
}

static Node *parse_stmt(Parser *p) {
    const Token *t = peek(p);
    switch (t->kind) {
    case TK_INT: {
        advance(p);
        const Token *name = expect(p, TK_IDENT, "variable name");
        Node *n = new_node(ND_DECL);
        n->var = declare_local(p, name);
        if (accept(p, TK_ASSIGN)) n->lhs = parse_assignment(p);
        expect(p, TK_SEMI, "';'");
        return n;
    }
    case TK_RETURN: {
        advance(p);
        Node *n = new_node(ND_RETURN);
        n->lhs = parse_assignment(p);
        expect(p, TK_SEMI, "';'");
        return n;
    }
    case TK_IF: {
        advance(p);
        Node *n = new_node(ND_IF);
        expect(p, TK_LPAREN, "'('");
        n->cond = parse_assignment(p);
        expect(p, TK_RPAREN, "')'");
        n->then = parse_stmt(p);
        if (accept(p, TK_ELSE)) n->els = parse_stmt(p);
        return n;
    }
    case TK_WHILE: {
        advance(p);
        Node *n = new_node(ND_WHILE);
        expect(p, TK_LPAREN, "'('");
        n->cond = parse_assignment(p);
        expect(p, TK_RPAREN, "')'");
        n->then = parse_stmt(p);
        return n;
    }
    case TK_LBRACE:
        return parse_block(p);
    default: {
        Node *n = new_node(ND_EXPR_STMT);
        n->lhs = parse_assignment(p);
        expect(p, TK_SEMI, "';'");
        return n;
    }
    }
}

Function parse(const TokenList *tokens) {
    Parser p = {0};
    p.toks = *tokens;

    expect(&p, TK_INT, "'int' before main");
    const Token *name = expect(&p, TK_IDENT, "'main'");
    if (strcmp(name->name, "main") != 0) {
        die_at(name->line, "only a function named 'main' is supported");
    }
    expect(&p, TK_LPAREN, "'('");
    expect(&p, TK_RPAREN, "')'");

    Function fn = {0};
    fn.body = parse_block(&p);
    if (peek(&p)->kind != TK_EOF) {
        die_at(peek(&p)->line, "unexpected code after the end of main");
    }
    fn.locals = p.locals;
    return fn;
}
