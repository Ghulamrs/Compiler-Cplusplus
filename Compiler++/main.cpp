//
//  main.cpp
//  Compiler++
//
//  Created by G. R. Akhtar on 29/08/2026.
//
//  Reads a source file, runs it through the layered front end, and reports
//  what the semantic pass found:
//
//      cc::Parser        -- LAYER 1, the C layer   (base class)
//      cxx::Parser       -- LAYER 2, the C++ layer (derives from cc::Parser)
//      SemanticAnalyzer  -- PASS 3, walks the mixed cc:: / cxx:: tree
//
//  Usage:   Compiler++ [source-file]
//           Compiler++ --demos          run the built-in layering demos instead
//
//  With no argument it reads DEFAULT_INPUT below.  Xcode runs the binary with
//  its working directory set to the build products folder, not the project
//  folder, so the default is an absolute path; pass a path on the command line
//  (Product > Scheme > Edit Scheme > Arguments) to analyse a different file.
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

// ---------------------------------------------------------------------
// The real job: parse a file with the C++ layer and analyse it
// ---------------------------------------------------------------------
static int analyzeFile(const std::string &path) {
    std::string source;
    if (!readFile(path, source)) {
        std::cerr << "Cannot open input file: " << path << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "=== SOURCE: " << path << " ===" << std::endl;
    std::cout << source << std::endl;

    // LAYERS 1+2.  parseTranslationUnit() is the C++ layer's, but the function
    // bodies inside it are parsed by cc::Parser::parseBlock() and the whole
    // +-*/ precedence chain, inherited unchanged from the C layer.
    std::cout << "=== PARSE (cxx::Parser over cc::Parser) ===" << std::endl;
    cxx::Parser parser(source);
    std::vector<cxx::Decl*> unit = parser.parseTranslationUnit();
    for (std::size_t i = 0; i < unit.size(); ++i) {
        unit[i]->print(0);
    }
    std::cout << std::endl;

    std::cout << "=== SEMANTIC ANALYSIS (SemanticAnalyzer) ===" << std::endl;
    SemanticAnalyzer sem;
    sem.analyze(unit);

    int errs = sem.errors();
    std::cout << std::endl;
    if (errs > 0) {
        std::cout << "Semantic analysis found " << errs << " error(s)." << std::endl;
    } else {
        std::cout << "Semantic analysis passed with no errors." << std::endl;
    }

    for (std::size_t i = 0; i < unit.size(); ++i) delete unit[i];
    return errs > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}

// ---------------------------------------------------------------------
// The built-in demos, kept for what they document about the layering
// ---------------------------------------------------------------------

// LAYER 1 on its own: plain C, parsed by the base class.
static void runCLayer() {
    const std::string source =
        "int main() {"
        "    int a = 1 + 2 * 3;"
        "    int b = (a - 1) / 2;"
        "    return a + b;"
        "}";

    std::cout << "=== LAYER 1: C layer (cc::Parser) ===" << std::endl;
    cc::Parser parser(source);
    cc::Function *fn = parser.parse();
    fn->print(0);
    delete fn;
    std::cout << std::endl;
}

// LAYER 2 on its own: C++-only grammar the base class knows nothing about.
static void runCppLayer() {
    const std::string source =
        "class Point {"
        "public:"
        "    int getX();"
        "    int distance(int dx, int dy) { return 0; }"
        "    int move(Point& target, int* delta) { return 0; }"
        "private:"
        "    int x;"
        "    int y;"
        "    int* cache;"
        "    int** grid;"
        "};"
        "Point origin;"
        "Point* current;";

    // Note the types below: int / int* / int** come from cc::Parser::parseType(),
    // while Point& and Point* come from cxx::Parser::parseType() overriding it.
    std::cout << "=== LAYER 2: C++ layer (cxx::Parser) ===" << std::endl;
    cxx::Parser parser(source);
    std::vector<cxx::Decl*> unit = parser.parseTranslationUnit();
    for (std::size_t i = 0; i < unit.size(); ++i) {
        unit[i]->print(0);
        delete unit[i];
    }
    std::cout << std::endl;
}

// The two layers cooperating, which is the point of the inheritance:
//
//   * parseBlock(), parseStatement(), parseDeclTail() and the whole +-*/
//     precedence chain are INHERITED from cc::Parser -- not one line of that
//     is repeated in the C++ layer.
//   * cc::Parser::parseMulDiv() calls parsePrimary() virtually, so it lands in
//     cxx::Parser::parsePrimary(), which understands  obj.field  and  p->next.
//   * cc::Parser::parseStatement() calls parseType() virtually, so a C++ type
//     declares a C statement:  Point p;  and  int &r = p.x;
//   * The result is ONE tree mixing cc:: and cxx:: nodes, because
//     cxx::MemberAccessExpr derives from cc::Expr.
static void runLayeredParse() {
    const std::string source =
        "class Point { public: int x; int y; };"
        "Point origin;"
        "Point* current;"
        "int main() {"
        "    int d = origin.x + current->y * 2;"
        "    return d;"
        "}";

    std::cout << "=== LAYERS 1+2: cxx::Parser using the inherited C-layer rules ===" << std::endl;
    cxx::Parser parser(source);
    std::vector<cxx::Decl*> unit = parser.parseTranslationUnit();
    for (std::size_t i = 0; i < unit.size(); ++i) {
        unit[i]->print(0);
    }

    SemanticAnalyzer sem;
    sem.analyze(unit);
    std::cout << (sem.hadError() ? "  [semantics: FAILED]" : "  [semantics: ok]") << std::endl;

    for (std::size_t i = 0; i < unit.size(); ++i) delete unit[i];
    std::cout << std::endl;
}

static void runDemos() {
    runCLayer();
    runCppLayer();
    runLayeredParse();
}

int main(int argc, char **argv) {
    if (argc > 1 && std::string(argv[1]) == "--demos") {
        runDemos();
        return EXIT_SUCCESS;
    }
    const std::string path = (argc > 1) ? std::string(argv[1]) : std::string(DEFAULT_INPUT);
    return analyzeFile(path);
}
