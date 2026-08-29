// Parser.h
//
// LAYER 1 -- the C layer's parser, in namespace `cc`.
//
// This class is the BASE of the layered parser.  cxx::Parser (Parser1.h)
// derives from it, so the members a derived layer needs are `protected` and
// the grammar points where C++ extends C are `virtual`:
//
//     virtual parsePrimary()    C++ adds member access:  a.b   a->b
//     virtual parseType()       C++ adds references T& and class types
//     virtual parseStatement()  extension point for C++-only statements
//
// The authoritative layering model lives at the top of AST.h.
//
// C++98 only  (note: C++98 has no `override` keyword, so a derived layer just
// re-declares the function with the same signature).

#ifndef PARSER_H
#define PARSER_H

#include "Token.h"
#include "AST.h"
#include <cstddef>
#include <string>
#include <vector>

// The lexer is shared by every layer and lives at global scope.
class Lexer;

namespace cc {

class Parser {
public:
    Parser(const std::string &s);
    virtual ~Parser();               // virtual: this class is a base
    Function *parse();

protected:                           // was private: the C++ layer builds on these
    void advance();
    Token cur;
    ::Lexer *lexer;                  // was an untyped void*

    // --- speculation ----------------------------------------------------
    // Some statements cannot be told apart from their first token:  Point p;
    // is a declaration and  p.x = 1;  is an expression, yet both start with an
    // identifier.  parseStatement() attempts the declaration rule and rewinds
    // if it does not fit.  A State captures the lookahead token together with
    // the lexer's position, which is all the parser's state.
    struct State {
        Token cur;
        std::size_t lexPos;
    };
    State save() const;
    void restore(const State &st);

    // parsing helpers
    Function *parseFunction();
    // Parses '{' statement* '}' into out.  Shared by the C layer's functions
    // and the C++ layer's function and method bodies.
    void parseBlock(std::vector<Stmt*> &out);
    // The type has already been consumed by the caller (it had to be, to know
    // this was a declaration at all), so it is passed in.
    DeclStmt *parseDeclTail(Type *type);
    ReturnStmt *parseReturn();
    ExprStmt *parseExprStatement();

    // shared expression chain -- defined ONCE, here, for every layer
    Expr *parseExpression();
    Expr *parseAssign();
    Expr *parseAddSub();
    Expr *parseMulDiv();

    // C's type grammar:  int   int*   int**
    // Shared with the C++ layer, which applies the same * suffixes to its
    // own class types before adding a reference suffix.
    Type *parsePointerSuffixes(Type *base);

    // extension points overridden by the C++ layer
    virtual Stmt *parseStatement();
    virtual Expr *parsePrimary();
    virtual Type *parseType();

private:
    // not copyable (C++98 way: declared private, never defined)
    Parser(const Parser &);
    Parser &operator=(const Parser &);
};

} // namespace cc

#endif
