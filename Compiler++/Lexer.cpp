// Lexer.cpp
//
// C++98 only.

#include "Lexer.h"

#include <cctype>
#include <cstdlib>
#include <string>

// --- token spelling, for "expected X" messages -------------------------
const char *tokenName(TokenKind k) {
    switch (k) {
    case TOK_EOF:        return "end of file";
    case TOK_IDENTIFIER: return "identifier";
    case TOK_NUMBER:     return "number";
    case TOK_FLOATLIT:   return "floating literal";
    case TOK_CHARLIT:    return "character literal";
    case TOK_STRINGLIT:  return "string literal";
    case TOK_INT:        return "int";
    case TOK_CHAR:       return "char";
    case TOK_VOID:       return "void";
    case TOK_SHORT:      return "short";
    case TOK_LONG:       return "long";
    case TOK_SIGNED:     return "signed";
    case TOK_UNSIGNED:   return "unsigned";
    case TOK_FLOAT:      return "float";
    case TOK_DOUBLE:     return "double";
    case TOK_CONST:      return "const";
    case TOK_RETURN:     return "return";
    case TOK_IF:         return "if";
    case TOK_ELSE:       return "else";
    case TOK_WHILE:      return "while";
    case TOK_FOR:        return "for";
    case TOK_BREAK:      return "break";
    case TOK_CONTINUE:   return "continue";
    case TOK_CLASS:      return "class";
    case TOK_STRUCT:     return "struct";
    case TOK_PUBLIC:     return "public";
    case TOK_PRIVATE:    return "private";
    case TOK_PROTECTED:  return "protected";
    case TOK_VIRTUAL:    return "virtual";
    case TOK_NEW:        return "new";
    case TOK_DELETE:     return "delete";
    case TOK_THIS:       return "this";
    case TOK_SEMI:       return "';'";
    case TOK_LPAREN:     return "'('";
    case TOK_RPAREN:     return "')'";
    case TOK_LBRACE:     return "'{'";
    case TOK_RBRACE:     return "'}'";
    case TOK_LBRACKET:   return "'['";
    case TOK_RBRACKET:   return "']'";
    case TOK_COMMA:      return "','";
    case TOK_COLON:      return "':'";
    case TOK_COLONCOLON: return "'::'";
    case TOK_DOT:        return "'.'";
    case TOK_ARROW:      return "'->'";
    case TOK_TILDE:      return "'~'";
    case TOK_PLUS:       return "'+'";
    case TOK_MINUS:      return "'-'";
    case TOK_STAR:       return "'*'";
    case TOK_SLASH:      return "'/'";
    case TOK_PERCENT:    return "'%'";
    case TOK_ASSIGN:     return "'='";
    case TOK_EQ:         return "'=='";
    case TOK_NE:         return "'!='";
    case TOK_LT:         return "'<'";
    case TOK_GT:         return "'>'";
    case TOK_LE:         return "'<='";
    case TOK_GE:         return "'>='";
    case TOK_ANDAND:     return "'&&'";
    case TOK_OROR:       return "'||'";
    case TOK_NOT:        return "'!'";
    case TOK_AMP:        return "'&'";
    case TOK_UNKNOWN:    return "unknown token";
    }
    return "token";
}

// One place for the escapes both literal forms share.
static char decodeEscape(char c) {
    switch (c) {
    case 'n':  return '\n';
    case 't':  return '\t';
    case 'r':  return '\r';
    case '0':  return '\0';
    case '\\': return '\\';
    case '\'': return '\'';
    case '"':  return '"';
    default:   return c;
    }
}

// --- position bookkeeping ---------------------------------------------

char Lexer::get() {
    if (pos >= src.size()) return '\0';
    char c = src[pos++];
    if (c == '\n') { ++line; col = 1; }
    else           { ++col; }
    return c;
}

Lexer::Position Lexer::tell() const {
    Position p;
    p.offset = pos;
    p.line = line;
    p.col = col;
    return p;
}

void Lexer::seek(const Position &p) {
    pos = p.offset;
    line = p.line;
    col = p.col;
}

void Lexer::skipWhitespaceAndComments() {
    for (;;) {
        while (std::isspace(static_cast<unsigned char>(peek()))) get();
        if (peek() == '/' && peekAt(1) == '/') {
            while (peek() != '\0' && peek() != '\n') get();
            continue;
        }
        if (peek() == '/' && peekAt(1) == '*') {
            get(); get();                       // consume the opening /*
            while (peek() != '\0') {
                if (peek() == '*' && peekAt(1) == '/') {
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

Token Lexer::makeToken(TokenKind k, int startLine, int startCol) {
    Token t;
    t.kind = k;
    t.line = startLine;
    t.col = startCol;
    return t;
}

// --- the scanner ------------------------------------------------------

Token Lexer::nextToken() {
    skipWhitespaceAndComments();

    const int startLine = line;
    const int startCol = col;

    char c = peek();
    if (c == '\0') return makeToken(TOK_EOF, startLine, startCol);

    // identifier or keyword
    if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
        std::string id;
        while (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_') id += get();
        Token tok = makeToken(TOK_IDENTIFIER, startLine, startCol);
        if      (id == "int")       tok.kind = TOK_INT;
        else if (id == "char")      tok.kind = TOK_CHAR;
        else if (id == "void")      tok.kind = TOK_VOID;
        else if (id == "short")     tok.kind = TOK_SHORT;
        else if (id == "long")      tok.kind = TOK_LONG;
        else if (id == "signed")    tok.kind = TOK_SIGNED;
        else if (id == "unsigned")  tok.kind = TOK_UNSIGNED;
        else if (id == "float")     tok.kind = TOK_FLOAT;
        else if (id == "double")    tok.kind = TOK_DOUBLE;
        else if (id == "const")     tok.kind = TOK_CONST;
        else if (id == "return")    tok.kind = TOK_RETURN;
        else if (id == "if")        tok.kind = TOK_IF;
        else if (id == "else")      tok.kind = TOK_ELSE;
        else if (id == "while")     tok.kind = TOK_WHILE;
        else if (id == "for")       tok.kind = TOK_FOR;
        else if (id == "break")     tok.kind = TOK_BREAK;
        else if (id == "continue")  tok.kind = TOK_CONTINUE;
        else if (id == "class")     tok.kind = TOK_CLASS;
        else if (id == "struct")    tok.kind = TOK_STRUCT;
        else if (id == "public")    tok.kind = TOK_PUBLIC;
        else if (id == "private")   tok.kind = TOK_PRIVATE;
        else if (id == "protected") tok.kind = TOK_PROTECTED;
        else if (id == "virtual")   tok.kind = TOK_VIRTUAL;
        else if (id == "new")       tok.kind = TOK_NEW;
        else if (id == "delete")    tok.kind = TOK_DELETE;
        else if (id == "this")      tok.kind = TOK_THIS;
        else                        tok.text = id;
        return tok;
    }

    // A number is integer until a '.' or an exponent proves otherwise.  The
    // '.' is only part of the number when a digit follows, so that  p.x  still
    // lexes as three tokens.
    if (std::isdigit(static_cast<unsigned char>(c))) {
        std::string num;
        while (std::isdigit(static_cast<unsigned char>(peek()))) num += get();

        bool isFloat = false;
        if (peek() == '.' && std::isdigit(static_cast<unsigned char>(peekAt(1)))) {
            isFloat = true;
            num += get();                                   // the '.'
            while (std::isdigit(static_cast<unsigned char>(peek()))) num += get();
        }
        if (peek() == 'e' || peek() == 'E') {
            const char sign = peekAt(1);
            const bool signedExp = (sign == '+' || sign == '-');
            if (std::isdigit(static_cast<unsigned char>(signedExp ? peekAt(2) : sign))) {
                isFloat = true;
                num += get();                               // 'e'
                if (signedExp) num += get();
                while (std::isdigit(static_cast<unsigned char>(peek()))) num += get();
            }
        }

        if (isFloat) {
            Token tok = makeToken(TOK_FLOATLIT, startLine, startCol);
            tok.text = num;
            tok.floatValue = std::atof(num.c_str());
            if (peek() == 'f' || peek() == 'F') { get(); tok.isFloatSuffixed = true; }
            return tok;
        }
        Token tok = makeToken(TOK_NUMBER, startLine, startCol);
        tok.text = num;
        tok.numberValue = std::atol(num.c_str());
        return tok;
    }

    // 'A' and "text".  Escapes are shared, so they are decoded in one place.
    if (c == '\'' || c == '"') {
        const char quote = get();
        std::string body;
        bool closed = false;
        while (peek() != '\0') {
            if (peek() == quote) { get(); closed = true; break; }
            if (peek() == '\n') break;                       // unterminated
            char ch = get();
            if (ch == '\\') ch = decodeEscape(get());
            body += ch;
        }
        Token tok = makeToken(quote == '\'' ? TOK_CHARLIT : TOK_STRINGLIT,
                              startLine, startCol);
        if (!closed) {
            tok.kind = TOK_UNKNOWN;
            tok.text = "unterminated literal";
            return tok;
        }
        if (quote == '"') { tok.text = body; return tok; }
        tok.numberValue = body.empty() ? 0 : static_cast<unsigned char>(body[0]);
        tok.text = body;
        return tok;
    }

    // punctuation and operators.  Two-character forms are tested before the
    // one-character form they start with, or  ==  would lex as  = =.
    char punct = get();
    TokenKind kind = TOK_UNKNOWN;
    switch (punct) {
    case ';': kind = TOK_SEMI; break;
    case '(': kind = TOK_LPAREN; break;
    case ')': kind = TOK_RPAREN; break;
    case '{': kind = TOK_LBRACE; break;
    case '}': kind = TOK_RBRACE; break;
    case '[': kind = TOK_LBRACKET; break;
    case ']': kind = TOK_RBRACKET; break;
    case ',': kind = TOK_COMMA; break;
    case '~': kind = TOK_TILDE; break;
    case '+': kind = TOK_PLUS; break;
    case '*': kind = TOK_STAR; break;
    case '/': kind = TOK_SLASH; break;
    case '%': kind = TOK_PERCENT; break;
    case '.': kind = TOK_DOT; break;
    case '-':
        if (peek() == '>') { get(); kind = TOK_ARROW; }
        else kind = TOK_MINUS;
        break;
    case '=':
        if (peek() == '=') { get(); kind = TOK_EQ; }
        else kind = TOK_ASSIGN;
        break;
    case '!':
        if (peek() == '=') { get(); kind = TOK_NE; }
        else kind = TOK_NOT;
        break;
    case '<':
        if (peek() == '=') { get(); kind = TOK_LE; }
        else kind = TOK_LT;
        break;
    case '>':
        if (peek() == '=') { get(); kind = TOK_GE; }
        else kind = TOK_GT;
        break;
    case '&':
        if (peek() == '&') { get(); kind = TOK_ANDAND; }
        else kind = TOK_AMP;
        break;
    case '|':
        if (peek() == '|') { get(); kind = TOK_OROR; }
        else kind = TOK_UNKNOWN;
        break;
    case ':':
        if (peek() == ':') { get(); kind = TOK_COLONCOLON; }
        else kind = TOK_COLON;
        break;
    default:
        kind = TOK_UNKNOWN;
        break;
    }

    Token tok = makeToken(kind, startLine, startCol);
    if (kind == TOK_UNKNOWN) tok.text = std::string(1, punct);
    return tok;
}

// A factory, so the parsers never need the lexer's definition in their headers.
Lexer *createLexer(const std::string &s) {
    return new Lexer(s);
}
