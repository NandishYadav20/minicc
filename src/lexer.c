#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "trie.h"
#include "util.h"

/* Keywords are stored in a trie so identifiers are classified in one walk. */
static Trie *keyword_table(void) {
    static Trie *table;
    if (!table) {
        table = trie_new();
        trie_insert(table, "int", TK_INT);
        trie_insert(table, "return", TK_RETURN);
        trie_insert(table, "if", TK_IF);
        trie_insert(table, "else", TK_ELSE);
        trie_insert(table, "while", TK_WHILE);
    }
    return table;
}

static void push(TokenList *list, Token tok) {
    if (list->count == list->capacity) {
        list->capacity = list->capacity ? list->capacity * 2 : 64;
        list->items = xrealloc(list->items, (size_t)list->capacity * sizeof(Token));
    }
    list->items[list->count++] = tok;
}

/* Read an operator at *cursor, advancing it past the operator. */
static TokenKind read_operator(const char **cursor, int line) {
    static const struct { const char *text; TokenKind kind; } two_char[] = {
        {"==", TK_EQ}, {"!=", TK_NE}, {"<=", TK_LE}, {">=", TK_GE},
    };
    static const struct { char ch; TokenKind kind; } one_char[] = {
        {'+', TK_PLUS},   {'-', TK_MINUS},  {'*', TK_STAR},   {'/', TK_SLASH},
        {'%', TK_PERCENT}, {'(', TK_LPAREN}, {')', TK_RPAREN}, {'{', TK_LBRACE},
        {'}', TK_RBRACE}, {';', TK_SEMI},   {'=', TK_ASSIGN}, {'<', TK_LT},
        {'>', TK_GT},     {'!', TK_BANG},
    };

    const char *p = *cursor;
    for (size_t i = 0; i < sizeof two_char / sizeof two_char[0]; i++) {
        if (strncmp(p, two_char[i].text, 2) == 0) {
            *cursor = p + 2;
            return two_char[i].kind;
        }
    }
    for (size_t i = 0; i < sizeof one_char / sizeof one_char[0]; i++) {
        if (*p == one_char[i].ch) {
            *cursor = p + 1;
            return one_char[i].kind;
        }
    }
    die_at(line, "unexpected character '%c'", *p);
    return TK_EOF; /* not reached */
}

TokenList lex(const char *src) {
    TokenList list = {0};
    const char *p = src;
    int line = 1;

    for (;;) {
        /* Skip whitespace and // comments. */
        for (;;) {
            if (*p == '\n') {
                line++;
                p++;
            } else if (isspace((unsigned char)*p)) {
                p++;
            } else if (p[0] == '/' && p[1] == '/') {
                while (*p && *p != '\n') p++;
            } else {
                break;
            }
        }

        Token tok = {0};
        tok.line = line;

        if (*p == '\0') {
            tok.kind = TK_EOF;
            push(&list, tok);
            return list;
        }

        if (isdigit((unsigned char)*p)) {
            char *end;
            tok.kind = TK_NUM;
            tok.value = strtol(p, &end, 10);
            p = end;
        } else if (isalpha((unsigned char)*p) || *p == '_') {
            const char *start = p;
            while (isalnum((unsigned char)*p) || *p == '_') p++;
            size_t len = (size_t)(p - start);
            tok.kind = (TokenKind)trie_lookup(keyword_table(), start, len, TK_IDENT);
            if (tok.kind == TK_IDENT) tok.name = xstrndup(start, len);
        } else {
            tok.kind = read_operator(&p, line);
        }

        push(&list, tok);
    }
}
