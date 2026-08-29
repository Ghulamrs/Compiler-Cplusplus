// Token.h

#ifndef TOKEN_H
#define TOKEN_H

#include <string>

enum TokenKind {
    TOK_EOF,
    TOK_IDENTIFIER,
    TOK_NUMBER,
    TOK_INT,
    TOK_RETURN,
    TOK_SEMI,
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_LBRACE,
    TOK_RBRACE,
    TOK_PLUS,
    TOK_MINUS,
    TOK_STAR,
    TOK_SLASH,
    TOK_ASSIGN,

//  TokenKind enum
    TOK_CLASS,
    TOK_STRUCT,
    TOK_PUBLIC,
    TOK_PRIVATE,
    TOK_PROTECTED,
    TOK_COLON,      // :
    TOK_COLONCOLON, // ::
    TOK_DOT,        // .
    TOK_ARROW,      // ->
    TOK_AMP,        // &

    TOK_UNKNOWN
};

struct Token {
    TokenKind kind;
    std::string text;
    int numberValue;
    Token() : kind(TOK_UNKNOWN), numberValue(0) {}
};

#endif
