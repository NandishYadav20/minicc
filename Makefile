CC     ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -g -O2

SRC := src/main.c src/lexer.c src/trie.c src/parser.c src/codegen.c src/util.c
OBJ := $(SRC:src/%.c=build/%.o)
BIN := bin/minicc

all: $(BIN)

$(BIN): $(OBJ) | bin
	$(CC) $(CFLAGS) -o $@ $(OBJ)

build/%.o: src/%.c | build
	$(CC) $(CFLAGS) -Isrc -c $< -o $@

build bin:
	mkdir -p $@

test: $(BIN)
	./tests/run_tests.sh

clean:
	rm -rf build bin

.PHONY: all test clean
