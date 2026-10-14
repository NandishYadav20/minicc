#include <stdio.h>
#include "codegen.h"
#include "util.h"

/*
 * Stack-machine code generation. Every expression leaves its value in %rax.
 * Binary operators push the left value, evaluate the right side, then pop
 * the left value into %rax and combine. Each local is an 8-byte slot below
 * the frame pointer.
 */

static FILE *out;
static int label_count;

static int var_offset(int index) {
    return -(index + 1) * 8;
}

static void gen_stmt(const Node *n);

/* %rax = (%rax <cmp> %rcx), as 0 or 1. */
static void compare(const char *setcc) {
    fprintf(out, "  cmp %%rcx, %%rax\n  %s %%al\n  movzbq %%al, %%rax\n", setcc);
}

static void gen_binary(TokenKind op) {
    switch (op) {
    case TK_PLUS:    fprintf(out, "  add %%rcx, %%rax\n"); return;
    case TK_MINUS:   fprintf(out, "  sub %%rcx, %%rax\n"); return;
    case TK_STAR:    fprintf(out, "  imul %%rcx, %%rax\n"); return;
    case TK_SLASH:   fprintf(out, "  cqo\n  idiv %%rcx\n"); return;
    case TK_PERCENT: fprintf(out, "  cqo\n  idiv %%rcx\n  mov %%rdx, %%rax\n"); return;
    case TK_EQ:      compare("sete"); return;
    case TK_NE:      compare("setne"); return;
    case TK_LT:      compare("setl"); return;
    case TK_LE:      compare("setle"); return;
    case TK_GT:      compare("setg"); return;
    case TK_GE:      compare("setge"); return;
    default:         die("internal error: unknown binary operator");
    }
}

static void gen_expr(const Node *n) {
    switch (n->kind) {
    case ND_NUM:
        fprintf(out, "  mov $%ld, %%rax\n", n->value);
        return;
    case ND_VAR:
        fprintf(out, "  mov %d(%%rbp), %%rax\n", var_offset(n->var));
        return;
    case ND_ASSIGN:
        gen_expr(n->lhs);
        fprintf(out, "  mov %%rax, %d(%%rbp)\n", var_offset(n->var));
        return;
    case ND_NEG:
        gen_expr(n->lhs);
        fprintf(out, "  neg %%rax\n");
        return;
    case ND_NOT:
        gen_expr(n->lhs);
        fprintf(out, "  cmp $0, %%rax\n  sete %%al\n  movzbq %%al, %%rax\n");
        return;
    case ND_BINARY:
        gen_expr(n->lhs);
        fprintf(out, "  push %%rax\n");
        gen_expr(n->rhs);
        fprintf(out, "  mov %%rax, %%rcx\n  pop %%rax\n");
        gen_binary(n->op);
        return;
    default:
        die("internal error: unknown expression node");
    }
}

static void gen_stmt(const Node *n) {
    switch (n->kind) {
    case ND_DECL:
        if (n->lhs) {
            gen_expr(n->lhs);
        } else {
            fprintf(out, "  mov $0, %%rax\n"); /* uninitialized locals read as 0 */
        }
        fprintf(out, "  mov %%rax, %d(%%rbp)\n", var_offset(n->var));
        return;
    case ND_EXPR_STMT:
        gen_expr(n->lhs);
        return;
    case ND_RETURN:
        gen_expr(n->lhs);
        fprintf(out, "  jmp .Lreturn\n");
        return;
    case ND_BLOCK:
        for (int i = 0; i < n->nitems; i++) gen_stmt(n->items[i]);
        return;
    case ND_IF: {
        int id = label_count++;
        gen_expr(n->cond);
        fprintf(out, "  cmp $0, %%rax\n  je .Lelse%d\n", id);
        gen_stmt(n->then);
        fprintf(out, "  jmp .Lend%d\n.Lelse%d:\n", id, id);
        if (n->els) gen_stmt(n->els);
        fprintf(out, ".Lend%d:\n", id);
        return;
    }
    case ND_WHILE: {
        int id = label_count++;
        fprintf(out, ".Lbegin%d:\n", id);
        gen_expr(n->cond);
        fprintf(out, "  cmp $0, %%rax\n  je .Lend%d\n", id);
        gen_stmt(n->then);
        fprintf(out, "  jmp .Lbegin%d\n.Lend%d:\n", id, id);
        return;
    }
    default:
        die("internal error: unknown statement node");
    }
}

void codegen(const Function *fn, FILE *file) {
    out = file;

    /* Reserve 8 bytes per local, rounded up to keep the stack 16-byte aligned. */
    int frame = fn->locals.count * 8;
    frame = (frame + 15) / 16 * 16;

    fprintf(out, "  .text\n");

    /* Entry point: call main, then exit with its return value as the status. */
    fprintf(out, "  .globl _start\n_start:\n");
    fprintf(out, "  call main\n");
    fprintf(out, "  mov %%eax, %%edi\n");
    fprintf(out, "  mov $60, %%eax\n"); /* sys_exit */
    fprintf(out, "  syscall\n\n");

    fprintf(out, "  .globl main\nmain:\n");
    fprintf(out, "  push %%rbp\n  mov %%rsp, %%rbp\n");
    if (frame) fprintf(out, "  sub $%d, %%rsp\n", frame);

    gen_stmt(fn->body);

    fprintf(out, "  mov $0, %%eax\n"); /* falling off the end of main returns 0 */
    fprintf(out, ".Lreturn:\n");
    fprintf(out, "  mov %%rbp, %%rsp\n  pop %%rbp\n  ret\n");
}
