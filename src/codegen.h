#ifndef MINICC_CODEGEN_H
#define MINICC_CODEGEN_H

#include <stdio.h>
#include "ast.h"

/* Write x86-64 GNU assembly for `fn` (AT&T syntax) to `out`. */
void codegen(const Function *fn, FILE *out);

#endif
