// Lexer.h
//
// C++98 only.

#ifndef LEXER_H
#define LEXER_H

#include <cctype>
#include <cstddef>
#include <string>
#include "Token.h"

// One lexer serves both class layers, so it stays at global scope.
class Lexer {
public:
    Lexer(const std::string &s) : src(s), pos(0) {}
    Token nextToken();

    // Speculation support.  The parser sometimes has to try a rule and take it
    // back -- inside a function body,  Point p;  and  p.x = 1;  both start with
    // an identifier, so the declaration rule is attempted first and rewound if
    // it does not fit.  See cc::Parser::save() / restore().
    std::size_t tell() const { return pos; }
    void seek(std::size_t p) { pos = p; }

private:
    std::string src;
    std::size_t pos;

    char peek() const { return pos < src.size() ? src[pos] : '\0'; }
    char get() { return pos < src.size() ? src[pos++] : '\0'; }
    // Whitespace and comments are equivalent for the grammar, so they are
    // skipped together:  //  to end of line, and  /* */  possibly spanning lines.
    void skipWhitespace() {
        for (;;) {
            while (std::isspace(static_cast<unsigned char>(peek()))) get();
            if (peek() == '/' && pos + 1 < src.size() && src[pos + 1] == '/') {
                while (peek() != '\0' && peek() != '\n') get();
                continue;
            }
            if (peek() == '/' && pos + 1 < src.size() && src[pos + 1] == '*') {
                get(); get();                       // consume the opening /*
                while (peek() != '\0') {
                    if (peek() == '*' && pos + 1 < src.size() && src[pos + 1] == '/') {
                        get(); get();               // consume the closing */
                        break;
                    }
                    get();
                }
                continue;
            }
            return;
        }
    }
    bool startsWith(const std::string &kw) {
        if (src.size() - pos < kw.size()) return false;
        return src.substr(pos, kw.size()) == kw;
    }
};

Lexer *createLexer(const std::string &s);

#endif
