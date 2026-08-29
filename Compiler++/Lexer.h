// Lexer.h -- one lexer for both layers, so it stays at global scope.
//
// C++98 only.

#ifndef LEXER_H
#define LEXER_H

#include <cctype>
#include <cstddef>
#include <string>
#include "Token.h"

class Lexer {
public:
    Lexer(const std::string &s) : src(s), pos(0), line(1), col(1) {}
    Token nextToken();

    // Rewind, for the parser's speculation.  Line and column travel with the
    // offset, or later tokens would report the wrong place.
    struct Position {
        std::size_t offset;
        int line;
        int col;
    };
    Position tell() const;
    void seek(const Position &p);

private:
    std::string src;
    std::size_t pos;
    int line;
    int col;

    char peek() const { return pos < src.size() ? src[pos] : '\0'; }
    char peekAt(std::size_t ahead) const {
        return pos + ahead < src.size() ? src[pos + ahead] : '\0';
    }
    // The only place the position advances, so line and column cannot drift.
    char get();
    void skipWhitespaceAndComments();

    Token makeToken(TokenKind k, int startLine, int startCol);
};

Lexer *createLexer(const std::string &s);

// Object-like macros, expanded before anything is lexed.
//
//     #define PI 3.14159
//
// is a textual substitution and nothing more -- which is all a constant needs,
// and is why it can live here rather than in a pass of its own.  A directive
// line is blanked rather than removed, so every later line keeps its number.
// Function-like macros and conditional compilation are refused by name.
class Diagnostics;
std::string expandDefines(const std::string &src, Diagnostics &diag);

#endif
