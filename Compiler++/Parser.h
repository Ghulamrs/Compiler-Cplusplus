// Parser.h -- LAYER 1, the C layer's parser, namespace `cc`.
//
// The base of the layered parser.  Every point where C++ extends C is virtual;
// see AST.h for the model and Parser1.h for what the C++ layer does with them.
//
// Errors are reported, never fatal: the parser tells Diagnostics and then
// resynchronises, so one mistake does not hide the next.
//
// C++98 only.

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
    bool expect(TokenKind k, const char *context);  // reports if it does not match
    bool match(TokenKind k);                        // consume if it matches
    void synchronize();         // panic-mode: skip to a plausible restart point

    // --- speculation ------------------------------------------------------
    // Point p; and p.x = 1; both start with an identifier, so parseStatement()
    // tries the declaration rule and rewinds if it does not fit.
    struct State {
        Token cur;
        ::Lexer::Position lexPos;
    };
    State save() const;
    void restore(const State &st);

    // --- declarations -----------------------------------------------------
    // The return type and name are already consumed by the caller.
    Function *parseFunctionRest(Type *retType, const std::string &name);
    // Fills an already-created node, so the C++ layer can pass a MethodDecl --
    // which is a cc::Function -- and have this layer populate it.
    void parseFunctionParamsAndBody(Function *fn);
    // nameLine/nameCol point a diagnostic at the variable, not its type keyword.
    VarDecl *parseVarDeclTail(Type *type, const std::string &name,
                              int nameLine, int nameCol);

    // --- statements -------------------------------------------------------
    CompoundStmt *parseBlock();
    Stmt *parseIf();
    Stmt *parseWhile();
    Stmt *parseFor();
    Stmt *parseReturn();
    Stmt *parseDoWhile();
    Stmt *parseSwitch();
    Stmt *parseExprStatement();

    // --- the precedence chain, defined ONCE, here -------------------------
    Expr *parseExpression();
    Expr *parseAssign();
    Expr *parseLogicalOr();
    Expr *parseLogicalAnd();
    Expr *parseEquality();
    Expr *parseRelational();
    Expr *parseAddSub();
    Expr *parseMulDiv();
    Expr *parseUnary();
    // (T)expr, told from a parenthesised expression by trying the type rule
    // and rewinding when it does not fit.
    Expr *parseCastOrParen();
    Expr *parsePostfix();
    Expr *parseCallSuffix(Expr *callee);
    // a[i] is desugared to *(a + i), which is what it means in C -- so it
    // needs no node, no type rule and no lowering of its own.
    Expr *parseIndexSuffix(Expr *base);

    // C's type grammar:  int   int*   int**
    Type *parsePointerSuffixes(Type *base);
    // In C the array part follows the NAME -- int a[10] -- so it is applied by
    // the declarator, not by parseType.  Dimensions nest inside out:
    // int a[3][4] is 3 arrays of 4 ints.
    Type *parseArraySuffixes(Type *element);

    // --- extension points overridden by the C++ layer ---------------------
    virtual Decl *parseDeclaration();
    virtual Stmt *parseStatement();
    virtual Expr *parsePrimary();
    virtual Type *parseType();
    virtual Expr *parseMemberSuffix(Expr *base);    // 0 if not a member access
    virtual void parseFunctionTail(Function *fn);   // a ctor's initialiser list
    virtual void parseVarInitializer(VarDecl *vd);  // = expr, and C++'s (args)

private:
    // not copyable (C++98 way: declared private, never defined)
    Parser(const Parser &);
    Parser &operator=(const Parser &);
};

} // namespace cc

#endif
