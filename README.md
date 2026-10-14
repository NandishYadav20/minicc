# minicc

A small C compiler written in C. It reads a subset of C, builds an abstract syntax tree, and emits x86-64 GNU assembly. The system assembler and linker turn that assembly into an executable.

I wrote it from scratch to learn how each stage of a compiler works: lexing, parsing, keyword lookup, and code generation. It was inspired by a YouTube video series on building a C compiler, but the code is my own.

## Supported language

- A single function: `int main() { ... }`
- Declarations with optional initializers: `int x;`, `int x = expr;`
- Assignment, including chains: `a = b = 1;`
- `return`, `if` / `else`, `while`, blocks `{ ... }`, and expression statements
- Integer literals, variables, `+ - * / %`, comparisons `== != < <= > >=`, unary `-` and `!`, and parentheses
- Operator precedence and left-associativity follow C

Not supported yet: functions other than `main`, function calls, pointers, arrays, strings, types other than `int`, block-scoped variables (all locals share one function scope), and loops like `for`. All values are 64-bit. The program's exit status is `main`'s return value, which Linux keeps modulo 256.

## Requirements

- Linux on x86-64
- `gcc` (or any C11 compiler), `make`
- GNU `as` and `ld` (from binutils)

On Windows, use WSL. The compiler emits Linux x86-64 code, so you can't build and run it natively on Windows.

## Build and run

```sh
make                       # builds bin/minicc
bin/minicc prog.c -o prog  # compile and link
./prog; echo $?            # run it and print the exit status
```

Add `--keep-asm` to keep the generated `prog.s` for inspection.

Example:

```c
int main() {
  int a = 6;
  int b = 7;
  return a * b;   /* exit status 42 */
}
```

## Tests

```sh
make test
```

`tests/cases/*.c` are compiled and run. Each file has a `// expect: N` comment, and the test passes if the exit status is `N`. `tests/errors/*.c` must be rejected by the compiler.

## How it works

| Stage | File | Job |
|---|---|---|
| Lexer | `src/lexer.c` | Turns source text into tokens. Identifiers are checked against a keyword trie. |
| Keyword trie | `src/trie.c` | Maps keyword strings to token kinds, one character per step. |
| Parser | `src/parser.c` | Recursive descent for statements, plus a Pratt parser for expressions. Builds the AST and resolves variables. |
| AST | `src/ast.h` | Node types and the function/locals representation. |
| Code generator | `src/codegen.c` | Emits AT&T-syntax assembly with a stack-machine strategy. |
| Driver | `src/main.c` | Ties the stages together, then runs `as` and `ld`. |

### Design notes

- **Keyword trie.** Keywords are stored in a trie, so classifying an identifier takes one walk over its characters. Punctuation is matched with a small table.
- **Pratt parsing for expressions.** Each binary operator has a binding power, and `parse_expr` keeps consuming operators that bind at least as tightly as the current minimum. Assignment is handled above this, because it is right-associative and has the lowest precedence.
- **Variables resolved at parse time.** Undefined and duplicate variables are reported as errors before code generation, and the stack-frame size is known as soon as parsing finishes.
- **Stack-machine code generation.** Every expression leaves its result in `%rax`. A binary operator pushes its left operand, evaluates the right one, then pops the left into `%rax`. This is simple to reason about, though it isn't fast.
