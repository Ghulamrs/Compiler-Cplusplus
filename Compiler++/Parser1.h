// Parser1.h
//
// LAYER 2 -- the C++ layer's parser, in namespace `cxx`.
//
// It DERIVES from cc::Parser.  Everything C already knew how to parse -- the
// expression precedence chain, control flow, function bodies, declarations
// with initialisers -- is inherited, not copied.  This class adds only what
// C++ adds to C:
//
//     parseDeclaration()    OVERRIDE: adds class/struct definitions, then
//                           defers to the C layer for everything else
//     parseClass()          class body + access specifiers
//     parseMemberDecl()     fields and methods, tagged with their access
//     parseType()           OVERRIDE: extends C's int/int* with qualified
//                           names A::B and references T&
//     parsePrimary()        OVERRIDE: adds  this  and  new T
//     parseMemberSuffix()   OVERRIDE: adds  a.b  and  p->q  to the inherited
//                           postfix loop
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
    Parser(const std::string &s, Diagnostics &d);

private:
    // new in the C++ layer
    ClassDecl *parseClass();
    Decl *parseMemberDecl(const std::string &className, Access access);
    QualifiedName *parseQualifiedName();

    // extension points taken over from the C layer.
    // C++98 has no `override` keyword -- each signature must match
    // cc::Parser's exactly or this would silently hide instead of override.
    virtual cc::Decl *parseDeclaration();
    virtual cc::Expr *parsePrimary();
    virtual cc::Expr *parseMemberSuffix(cc::Expr *base);
    virtual cc::Type *parseType();
};

} // namespace cxx

#endif
