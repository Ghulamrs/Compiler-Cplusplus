// Lexer.h
//
// One lexer serves both class layers, so it stays at global scope: the token
// set is the one thing C and C++ genuinely share unchanged.
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

    // Speculation support.  The parser sometimes has to try a rule and take it
    // back -- inside a function body,  Point p;  and  p.x = 1;  both start with
    // an identifier, so the declaration rule is attempted first and rewound if
    // it does not fit.  The line and column travel with the offset, or a rewind
    // would leave later tokens reporting the wrong place.
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
    // The single place the position advances, so line and column cannot drift
    // out of step with the offset.
    char get();

    // Whitespace and comments are equivalent for the grammar, so they are
    // skipped together:  //  to end of line, and  /* */  possibly spanning lines.
    void skipWhitespaceAndComments();

    Token makeToken(TokenKind k, int startLine, int startCol);
};

Lexer *createLexer(const std::string &s);

#endif
