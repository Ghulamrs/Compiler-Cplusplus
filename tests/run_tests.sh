#!/bin/sh
#
# Regression tests for Compiler++.
#
#   ./run_tests.sh <path-to-Compiler++-binary>
#   ./run_tests.sh <binary> --accept      re-record every expected file
#
# Each cases/NAME.cpp is run through the compiler and its combined output is
# compared with expected/NAME.txt.  The name also states the intent, and the
# script checks the exit status agrees:
#
#     *err_*    must be REJECTED   (exit 1)
#     anything else must be ACCEPTED (exit 0) -- including a "warn_" case,
#               which is expected to produce warnings but still compile
#
# Xcode is not needed -- any C++98 compiler builds the binary, and this script
# only runs it.

BIN="$1"
ACCEPT="$2"

if [ -z "$BIN" ] || [ ! -x "$BIN" ]; then
    echo "usage: $0 <path-to-Compiler++-binary> [--accept]" >&2
    exit 2
fi

DIR=$(dirname "$0")
cd "$DIR" || exit 2
mkdir -p expected

pass=0
fail=0

for case_file in cases/*.cpp; do
    name=$(basename "$case_file" .cpp)
    expected="expected/$name.txt"
    actual=$("$BIN" -ast -layout -ir -q "$case_file" 2>&1)
    status=$?

    # A case named *err_* must be rejected; every other case must be accepted.
    case "$name" in
        *err_*) want_status=1 ;;
        *)      want_status=0 ;;
    esac

    if [ "$ACCEPT" = "--accept" ]; then
        printf '%s\n' "$actual" > "$expected"
        echo "recorded $name"
        continue
    fi

    if [ ! -f "$expected" ]; then
        echo "MISSING  $name  (run with --accept to record)"
        fail=$((fail + 1))
        continue
    fi

    if [ "$status" -ne "$want_status" ]; then
        echo "FAIL     $name  (exit $status, wanted $want_status)"
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
    if printf '%s\n' "$actual" | diff -u --strip-trailing-cr "$expected" - > /tmp/ccpp_diff.$$ 2>&1; then
        echo "ok       $name"
        pass=$((pass + 1))
    else
        echo "FAIL     $name  (output changed)"
        sed -n '1,20p' /tmp/ccpp_diff.$$
        fail=$((fail + 1))
    fi
    rm -f /tmp/ccpp_diff.$$
done

echo
echo "$pass passed, $fail failed"
[ "$fail" -eq 0 ]
