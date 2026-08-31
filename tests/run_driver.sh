#!/bin/sh
#
# What the compiler does with its ARGUMENTS, which no case file can ask.
#
#   ./run_driver.sh <path-to-Compiler++-binary>
#
# The other four suites all hand the compiler a file and a set of switches that
# are known good, so every one of them passed while `compilerpp` with no
# arguments read an absolute path inside one developer's home directory, and
# `--help` was reported as a file that could not be opened.  Nothing was
# checking the driver because nothing could.
#
# Each check states the argument line, the exit status it must give, and a
# phrase its output must contain.  Exit status matters as much as the words: a
# script that runs the compiler wants to tell a clean compile from a refused
# command line, and only the status says so.

BIN="$1"
if [ -z "$BIN" ] || [ ! -x "$BIN" ]; then
    echo "usage: $0 <path-to-Compiler++-binary>" >&2
    exit 2
fi

DIR=$(dirname "$0")
cd "$DIR" || exit 2

pass=0
fail=0

# check <description> <want-status> <phrase-that-must-appear> -- <args...>
check() {
    what="$1"; want="$2"; phrase="$3"
    shift 3
    [ "$1" = "--" ] && shift
    out=$("$BIN" "$@" 2>&1 </dev/null)
    got=$?
    if [ "$got" -ne "$want" ]; then
        echo "FAIL     $what  (exit $got, wanted $want)"
        fail=$((fail + 1))
        return
    fi
    case "$out" in
        *"$phrase"*) echo "ok       $what"; pass=$((pass + 1)) ;;
        *)
            echo "FAIL     $what  (output does not mention '$phrase')"
            printf '%s\n' "$out" | sed -n '1,3p'
            fail=$((fail + 1))
            ;;
    esac
}

# No arguments is a mistake, not a request to compile something: usage, and a
# status that says the command line was refused.
check "no arguments"      2 "usage: compilerpp"

# Asking for help is not a mistake, so it succeeds and prints to stdout.
check "-h"                0 "-run      compile and run"  -- -h
check "--help"            0 "-run      compile and run"  -- --help

# An unrecognised switch used to become the input path.
check "unknown option"    2 "Unknown option: -zap"       -- -zap cases/36_run_c_layer.cpp

# `-o` last on the line used to become the input path too.
check "dangling -o"       2 "-o needs a file to write"   -- cases/36_run_c_layer.cpp -o

# A second input silently replaced the first.
check "two input files"   2 "Only one input file"        -- cases/36_run_c_layer.cpp cases/38_run_arithmetic.cpp

# A file that is not there is a different failure from a bad command line, and
# says so -- but shares the status, both being "nothing could be read".
check "missing input"     2 "Cannot open input file"     -- no_such_file.cpp

# The ordinary path still works, and -q still suppresses the banner.
check "a file compiles"   0 "No errors."                 -- cases/36_run_c_layer.cpp
check "a file runs"       0 "42"                         -- -run -q cases/38_run_arithmetic.cpp

echo
echo "$pass passed, $fail failed"
[ "$fail" -eq 0 ]
