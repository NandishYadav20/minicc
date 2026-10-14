#!/usr/bin/env bash
# Compiles every program in tests/cases and checks its exit status against the
# "// expect: N" comment, then checks that every program in tests/errors is rejected.
set -u
cd "$(dirname "$0")/.."

BIN=./bin/minicc
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

pass=0
fail=0

for src in tests/cases/*.c; do
    name=$(basename "$src" .c)
    expected=$(sed -n 's#^.*// expect: *\([0-9][0-9]*\)$#\1#p' "$src" | head -n1)
    if ! "$BIN" "$src" -o "$TMP/$name" 2>"$TMP/$name.err"; then
        echo "FAIL $name: compile error: $(cat "$TMP/$name.err")"
        fail=$((fail + 1))
        continue
    fi
    "$TMP/$name" >/dev/null
    actual=$?
    if [ "$actual" -eq "$expected" ]; then
        pass=$((pass + 1))
    else
        echo "FAIL $name: expected exit status $expected, got $actual"
        fail=$((fail + 1))
    fi
done

for src in tests/errors/*.c; do
    name=$(basename "$src" .c)
    if "$BIN" "$src" -o "$TMP/$name" 2>/dev/null; then
        echo "FAIL $name: should have been rejected"
        fail=$((fail + 1))
    else
        pass=$((pass + 1))
    fi
done

echo "passed $pass, failed $fail"
[ "$fail" -eq 0 ]
