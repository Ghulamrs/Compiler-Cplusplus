// Token.h -- the token set, shared by both layers.
//
// C++98 only.  No feature from C++11 or later is used anywhere in this project.

#ifndef TOKEN_H
#define TOKEN_H

#include <string>

enum TokenKind {
    TOK_EOF,
    TOK_IDENTIFIER,
    TOK_NUMBER,         // integer literal
    TOK_FLOATLIT,       // 1.5, 1e3, 1.5f
    TOK_CHARLIT,        // 'A'
    TOK_STRINGLIT,      // "text"

    // --- keywords the C layer needs ---
    TOK_INT,
    TOK_CHAR,
    TOK_VOID,
    TOK_BOOL,
    TOK_SHORT,
    TOK_LONG,
    TOK_SIGNED,
    TOK_UNSIGNED,
    TOK_FLOAT,
    TOK_DOUBLE,
    TOK_CONST,
    TOK_DO,
    TOK_SWITCH,
    TOK_CASE,
    TOK_DEFAULT,
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
    TOK_TRUE,
    TOK_FALSE,

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
    TOK_PLUSPLUS,   // ++
    TOK_MINUSMINUS, // --
    TOK_PLUSEQ,     // +=
    TOK_MINUSEQ,    // -=
    TOK_STAREQ,     // *=
    TOK_SLASHEQ,    // /=
    TOK_PERCENTEQ,  // %=
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

// The position is copied onto AST nodes as they are built, so the semantic
// pass can point at source just as a syntax error does.
struct Token {
    TokenKind kind;
    std::string text;       // identifier spelling, or a string literal's body
    long numberValue;       // integer and character literals
    double floatValue;      // floating literals
    bool isFloatSuffixed;   // 1.5f rather than 1.5
    int line;
    int col;
    Token()
        : kind(TOK_UNKNOWN), numberValue(0), floatValue(0.0),
          isFloatSuffixed(false), line(0), col(0) {}
};

// Human-readable spelling, for "expected X, found Y" messages.
const char *tokenName(TokenKind k);

#endif
