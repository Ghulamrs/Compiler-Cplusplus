#!/bin/sh
# Differential suite: every run_ case is also a valid C++98 program once the
# natives it calls are supplied, so a real compiler can answer the same
# question and the two answers can be compared.  This is the suite that
# catches a wrong answer nothing was asserting.
#
#   ./run_differential.sh <compilerpp> [host-c++]
#
# tests/differential_shim.h defines print_int, print_line and the rest on top
# of <iostream>, so g++/clang++ can build a case unchanged.  Pointer output is
# normalised: the VM prints its own addresses and the host prints the host's.
#
# A case named in ALLOWED below differs for a reason that is understood and
# permitted, and is reported as ALLOW rather than counted as a failure.  The
# list is short on purpose: anything on it is a claim about the STANDARD, not
# an excuse.  A difference with no entry is a bug in this compiler.
BIN="$1"
HOSTCXX="${2:-c++}"
[ -x "$BIN" ] || { echo "usage: $0 <compilerpp> [host-c++]" >&2; exit 2; }
DIR=$(dirname "$0"); cd "$DIR" || exit 2
WORK=${TMPDIR:-/tmp}/cxxdiff.$$
mkdir -p "$WORK" || exit 2
trap 'rm -rf "$WORK"' EXIT

# Copy elision is PERMITTED, not required: the host elides the copy out of a
# returned local and this compiler does not, so a counted copy constructor runs
# once here and not there.  Both are conforming; neither is the wrong answer.
allowed_reason() {
    case "$1" in
        93_run_lifetime_balance)      echo "host elides the return copy" ;;
        110_run_return_by_value_copies) echo "host elides the return copy" ;;
        *) echo "" ;;
    esac
}

pass=0; fail=0; skip=0; allow=0
for case_file in cases/*run_*.cpp; do
    name=$(basename "$case_file" .cpp)
    input=/dev/null
    [ -f "input/$name.txt" ] && input="input/$name.txt"

    # The case's own #include lines name headers this compiler synthesises;
    # the shim supplies the same names from the real standard library.
    { cat differential_shim.h; grep -v '^[[:space:]]*#include' "$case_file"; } > "$WORK/$name.cpp"
    if ! "$HOSTCXX" -std=c++98 -w -o "$WORK/$name.bin" "$WORK/$name.cpp" 2> "$WORK/$name.err"; then
        echo "SKIP  $name  (host compiler rejected it)"
        skip=$((skip + 1))
        continue
    fi

    host=$("$WORK/$name.bin" < "$input" 2>/dev/null | sed 's/0x[0-9a-f]*/ADDR/g')
    mine=$("$BIN" -run -q "$case_file" < "$input" 2>/dev/null | sed 's/0x[0-9a-f]*/ADDR/g')

    if [ "$host" = "$mine" ]; then
        pass=$((pass + 1))
        continue
    fi

    reason=$(allowed_reason "$name")
    if [ -n "$reason" ]; then
        echo "ALLOW $name  ($reason)"
        allow=$((allow + 1))
        continue
    fi

    echo "DIFF  $name"
    printf '%s\n' "$host" > "$WORK/$name.host"
    printf '%s\n' "$mine" > "$WORK/$name.mine"
    diff -u "$WORK/$name.host" "$WORK/$name.mine" | sed -n '3,15p'
    fail=$((fail + 1))
done
echo
echo "$pass agreed, $fail differed, $allow allowed, $skip skipped"

# Nothing compared is not the same as nothing wrong.  Where there is no host
# compiler -- Windows, where cl is not GCC-shaped and takes different flags --
# every case skips and the suite would otherwise report success for a run that
# checked nothing.  Say so; a green line that means "did not run" is the kind
# of thing the other suites were already guilty of.
if [ "$pass" -eq 0 ] && [ "$skip" -gt 0 ]; then
    echo
    echo "NOTHING WAS COMPARED: no case built with '$HOSTCXX'."
    echo "This suite needs a GCC-style host compiler; pass one as the second"
    echo "argument.  The other four suites still cover this platform."
fi
[ "$fail" -eq 0 ]
