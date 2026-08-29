// Diagnostics.cpp
//
// C++98 only.

#include "Diagnostics.h"

#include <iostream>

Diagnostics::Diagnostics(const std::string &sourceName)
    : name(sourceName), errors(0), warnings(0), capped(false), lineOffset(0) {}

void Diagnostics::report(const char *level, int line, int col, const std::string &msg) {
    std::cout.flush();          // keep diagnostics in step with any AST dump
    std::cerr << name;
    const int shown = line - lineOffset;
    if (shown > 0) std::cerr << ":" << shown << ":" << col;
    std::cerr << ": " << level << ": " << msg << std::endl;
}

void Diagnostics::error(int line, int col, const std::string &msg) {
    ++errors;
    if (errors > MaxReported) {
        if (!capped) {
            capped = true;
            std::cerr << name << ": error: too many errors; stopping here" << std::endl;
        }
        return;
    }
    report("error", line, col, msg);
}

void Diagnostics::warning(int line, int col, const std::string &msg) {
    ++warnings;
    if (capped || warnings > MaxReported) return;
    report("warning", line, col, msg);
}

void Diagnostics::printSummary() const {
    std::cout.flush();
    if (errors == 0 && warnings == 0) {
        std::cout << "No errors." << std::endl;
        return;
    }
    std::cerr << errors << " error(s), " << warnings << " warning(s)." << std::endl;
}
