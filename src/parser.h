#ifndef MINICC_PARSER_H
#define MINICC_PARSER_H

#include "ast.h"
#include "lexer.h"

/* Parse `int main() { ... }` into a Function. Exits with an error on bad input. */
Function parse(const TokenList *tokens);

#endif
