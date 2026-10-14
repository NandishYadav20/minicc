#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "codegen.h"
#include "lexer.h"
#include "parser.h"
#include "util.h"

static void usage(void) {
    fprintf(stderr, "usage: minicc [--keep-asm] input.c [-o output]\n");
    exit(2);
}

static void run(const char *cmd) {
    if (system(cmd) != 0) die("command failed: %s", cmd);
}

int main(int argc, char **argv) {
    const char *input = NULL;
    const char *output = "a.out";
    int keep_asm = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0) {
            if (++i >= argc) usage();
            output = argv[i];
        } else if (strcmp(argv[i], "--keep-asm") == 0) {
            keep_asm = 1;
        } else if (argv[i][0] == '-') {
            usage();
        } else {
            input = argv[i];
        }
    }
    if (!input) usage();

    char *source = read_file(input);
    TokenList tokens = lex(source);
    Function fn = parse(&tokens);

    char asm_path[4096];
    char obj_path[4096];
    char cmd[12288];
    snprintf(asm_path, sizeof asm_path, "%s.s", output);
    snprintf(obj_path, sizeof obj_path, "%s.o", output);

    FILE *f = fopen(asm_path, "w");
    if (!f) die("cannot write '%s'", asm_path);
    codegen(&fn, f);
    fclose(f);

    /* Assemble, then link into a static executable. */
    snprintf(cmd, sizeof cmd, "as -o \"%s\" \"%s\"", obj_path, asm_path);
    run(cmd);
    snprintf(cmd, sizeof cmd, "ld -o \"%s\" \"%s\"", output, obj_path);
    run(cmd);

    remove(obj_path);
    if (!keep_asm) remove(asm_path);
    return 0;
}
