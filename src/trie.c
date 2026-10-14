#include <stddef.h>
#include "trie.h"
#include "util.h"

#define ALPHABET 128

struct Trie {
    Trie *next[ALPHABET];
    int has_value;
    int value;
};

Trie *trie_new(void) {
    return xcalloc(1, sizeof(Trie));
}

void trie_insert(Trie *t, const char *key, int value) {
    for (const char *p = key; *p; p++) {
        unsigned char c = (unsigned char)*p;
        if (c >= ALPHABET) die("trie: non-ASCII key");
        if (!t->next[c]) t->next[c] = trie_new();
        t = t->next[c];
    }
    t->has_value = 1;
    t->value = value;
}

int trie_lookup(const Trie *t, const char *s, size_t len, int fallback) {
    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)s[i];
        if (c >= ALPHABET || !t->next[c]) return fallback;
        t = t->next[c];
    }
    return t->has_value ? t->value : fallback;
}
