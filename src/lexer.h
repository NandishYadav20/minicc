#ifndef MINICC_LEXER_H
#define MINICC_LEXER_H

typedef enum {
    TK_EOF,
    TK_NUM,
    TK_IDENT,

    /* keywords */
    TK_INT,
    TK_RETURN,
    TK_IF,
    TK_ELSE,
    TK_WHILE,

    /* operators and punctuation */
    TK_PLUS,
    TK_MINUS,
    TK_STAR,
    TK_SLASH,
    TK_PERCENT,
    TK_LPAREN,
    TK_RPAREN,
    TK_LBRACE,
    TK_RBRACE,
    TK_SEMI,
    TK_ASSIGN,
    TK_EQ,
    TK_NE,
    TK_LT,
    TK_LE,
    TK_GT,
    TK_GE,
    TK_BANG,
} TokenKind;

typedef struct {
    TokenKind kind;
    int line;
    long value; /* TK_NUM */
    char *name; /* TK_IDENT */
} Token;

typedef struct {
    Token *items;
    int count;
    int capacity;
} TokenList;

/* Split source text into tokens. The last token is always TK_EOF. */
TokenList lex(const char *src);

#endif
