//
//  main.cpp
//  Compiler++
//
//  Created by G. R. Akhtar on 29/08/2026.
//
//  Reads a source file and runs it through the front end:
//
//      cc::Parser        -- LAYER 1, the C layer   (base class)
//      cxx::Parser       -- LAYER 2, the C++ layer (derives from cc::Parser)
//      SemanticAnalyzer  -- PASS 3, walks the mixed cc:: / cxx:: tree
//      Diagnostics       -- every complaint, with a file:line:col
//
//  Usage:   Compiler++ [options] [source-file]
//             -ast        print the syntax tree
//             -layout     print each class's object layout and vtable
//             -q          diagnostics only, no banner
//
//  With no file it reads DEFAULT_INPUT below.  Xcode runs the binary with its
//  working directory set to the build products folder, not the project folder,
//  so the default is an absolute path; pass a path on the command line
//  (Product > Scheme > Edit Scheme > Arguments) to analyse a different file.
//
//  Exit status is 0 when the file has no errors, 1 when it has some, and 2
//  when the file could not be read -- so a test script can just check it.
//
//  C++98 only.
//

#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "AST.h"
#include "AST1.h"
#include "Diagnostics.h"
#include "Layout.h"
#include "Parser.h"
#include "Parser1.h"
#include "Semantic.h"

// The test input lives in the PROJECT folder, one level above the sources.
// It deliberately sits outside the Compiler++ source folder: that folder is a
// synchronized group, so any .cpp inside it would be compiled into this target.
static const char *DEFAULT_INPUT =
    "/Users/g.r.akhtar/Documents/Compiler++/point_input.cpp";

static bool readFile(const std::string &path, std::string &out) {
    std::ifstream in(path.c_str());
    if (!in) return false;
    std::ostringstream ss;
    ss << in.rdbuf();
    out = ss.str();
    return true;
}

// Everything after the last '/' -- diagnostics read better without the path.
static std::string baseName(const std::string &path) {
    const std::string::size_type slash = path.find_last_of('/');
    return (slash == std::string::npos) ? path : path.substr(slash + 1);
}

int main(int argc, char **argv) {
    bool showAst = false;
    bool showLayout = false;
    bool quiet = false;
    std::string path;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if      (arg == "-ast")    showAst = true;
        else if (arg == "-layout") showLayout = true;
        else if (arg == "-q")      quiet = true;
        else                       path = arg;
    }
    if (path.empty()) path = DEFAULT_INPUT;

    std::string source;
    if (!readFile(path, source)) {
        std::cerr << "Cannot open input file: " << path << std::endl;
        return 2;
    }

    Diagnostics diag(baseName(path));

    // LAYERS 1+2.  parseTranslationUnit() is the C layer's, but every hook it
    // calls is virtual, so a cxx::Parser instance parses classes, references,
    // member access, `this` and `new` through the very same loop.
    cxx::Parser parser(source, diag);
    std::vector<cc::Decl*> unit = parser.parseTranslationUnit();

    // PASS 3.  Analysis runs even after syntax errors: the tree is still worth
    // walking, and a second round of diagnostics is more useful than silence.
    // It comes BEFORE printing, because it writes conclusions back into the
    // tree -- which methods are virtual, and what each one overrides -- and a
    // dump taken beforehand would not show them.
    {
        SemanticAnalyzer sem(diag);
        sem.analyze(unit);

        if (showAst) {
            if (!quiet) std::cout << "=== SYNTAX TREE: " << baseName(path) << " ===" << std::endl;
            for (std::size_t i = 0; i < unit.size(); ++i) unit[i]->print(0);
            std::cout << std::endl;
        }

        // PASS 4.  The object model: offsets, sizes and vtables.  It runs
        // ALWAYS, not only when printing, because it enforces constraints of
        // its own -- a diagnostic must not depend on whether a debug flag was
        // passed.  The flag controls the dump, not the pass.
        Layout layout(diag);
        layout.computeAll(sem.classMap());
        if (showLayout) {
            if (!quiet) std::cout << "=== OBJECT LAYOUT: " << baseName(path) << " ===" << std::endl;
            layout.print();
        }
    }

    if (!quiet) diag.printSummary();

    for (std::size_t i = 0; i < unit.size(); ++i) delete unit[i];
    return diag.hadError() ? 1 : 0;
}
