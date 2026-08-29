// Parser1.h
//
// LAYER 2 -- the C++ layer's parser, in namespace `cxx`.
//
// It DERIVES from cc::Parser (Parser.h).  Everything C already knew how to
// parse -- the expression precedence chain, statements, declarations with
// initialisers -- is inherited, not copied.  This class adds only what C++
// adds to C:
//
//     parseTranslationUnit()  class/struct definitions at file scope
//     parseClass()            class body + access specifiers
//     parseFunctionRest()     parameter lists and bodies, for members and for
//                             free functions -- the body itself is parsed by
//                             the INHERITED cc::Parser::parseBlock()
//     parseType()             OVERRIDE: extends C's  int / int*  with
//                             qualified names A::B and references T&
//     parsePrimary()          OVERRIDE: extends C's primary with  a.b  a->b
//
// The authoritative layering model lives at the top of AST.h.
//
// C++98 only.

#ifndef PARSER1_H
#define PARSER1_H

#include "Token.h"
#include "AST1.h"
#include "Parser.h"
#include <string>
#include <vector>

namespace cxx {

class Parser : public cc::Parser {
public:
    Parser(const std::string &s);
    std::vector<Decl*> parseTranslationUnit();

private:
    // new in the C++ layer
    Decl *parseDeclaration();
    ClassDecl *parseClass();
    Decl *parseMemberDecl();
    QualifiedName *parseQualifiedName();
    // The return type and name are already consumed; parses '(' params ')'
    // followed by either ';' or a body.  Used for class members and for
    // free functions, which differ only in where they appear.
    MethodDecl *parseFunctionRest(Type *retType, const std::string &name);

    // extension points taken over from the C layer.
    // C++98 has no `override` keyword -- each signature must match
    // cc::Parser's exactly or this would silently hide instead of override.
    virtual cc::Expr *parsePrimary();
    virtual cc::Type *parseType();
};

} // namespace cxx

#endif
