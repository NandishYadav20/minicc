#ifndef MINICC_TRIE_H
#define MINICC_TRIE_H

#include <stddef.h>

/*
 * A trie over ASCII strings. Each node has one child slot per character,
 * so a lookup walks one node per character of the input.
 */
typedef struct Trie Trie;

Trie *trie_new(void);

/* Associate `key` with `value`. */
void trie_insert(Trie *t, const char *key, int value);

/* Look up the first `len` characters of `s`. Returns `fallback` if not found. */
int trie_lookup(const Trie *t, const char *s, size_t len, int fallback);

#endif
