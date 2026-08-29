// Lexer.cpp
//
// C++98 only.

#include "Lexer.h"
#include <cctype>
#include <cstdlib>
#include <iostream>
#include <string>

Token Lexer::nextToken() {
    skipWhitespace();
    Token tok;
    char c = peek();
    if (c == '\0') { tok.kind = TOK_EOF; return tok; }

    if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
        std::string id;
        while (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_') id += get();
        if      (id == "int") tok.kind = TOK_INT;
        else if (id == "return") tok.kind = TOK_RETURN;
        else if (id == "class") tok.kind = TOK_CLASS;
        else if (id == "struct") tok.kind = TOK_STRUCT;
        else if (id == "public") tok.kind = TOK_PUBLIC;
        else if (id == "private") tok.kind = TOK_PRIVATE;
        else if (id == "protected") tok.kind = TOK_PROTECTED;
        else { tok.kind = TOK_IDENTIFIER; tok.text = id; }
        return tok;
    }

    if (std::isdigit(static_cast<unsigned char>(c))) {
        std::string num;
        while (std::isdigit(static_cast<unsigned char>(peek()))) num += get();
        tok.kind = TOK_NUMBER;
        tok.text = num;
        tok.numberValue = std::atoi(num.c_str());
        return tok;
    }

    char punct = get();
    switch (punct) {
    case ';': tok.kind = TOK_SEMI; break;
    case '(': tok.kind = TOK_LPAREN; break;
    case ')': tok.kind = TOK_RPAREN; break;
    case '{': tok.kind = TOK_LBRACE; break;
    case '}': tok.kind = TOK_RBRACE; break;
    case ',': tok.kind = TOK_COMMA; break;
    case '+': tok.kind = TOK_PLUS; break;
    case '-':
            if (peek() == '>') { get(); tok.kind = TOK_ARROW; }
            else { tok.kind = TOK_MINUS; }
            break;
    case '*': tok.kind = TOK_STAR; break;
    case '/': tok.kind = TOK_SLASH; break;
    case '=': tok.kind = TOK_ASSIGN; break;
    case '&': tok.kind = TOK_AMP; break;
    case '.': tok.kind = TOK_DOT; break;
    case ':':
            if (peek() == ':') { get(); tok.kind = TOK_COLONCOLON; }
            else tok.kind = TOK_COLON;
            break;
    default:
        tok.kind = TOK_UNKNOWN;
        tok.text = std::string(1, punct);
        break;
    }
    return tok;
}

// Provide a simple factory function for the parsers to use
Lexer *createLexer(const std::string &s) {
    return new Lexer(s);
}
