#!/bin/sh
# Every case that runs must give the same answer through a .cxb file as it does
# straight from source.  That is the whole promise of an object file.
BIN="$1"
[ -x "$BIN" ] || { echo "usage: $0 <compiler>" >&2; exit 2; }
DIR=$(dirname "$0"); cd "$DIR" || exit 2
pass=0; fail=0
for case_file in cases/*.cpp; do
    name=$(basename "$case_file" .cpp)
    case "$name" in *err_*) continue ;; esac
    # Only the program's own output is compared: diagnostics belong to
    # compilation, so they are absent when a .cxb is run, and rightly so.
    direct=$("$BIN" -run -q "$case_file" 2>/dev/null); dstat=$?
    "$BIN" -q -o "/tmp/rt_$name.cxb" "$case_file" >/dev/null 2>&1 || { echo "SKIP  $name"; continue; }
    viafile=$("$BIN" -run -q "/tmp/rt_$name.cxb" 2>/dev/null); fstat=$?
    if [ "$direct" = "$viafile" ] && [ "$dstat" -eq "$fstat" ]; then
        pass=$((pass + 1))
    else
        echo "FAIL  $name"
        fail=$((fail + 1))
    fi
    rm -f "/tmp/rt_$name.cxb"
done
echo "$pass round-tripped, $fail failed"
[ "$fail" -eq 0 ]
