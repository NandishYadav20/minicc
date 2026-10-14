#ifndef MINICC_UTIL_H
#define MINICC_UTIL_H

#include <stddef.h>

/* Print an error message and exit with status 1. */
void die(const char *fmt, ...);
void die_at(int line, const char *fmt, ...);

/* Allocation helpers that exit on failure. */
void *xmalloc(size_t size);
void *xcalloc(size_t count, size_t size);
void *xrealloc(void *ptr, size_t size);
char *xstrndup(const char *s, size_t n);

/* Read a whole file into a NUL-terminated buffer. */
char *read_file(const char *path);

#endif
