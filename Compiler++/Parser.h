// Parser.h
//
// LAYER 1 -- the C layer's parser, in namespace `cc`.
//
// This class is the BASE of the layered parser.  The members a derived layer
// needs are `protected`, and every grammar point where C++ extends C is
// `virtual`:
//
//     virtual parseDeclaration()   C++ adds class definitions at file scope
//     virtual parseStatement()     extension point for C++-only statements
//     virtual parseType()          C++ adds references T& and class types
//     virtual parsePrimary()       C++ adds  this  and  new T
//     virtual parseMemberSuffix()  C++ adds  a.b  and  p->q
//
// The last one is the interesting one.  parsePostfix() below runs the suffix
// loop -- calls, then member accesses, in any order -- and asks the virtual
// hook whether the current token starts a member access.  In the C layer the
// answer is always no.  That way  f(x).y  and  p.getX()  are parsed by one
// loop shared between the layers rather than by two competing ones.
//
// Errors are REPORTED, never fatal: the parser tells Diagnostics what went
// wrong and then resynchronises, so one mistake does not hide the next.
//
// The authoritative layering model lives at the top of AST.h.
//
// C++98 only  (note: C++98 has no `override` keyword, so a derived layer just
// re-declares the function with the same signature).

#ifndef PARSER_H
#define PARSER_H

#include "Token.h"
#include "AST.h"
#include "Lexer.h"
#include "Diagnostics.h"

#include <cstddef>
#include <string>
#include <vector>

namespace cc {

class Parser {
public:
    Parser(const std::string &s, Diagnostics &d);
    virtual ~Parser();               // virtual: this class is a base

    // The whole file: a list of declarations.  Virtual dispatch inside means a
    // cxx::Parser instance parses classes here too.
    std::vector<Decl*> parseTranslationUnit();

    // Convenience for the C layer on its own: parse a single function.
    Function *parseSingleFunction();

protected:
    void advance();
    Token cur;
    ::Lexer *lexer;
    Diagnostics &diag;

    // --- error reporting and recovery ------------------------------------
    void errorAtCurrent(const std::string &msg);
    // Consumes the token if it matches; otherwise reports and returns false.
    bool expect(TokenKind k, const char *context);
    bool match(TokenKind k);        // consume and return true if it matches
    // Panic-mode recovery: skip tokens until something that plausibly starts a
    // new statement or declaration, so the next error is a real one and not an
    // echo of this one.
    void synchronize();

    // --- speculation ------------------------------------------------------
    // Some statements cannot be told apart from their first token:  Point p;
    // is a declaration and  p.x = 1;  is an expression, yet both start with an
    // identifier.  parseStatement() attempts the declaration rule and rewinds
    // if it does not fit.
    struct State {
        Token cur;
        ::Lexer::Position lexPos;
    };
    State save() const;
    void restore(const State &st);

    // --- declarations -----------------------------------------------------
    // The return type and name are already consumed by the caller.
    Function *parseFunctionRest(Type *retType, const std::string &name);
    // Fills an ALREADY CREATED function node with its parameters and body.
    // Split out from parseFunctionRest so the C++ layer can pass a
    // cxx::MethodDecl -- which is a cc::Function -- and have the C layer
    // populate it without knowing the derived type exists.
    void parseFunctionParamsAndBody(Function *fn);
    // nameLine/nameCol locate the declared name, so a diagnostic about the
    // variable points at the variable and not at its type keyword.
    VarDecl *parseVarDeclTail(Type *type, const std::string &name,
                              int nameLine, int nameCol);

    // --- statements -------------------------------------------------------
    CompoundStmt *parseBlock();
    Stmt *parseIf();
    Stmt *parseWhile();
    Stmt *parseFor();
    Stmt *parseReturn();
    Stmt *parseExprStatement();

    // --- the expression precedence chain, defined ONCE, here --------------
    Expr *parseExpression();
    Expr *parseAssign();
    Expr *parseLogicalOr();
    Expr *parseLogicalAnd();
    Expr *parseEquality();
    Expr *parseRelational();
    Expr *parseAddSub();
    Expr *parseMulDiv();
    Expr *parseUnary();
    Expr *parsePostfix();
    Expr *parseCallSuffix(Expr *callee);

    // C's type grammar:  int   int*   int**
    Type *parsePointerSuffixes(Type *base);

    // --- extension points overridden by the C++ layer ---------------------
    virtual Decl *parseDeclaration();
    virtual Stmt *parseStatement();
    virtual Expr *parsePrimary();
    virtual Type *parseType();
    // Returns 0 when the current token does not start a member access.
    virtual Expr *parseMemberSuffix(Expr *base);
    // Called after a function's ')' and before its ';' or '{'.  C has nothing
    // to put there; C++ has a constructor's initialiser list.
    virtual void parseFunctionTail(Function *fn);
    // Parses whatever follows a declared name:  = expr  in C, and additionally
    // the direct-initialisation form  (args)  in C++.
    virtual void parseVarInitializer(VarDecl *vd);

private:
    // not copyable (C++98 way: declared private, never defined)
    Parser(const Parser &);
    Parser &operator=(const Parser &);
};

} // namespace cc

#endif
