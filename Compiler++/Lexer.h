// Lexer.h

#include <cctype>
#include <iostream>
#include <string>
#include "Token.h"

class Lexer {
public:
    Lexer(const std::string &s) : src(s), pos(0) {}
    Token nextToken();

private:
    std::string src;
    size_t pos;

    char peek() const { return pos < src.size() ? src[pos] : '\0'; }
    char get() { return pos < src.size() ? src[pos++] : '\0'; }
    void skipWhitespace() {
        while (std::isspace(peek())) get();
    }
    bool startsWith(const std::string &kw) {
        if (src.size() - pos < kw.size()) return false;
        return src.substr(pos, kw.size()) == kw;
    }
};
