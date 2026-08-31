# Compiler++

A C++ compiler for a subset of C++, written by hand in C++98, targeting its own
stack VM. It compiles, links, and runs its own output: `compilerpp -run x.cpp`
executes the program, and `-o x.cxb` writes a bytecode object file that
`-run x.cxb` runs with no source present.

~13,000 lines. Nothing is generated; there is no parser generator, no runtime
library, and no dependency beyond the C++98 standard library.

## Layout

```
CLAUDE.md          this file
KNOWN-GAPS.md      defects found and not yet fixed -- read it before starting
CMakeLists.txt     builds and runs the suite anywhere; Xcode is the primary IDE
Compiler++/        all the source
tests/
  cases/           NNN_kind_name.cpp -- the whole corpus
  expected/        golden -ast -layout -ir dumps, one per case
  expected_run/    golden program output, for run_ and trap_ cases
  input/           stdin for a case that reads cin (optional, by name)
  images/          malformed .cxb files the VM must refuse, with their messages
  differential_shim.h  the natives a case calls, over the real <iostream>
  run_tests.sh run_exec.sh run_roundtrip.sh run_differential.sh run_driver.sh
  run_amalgamated.sh
dist/              amalgamate.py and a single-file build of the whole compiler
                   -- generated, committed, and checked by run_amalgamated.sh
_backup_*/         pre-git snapshots, gitignored, superseded by history
```

## The two layers

The single most important thing about this codebase.

- **The C layer** (`namespace cc`) is `AST`, `Parser`, `Lower` — a C89-ish
  language: builtins, pointers, arrays, functions, statements.
- **The C++ layer** (`namespace cxx`) is the same names with a `1` appended —
  `AST1`, `Parser1`, `Lower1`. Each class *derives* from its C-layer
  counterpart: `cxx::Parser : cc::Parser`, `cxx::Lowering : cc::Lowering`.
- Everything else is single-layer: `Lexer`, `Token`, `Semantic`, `SymbolTable`,
  `Layout`, `IR`, `Bytecode`, `CodeGen`, `VM`, `Diagnostics`.

**A new language feature goes in the LOWEST layer that has it.** If C has the
construct, it belongs in the C layer and the C++ layer inherits it by
derivation. Only what C++ adds — classes, references, `bool`, `new`/`delete`,
operator overloading — belongs in a `1` file.

The C layer must not know about the C++ layer. One violation exists and is
recorded: `IR.cpp`'s `typeCode` `dynamic_cast`s to `cxx::` types because the
name mangler happens to live there. Moving `mangle*` out would let `IR.h`/`IR.cpp`
drop both AST includes.

## Build and test

CMake is the portable path but is not needed. On any machine with a C++98
compiler:

```sh
mkdir -p /tmp/b
ls Compiler++/*.cpp | xargs -P 8 -I{} sh -c \
  'n=$(basename "{}" .cpp); g++ -std=c++98 -Wall -Wextra -c "{}" -o /tmp/b/$n.o'
g++ -o /tmp/b/compilerpp /tmp/b/*.o

sh tests/run_tests.sh     /tmp/b/compilerpp     # dumps + exit status
sh tests/run_exec.sh      /tmp/b/compilerpp     # program output
sh tests/run_roundtrip.sh /tmp/b/compilerpp     # same output through a .cxb
sh tests/run_differential.sh /tmp/b/compilerpp  # same answer as a real compiler
sh tests/run_driver.sh    /tmp/b/compilerpp     # what the ARGUMENTS do
sh tests/run_amalgamated.sh /tmp/b/compilerpp   # dist/ is still this compiler
```

`run_amalgamated.sh` and `run_differential.sh` each take the host compiler as a
second argument, and on Windows that is `cl`, from a shell that has sourced
`vcvars64.bat` — a `.bat` that calls it and then invokes `bash -c` (never
`bash -lc`, which rebuilds PATH from `/etc/profile` and loses cl again).

All six must pass before anything is committed. Add `--accept` to a runner to
re-record its golden files — and then **read the diff**, because `--accept`
happily records a bug. `run_differential.sh` and `run_driver.sh` have no golden
files to record: for the first the host compiler is the answer, and for the
second the check is written beside the argument line it makes. `run_differential.sh` needs a
GCC-style host compiler and says **NOTHING WAS COMPARED** where there is none —
which is Windows, `cl` taking different flags — so the four that do run there
are the coverage on that platform.

`-pedantic` is clean except for twelve `-Wlong-long` warnings from the VM's
fixed 64-bit word (`Bytecode.h`). That is the one knowing departure from the
C++98 rule; see KNOWN-GAPS.md for the way to remove it.

## Rules

1. **C++98 only, in the compiler's own source.** No `auto`, `nullptr`,
   range-`for`, `override`, smart pointers, `>>` without a space. This is a
   standing constraint, not a preference.
2. **Change the minimum inside the files.** Architecture changes are welcome
   when they buy efficiency or comprehension, but not as a side effect.
3. **Diagnostics say "in this version", never "in this subset."** They state
   the fact and stop — no trailing advice, except where naming the alternative
   IS the fact (`use (T)value`, `write a declaration each`).
4. **One mistake costs one line.** An excluded construct is lexed, named once,
   and skipped. A cascade of parse errors is a bug in the diagnostic, not in
   the program.
5. **Nothing in a `.cxb` is trusted.** Every length, offset, index and opcode a
   file claims is checked before it is used. A malformed image must produce a
   named runtime error and exit 3 — never a crash, a hang, or a clean exit.

## Test conventions

The name says what the case must do:

| Prefix in the name | Must |
|---|---|
| `NNN_err_*`  | be REJECTED — exit 1 |
| `NNN_warn_*` | compile, with warnings |
| `NNN_run_*`  | compile AND run; output compared byte for byte |
| `NNN_trap_*` | compile, then stop in the VM — exit 3 with a named error |

A case that reads `cin` gets `tests/input/NAME.txt`; everything else reads
`/dev/null`, so no case can wait on a terminal. Numbers are sequential; use the
next free one.

`tests/images/*.cxb` are committed as exact bytes, each with a `.txt` holding
the message it must produce. Their README explains how they were built.

## Things that look like accidents and are not

- **`bool` lives in the C++ layer** (`cxx::BoolType`), not as a `cc::BuiltinKind`.
  Layer 1's grammar stays honestly C89, and bool promotes to `int` on entering
  arithmetic and never survives as a computation's type, so the rank table needs
  no entry for it.
- **There is no string type.** `char*` plus string literals is the whole
  facility. A `String` class is something to write *in* Compiler++ — and doing
  so is the best completeness test this project has.
- **`<iostream>` is written in the language itself** and prepended as one line
  when a program includes it (`Lexer.cpp`, `preludeFor`). `ostream` and
  `istream` hold nothing; every operator forwards to a native and returns
  `*this`. Whether the last read succeeded lives in the VM, not in `cin`, which
  is why `cin.good()` is right after a whole chain.
- **`new[]` stores its element count in the block header** — in the free-list
  `next` field, spare while the block is allocated — not in a cookie in front of
  the payload. The block's *size* cannot stand in for the count: blocks round up
  to a multiple of eight. Because the form is recorded, `delete` on a `new[]`
  block is a named error rather than undefined.
- **`cin >> buf` asks the frame table** how much room a local buffer has. The
  array decayed and its type is gone, but the slot is still described by the
  function's own layout, and the VM keeps every frame.
- **`OP_AllocN` and `OP_ArrayCount` sit after `OP_Halt`**, out of category, so
  that no existing opcode's value moves. Committed `.cxb` fixtures depend on
  that. Append new opcodes at the end; do not tidy the enum.
- **A generated copy constructor names its array members**, which C++ has no
  syntax for and a user still cannot write — `analyzeMemberInits` admits the
  entry only when the constructor `isImplicit`. What the entry means is settled
  in lowering: elements that are objects are copy-constructed one at a time,
  and anything else is a whole-array move. Refusing to name them was the
  earlier design, and it cost the class its copy constructor altogether — so a
  member beside the array never ran its own, and one owning memory was freed
  twice. The list is not where to look for what an array member does.

## Traps

- **Early returns that skip the common path.** Four have been found: an operator
  result that missed `stripReference`; a constructor list tested for *empty*
  rather than for a *user-written* entry; `bool` returning before the reference
  suffix; `parseType` consuming a `const` it then failed on. When adding a
  branch near the top of a function, check what the bottom of that function
  does.
- **The overload-resolution loop is written five times** — `findFreeOperator`,
  `findIndexOperator`, `findMemberOperator`, `selectConstructor`,
  `resolveOverload` — with subtly different exactness tests. Several bugs came
  from the copies diverging, and stating one new rule means editing five lines.
  Folding them into one shared candidate-ranking routine is the highest-value
  refactor available.
- **`Layout` computes a `constructionPlan` nothing consumes.** `Lower1` re-derives
  the same ordering by hand, and the hand copy is where a bug lived.
- **The suite passes while the compiler is wrong.** It is an inventory of what
  was built, not a specification of the language: every defect in KNOWN-GAPS.md
  was found by reading or by adversarial probing, never by the suite.
  `run_differential.sh` is the answer to that, and the array-member copy is the
  first defect it found on its own. It works because a `run_` case is valid
  C++98 EXCEPT for the natives it calls — `print_int`, `print_line` and the
  rest, which no real toolchain has — so `differential_shim.h` defines them
  over the real `<iostream>` and the host compiles the case unchanged. Two
  cases are listed in the runner as allowed to differ, both because the host
  elides a return copy and this compiler does not; both are conforming. Add to
  that list only for a claim about the STANDARD, never to quiet a failure.
- **A generated file that is committed goes stale while still compiling.**
  `dist/compilerpp_amalgamated.cpp` was regenerated by hand, so it drifted
  twenty-two commits behind and nothing noticed — it built the whole time,
  which is exactly what made it look current. It was a different compiler:
  no `new[]`, no `cin`, no `bool&`, and the copy semantics of three weeks
  earlier, shipped to anyone who built the single-file distribution.
  `run_amalgamated.sh` regenerates into a temporary file and compares, so the
  drift is now a failing test rather than a discovery. Regenerate with
  `python3 dist/amalgamate.py` and commit the result with the change that
  caused it.
- **Line endings, both directions.** The whole tree is LF, pinned by
  `.gitattributes`, because the golden files are compared byte for byte against
  output written with `\n`. A CRLF checkout fails twelve of them against a
  correct compiler. The other direction is `main.cpp`'s
  `writeUntranslatedOutput`: Windows translates `'\n'` on a text-mode stream, so
  without it the compiler writes CRLF there and **every** case fails — 120 of
  120, on a compiler that is entirely correct. Pinning the checkout settles the
  bytes going in; that call settles the bytes coming out.

## Commits

One fix per commit, with a regression case. The message explains what was
wrong and why the fix is shaped the way it is — not what changed, which the
diff already says. Present tense in the subject, no ticket numbers, no
scope prefixes.

## Where to look next

`KNOWN-GAPS.md` is the ledger, ranked by whether the compiler **lies**:
accepts-and-miscompiles first, accepts-what-it-should-reject second,
rejects-what-it-should-accept last. The tier-three items are the most
irritating and the least dangerous, which is why they tend to get fixed first;
resist that.
