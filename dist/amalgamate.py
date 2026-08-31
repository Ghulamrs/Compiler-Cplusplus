#!/usr/bin/env python3
"""Regenerate dist/compilerpp_amalgamated.cpp from the files in Compiler++/.

Run from the project root:   python3 dist/amalgamate.py

The amalgamation exists so the compiler can be built on a machine that has
nothing but a C++ compiler.  It is generated, never edited by hand.
"""
import os
import re

SRC = "Compiler++"
OUT = os.path.join("dist", "compilerpp_amalgamated.cpp")

# Headers first, in DEPENDENCY order -- concatenation has no #include to
# resolve the order for it, so this list is the dependency graph written out.
# Then the translation units, in any order but kept parallel for readability.
# Bytecode.h sits near the front because it describes the MACHINE -- the word
# width, how much memory it has, what it will run -- and that is not only the
# back end's business.  Layout refuses an object too big for the machine, which
# means Layout needs `vmword`, which means Bytecode.h has to be there first.
HEADERS = ["Token.h", "Lexer.h", "Diagnostics.h", "Bytecode.h", "AST.h",
           "AST1.h", "SymbolTable.h", "Parser.h", "Parser1.h", "Semantic.h",
           "Layout.h", "IR.h", "Lower.h", "Lower1.h", "CodeGen.h", "VM.h"]
SOURCES = ["Lexer.cpp", "Diagnostics.cpp", "AST.cpp", "AST1.cpp",
           "SymbolTable.cpp", "Parser.cpp", "Parser1.cpp", "Semantic.cpp",
           "Layout.cpp", "IR.cpp", "Lower.cpp", "Lower1.cpp", "Bytecode.cpp",
           "CodeGen.cpp", "VM.cpp", "main.cpp"]

BANNER = '''// compilerpp_amalgamated.cpp
//
// The whole of Compiler++ in one translation unit, generated from the files in
// Compiler++/ by dist/amalgamate.py.  It exists so the compiler can be built on
// a machine that has nothing but a C++ compiler -- no project file, no CMake,
// no copying twenty files across.
//
//   Windows (MSVC):   cl /W4 /EHsc /Fe:compilerpp.exe compilerpp_amalgamated.cpp
//   Windows (MinGW):  g++ -std=c++98 -Wall -Wextra -o compilerpp.exe compilerpp_amalgamated.cpp
//   Linux / macOS:    g++ -std=c++98 -Wall -Wextra -o compilerpp compilerpp_amalgamated.cpp
//
//   Run:              compilerpp -ast -layout path\\\\to\\\\input.cpp
//
// Define COMPILERPP_NO_MAIN to leave main() out, which is what an application
// embedding the compiler wants: it has a main() already, and a second one does
// not link.  Everything else -- the parser, the analyser, the lowering and the
// VM -- is unchanged and is the whole of what an embedder calls.
//
// DO NOT EDIT.  Edit the files in Compiler++/ and regenerate.
// C++98 only.
'''


def strip_local_includes(text):
    """Local includes are resolved by concatenation.  System includes stay put:
    their own guards make repetition harmless."""
    return re.sub(r'^[ \t]*#include[ \t]*"[^"]+".*\n', '', text, flags=re.M)


def check_complete():
    """Refuse to write a half-empty amalgamation.

    The lists above are hand-ordered because concatenation needs a dependency
    order, but a hand-written list silently goes stale the moment a file is
    added -- which produces an amalgamation that builds and is simply missing a
    pass.  So every file in the source folder must appear in one of the lists.
    """
    listed = set(HEADERS) | set(SOURCES)
    present = set(f for f in os.listdir(SRC) if f.endswith((".h", ".cpp")))
    missing = sorted(present - listed)
    stale = sorted(listed - present)
    if missing:
        raise SystemExit("amalgamate.py is out of date: %s exist in %s/ but are "
                         "not listed. Add them, in dependency order for headers."
                         % (", ".join(missing), SRC))
    if stale:
        raise SystemExit("amalgamate.py lists files that no longer exist: %s"
                         % ", ".join(stale))


def main():
    if not os.path.isdir(SRC):
        raise SystemExit("run this from the project root (the folder holding %s/)" % SRC)
    check_complete()
    parts = [BANNER]
    for group, names in (("HEADERS", HEADERS), ("IMPLEMENTATION", SOURCES)):
        parts.append("\n// " + "=" * 70 + "\n// " + group + "\n// " + "=" * 70 + "\n")
        for name in names:
            parts.append("\n// ---------- %s ----------\n" % name)
            body = strip_local_includes(open(os.path.join(SRC, name)).read())
            # main.cpp is the command-line driver, and an application that
            # embeds this file already has a main().  Two of them do not link,
            # so the driver is the one part that can be compiled out.  Guarded
            # here rather than in main.cpp itself: the per-file build has no
            # such problem, and a #ifdef in the source would be a switch that
            # only the amalgamation ever reads.
            if name == "main.cpp":
                body = ("\n#ifndef COMPILERPP_NO_MAIN\n" + body
                        + "\n#endif  // COMPILERPP_NO_MAIN\n")
            parts.append(body)
    text = "".join(parts)
    if not os.path.isdir("dist"):
        os.mkdir("dist")
    open(OUT, "w").write(text)
    print("wrote %s (%d lines)" % (OUT, text.count("\n") + 1))


if __name__ == "__main__":
    main()
