// Parser1.h -- LAYER 2, the C++ layer's parser, namespace `cxx`.
//
// Derives from cc::Parser.  The expression chain, control flow and function
// bodies are inherited, not copied; this class adds classes, base clauses,
// constructors, initialiser lists, and the C++ forms of type, primary and
// postfix expression.
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
    // int Point::getX() { ... } at file scope: the body of a method declared
    // inside the class.  Returns 0 when the declaration is not qualified.
    Decl *parseOutOfLineDefinition();
    Decl *parseMemberDecl(const std::string &className, Access access);
    QualifiedName *parseQualifiedName();
    // Is the token stream sitting on  ClassName (  -- i.e. a constructor?
    // Needs two tokens of lookahead, which the inherited save()/restore()
    // rewind provides.
    bool looksLikeConstructor(const std::string &className);

    // extension points taken over from the C layer.
    // C++98 has no `override` keyword -- each signature must match
    // cc::Parser's exactly or this would silently hide instead of override.
    virtual cc::Decl *parseDeclaration();
    virtual cc::Expr *parsePrimary();
    virtual cc::Expr *parseMemberSuffix(cc::Expr *base);
    virtual cc::Type *parseType();
    virtual void parseFunctionTail(cc::Function *fn);
    virtual void parseVarInitializer(cc::VarDecl *vd);
};

} // namespace cxx

#endif
