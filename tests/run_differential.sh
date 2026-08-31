#!/bin/sh
# Differential suite: every run_ case is also a valid C++98 program once the
# natives it calls are supplied, so a real compiler can answer the same
# question and the two answers can be compared.  This is the suite that
# catches a wrong answer nothing was asserting.
#
#   ./run_differential.sh <compilerpp> [host-c++]
#
# tests/differential_shim.h defines print_int, print_line and the rest on top
# of <iostream>, so a host compiler can build a case unchanged.  Pointer output
# is normalised -- see normalise() below for what and why.
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

# cl is a host compiler like any other and takes none of these flags.  Windows
# is where this suite was skipping every case, which is the one platform where
# a third opinion is available at all -- MSVC is not a GCC, so a disagreement
# there is worth more than another clang saying the same thing.
#
# Three things that are about the SHELL, not the compiler.  cl's options are
# written with a dash because MSYS rewrites an argument beginning with '/' into
# a Windows path.  Paths handed to cl go through cygpath, cl being native and
# Git bash's /tmp not being anywhere it can find.  And -Fe:/-Fo: name the
# executable and the object directory, which cl otherwise drops in the cwd.
NATIVE_WORK="$WORK"
HOST_IS_CL=no
case "$(basename "$HOSTCXX")" in
    cl|cl.exe)
        HOST_IS_CL=yes
        command -v cygpath >/dev/null 2>&1 && NATIVE_WORK=$(cygpath -m "$WORK")
        ;;
esac

# Builds $2 from $1, quiet, into a runnable at $3.  The only thing that varies
# between the two is spelling.
build_case() {
    if [ "$HOST_IS_CL" = yes ]; then
        "$HOSTCXX" -nologo -EHsc -w "-Fe:$NATIVE_WORK/$1.exe" "-Fo:$NATIVE_WORK/" \
                   "$NATIVE_WORK/$1.cpp" > "$2" 2>&1
    else
        "$HOSTCXX" -std=c++98 -w -o "$WORK/$1.bin" "$WORK/$1.cpp" > "$2" 2>&1
    fi
}

# What a pointer LOOKS like is implementation-defined: C++98 says nothing about
# how operator<<(const void*) renders one, and the three hosts disagree three
# ways.  So the address itself is not a comparable answer and becomes a token.
#
# A NULL pointer is different -- whether a pointer is null is a fact about the
# program, and every host can be asked it -- so the spellings of null are
# reduced to the one the VM uses rather than thrown away with the rest.  MSVC
# writes sixteen zeros where clang, GCC and this compiler all write 0.
#
# MSVC pads a non-null pointer to sixteen hex digits with no 0x, which nothing
# here normalises: no case prints one today, and a rule wide enough to catch it
# would also catch a sixteen-digit number a program legitimately printed. If a
# case ever prints a real address on Windows it will show up as a difference,
# which is the right way round -- under-normalising fails loudly, and
# over-normalising hides.
normalise() {
    sed -e 's/0000000000000000/0/g' -e 's/0x[0-9a-fA-F][0-9a-fA-F]*/ADDR/g'
}

# A case the host CANNOT build, for a reason that is about the language rather
# than about this suite.  Named for the same purpose the allowed list is named:
# so that a skip nobody has explained stands out as one.
unbuildable_reason() {
    case "$1" in
        96_run_user_stream_operator)
            echo "it makes an ostream by value; std::ostream cannot be copied" ;;
        *) echo "" ;;
    esac
}

pass=0; fail=0; skip=0; allow=0
for case_file in cases/*run_*.cpp; do
    name=$(basename "$case_file" .cpp)
    input=/dev/null
    [ -f "input/$name.txt" ] && input="input/$name.txt"
    if [ "$HOST_IS_CL" = yes ]; then hostbin="$WORK/$name.exe"; else hostbin="$WORK/$name.bin"; fi

    # The case's own #include lines name headers this compiler synthesises;
    # the shim supplies the same names from the real standard library.
    { cat differential_shim.h; grep -v '^[[:space:]]*#include' "$case_file"; } > "$WORK/$name.cpp"
    if ! build_case "$name" "$WORK/$name.err"; then
        why=$(unbuildable_reason "$name")
        if [ -n "$why" ]; then
            echo "SKIP  $name  ($why)"
        else
            echo "SKIP  $name  (host compiler rejected it, and nothing says why)"
            sed -n '1,3p' "$WORK/$name.err"
        fi
        skip=$((skip + 1))
        continue
    fi

    host=$("$hostbin" < "$input" 2>/dev/null | normalise)
    mine=$("$BIN" -run -q "$case_file" < "$input" 2>/dev/null | normalise)

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
echo "$pass agreed, $fail differed, $allow allowed, $skip skipped, of $((pass + fail + allow + skip)) cases"

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
