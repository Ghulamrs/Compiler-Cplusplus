// Diagnostics.h
//
// One place for every complaint the compiler makes.
//
// Before this class existed, each parser error called std::exit(1): the first
// mistake in a file ended the process, so a second error could never be shown
// and no test could assert on malformed input.  Reporting is now separated from
// deciding what to do next -- the parser reports and then recovers, the
// semantic pass reports and carries on -- and main() asks at the end whether
// anything went wrong.
//
// Messages use the conventional  file:line:col: error: text  format, which is
// also the format Xcode parses into clickable entries in its issue navigator.
//
// C++98 only.

#ifndef DIAGNOSTICS_H
#define DIAGNOSTICS_H

#include <string>

class Diagnostics {
public:
    explicit Diagnostics(const std::string &sourceName);

    void error(int line, int col, const std::string &msg);
    void warning(int line, int col, const std::string &msg);

    int errorCount() const { return errors; }
    int warningCount() const { return warnings; }
    bool hadError() const { return errors > 0; }

    // Printed once at the end of a run.
    void printSummary() const;

private:
    std::string name;
    int errors;
    int warnings;

    void report(const char *level, int line, int col, const std::string &msg);

    // not copyable (C++98 way: declared private, never defined)
    Diagnostics(const Diagnostics &);
    Diagnostics &operator=(const Diagnostics &);
};

#endif
