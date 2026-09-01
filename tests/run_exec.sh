#!/bin/sh
#
# Runtime tests: what does the program actually PRINT?
#
#   ./run_exec.sh <compiler> [--accept]
#
# run_tests.sh only compiles.  A compiler can emit the wrong opcode, agree with
# itself through a .cxb, and pass both other suites -- which is exactly what
# happened.  This suite is the one that looks at the answer.
#
# Every cases/*run_*.cpp is executed and its stdout compared with
# expected_run/NAME.txt.  Those files are hand-checked, not merely recorded:
# --accept is for reviewing a diff, never for blessing an unexamined result.

BIN="$1"
ACCEPT="$2"

if [ -z "$BIN" ] || [ ! -x "$BIN" ]; then
    echo "usage: $0 <path-to-Compiler++-binary> [--accept]" >&2
    exit 2
fi

DIR=$(dirname "$0")
cd "$DIR" || exit 2
mkdir -p expected_run

pass=0
fail=0

for case_file in cases/*.cpp; do
    name=$(basename "$case_file" .cpp)
    case "$name" in *run_*) ;; *) continue ;; esac

    expected="expected_run/$name.txt"
    # A case that reads cin gets its input from input/NAME.txt.  Everything
    # else gets /dev/null -- never the terminal, or a case that reads would
    # sit waiting for a suite nobody is watching.
    input=/dev/null
    [ -f "input/$name.txt" ] && input="input/$name.txt"
    actual=$("$BIN" -run -q "$case_file" < "$input" 2>&1)
    status=$?

    if [ "$ACCEPT" = "--accept" ]; then
        printf '%s\n' "$actual" > "$expected"
        echo "recorded $name"
        continue
    fi

    if [ "$status" -ne 0 ]; then
        echo "FAIL     $name  (exit $status)"
        printf '%s\n' "$actual" | sed -n '1,5p'
        fail=$((fail + 1))
        continue
    fi

    if [ ! -f "$expected" ]; then
        echo "MISSING  $name  (run with --accept, then CHECK the file)"
        fail=$((fail + 1))
        continue
    fi

    # --strip-trailing-cr because the golden files are LF and MSVC's stdout is
    # not: a text-mode C++ stream translates '\n' on Windows, so without this
    # every case on that box reports as a failure while the compiler is
    # entirely correct. It is the third box's whole suite, bought with a flag
    # both BSD and GNU diff have had for years. Nothing in expected/ or
    # expected_run/ contains a carriage return of its own, so there is nothing
    # here for it to hide.
    if printf '%s\n' "$actual" | diff -u --strip-trailing-cr "$expected" - > /tmp/exec_diff.$$ 2>&1; then
        echo "ok       $name"
        pass=$((pass + 1))
    else
        echo "FAIL     $name  (wrong output)"
        sed -n '1,20p' /tmp/exec_diff.$$
        fail=$((fail + 1))
    fi
    rm -f /tmp/exec_diff.$$
done

echo
echo "$pass passed, $fail failed"
[ "$fail" -eq 0 ]
