// Token.h
//
// The token set is shared by both class layers, so it lives at global scope
// and names C and C++ keywords side by side.
//
// C++98 only. No feature from C++11 or later is used anywhere in this project.

#ifndef TOKEN_H
#define TOKEN_H

#include <string>

enum TokenKind {
    TOK_EOF,
    TOK_IDENTIFIER,
    TOK_NUMBER,

    // --- keywords the C layer needs ---
    TOK_INT,
    TOK_CHAR,
    TOK_VOID,
    TOK_BOOL,
    TOK_CONST,
    TOK_RETURN,
    TOK_IF,
    TOK_ELSE,
    TOK_WHILE,
    TOK_FOR,
    TOK_BREAK,
    TOK_CONTINUE,

    // --- keywords the C++ layer adds ---
    TOK_CLASS,
    TOK_STRUCT,
    TOK_PUBLIC,
    TOK_PRIVATE,
    TOK_PROTECTED,
    TOK_VIRTUAL,
    TOK_NEW,
    TOK_DELETE,
    TOK_THIS,

    // --- punctuation ---
    TOK_SEMI,
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_LBRACE,
    TOK_RBRACE,
    TOK_LBRACKET,
    TOK_RBRACKET,
    TOK_COMMA,
    TOK_COLON,      // :
    TOK_COLONCOLON, // ::
    TOK_DOT,        // .
    TOK_ARROW,      // ->
    TOK_TILDE,      // ~   (destructor names)

    // --- operators ---
    TOK_PLUS,
    TOK_MINUS,
    TOK_STAR,
    TOK_SLASH,
    TOK_PERCENT,
    TOK_ASSIGN,     // =
    TOK_EQ,         // ==
    TOK_NE,         // !=
    TOK_LT,         // <
    TOK_GT,         // >
    TOK_LE,         // <=
    TOK_GE,         // >=
    TOK_ANDAND,     // &&
    TOK_OROR,       // ||
    TOK_NOT,        // !
    TOK_AMP,        // &

    TOK_UNKNOWN
};

// A token remembers WHERE it came from.  Every diagnostic in the compiler is
// only as good as this: without a line and column, an error message can say
// what went wrong but never where.  The position is copied onto AST nodes as
// they are built, so the semantic pass can point at source too.
struct Token {
    TokenKind kind;
    std::string text;
    int numberValue;
    int line;
    int col;
    Token() : kind(TOK_UNKNOWN), numberValue(0), line(0), col(0) {}
};

// Human-readable spelling, for "expected X, found Y" messages.
const char *tokenName(TokenKind k);

#endif
