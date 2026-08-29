//
//  Semantic.h
//  Compiler++
//
//  Created by G. R. Akhtar on 29/08/2026.
//
//  PASS 3 -- semantic analysis, run after the layered parser has produced a
//  tree.  Like the parser, it works across BOTH class layers at once:
//
//      cc::   LAYER 1, the C layer   -- functions, statements, expressions
//      cxx::  LAYER 2, the C++ layer -- classes, fields, methods, members
//
//  The analyzer is deliberately NOT split in two: a single tree mixes cc::
//  and cxx:: nodes (see the layering model at the top of AST.h), so one walk
//  resolves both.  Node kinds are told apart with dynamic_cast, which works
//  across the layers because every node derives from cc::ASTNode.
//
//  What it checks today:
//    * every name resolves, and nothing is declared twice in one scope
//    * a reference initialiser is an lvalue, and an assignment target is one
//    * types match on initialisation, assignment and arithmetic
//    * calls match their function's parameter count and types
//    * a return expression matches the enclosing function's return type
//    * a private member is not touched from outside its class
//
//  C++98 only.

#ifndef SEMANTIC_H
#define SEMANTIC_H

#include <map>
#include <string>
#include <vector>

#include "AST.h"       // LAYER 1 nodes
#include "AST1.h"      // LAYER 2 nodes
#include "Diagnostics.h"
#include "SymbolTable.h"

class SemanticAnalyzer {
public:
    explicit SemanticAnalyzer(Diagnostics &d);
    ~SemanticAnalyzer();

    // entry point: a whole translation unit
    void analyze(const std::vector<cc::Decl*> &units);

    // The resolved hierarchy, for the layout pass that runs next.  Handing it
    // over rather than recomputing it means there is one answer to "what is
    // this class's base", and the layout pass inherits the cycle-breaking the
    // semantic pass already did.
    const std::map<std::string, cxx::ClassDecl*> &classMap() const { return classes; }

private:
    SymbolTable symbols;
    Diagnostics &diag;

    // Every class by name, so a member access can find the class a value's
    // type names.  When single inheritance lands, member lookup becomes a walk
    // up this map from a class to its base.
    std::map<std::string, cxx::ClassDecl*> classes;

    // Context for the function currently being analysed.
    cc::Type *currentReturnType;
    std::string currentClass;       // empty outside a method body
    // A constructor and a destructor have no return type at all, which is not
    // the same as returning void: `return;` is fine, `return x;` is not.
    bool currentIsCtorOrDtor;
    int loopDepth;                  // break/continue legality

    // Types the analyzer creates for expression results.  They belong to no
    // AST node, so the analyzer owns them and frees them in its destructor.
    std::vector<cc::Type*> ownedTypes;
    cc::Type *makeBuiltin(const std::string &name);
    // A deep copy, registered for cleanup.  Needed whenever the analyzer forms
    // a NEW type out of an existing one -- &x is a pointer to x's type, and
    // that pointer type cannot borrow a subtree the AST will delete.
    cc::Type *cloneType(cc::Type *t);
    cc::Type *makePointerTo(cc::Type *t);

    // passes
    void collectClasses(const std::vector<cc::Decl*> &units);
    // Links each class to its base and rejects unknown bases and cycles.
    // Runs before anything looks a member up, because member lookup walks the
    // chain this pass builds.
    void resolveBases();
    // Marks a method that matches a base virtual as an override.
    void resolveOverrides(cxx::ClassDecl *cd);
    // A constructor's  : x(1), Base(2)  list.
    void analyzeMemberInits(cxx::MethodDecl *ctor, cxx::ClassDecl *cd);
    // Rules that concern a class as a whole rather than one member.
    void checkClassInvariants(cxx::ClassDecl *cd);
    // Chooses the constructor taking argCount arguments; reports if there is
    // none, or more than one.  0 means "the class declares no constructors",
    // which is legal and means there is nothing to call.
    cxx::MethodDecl *selectConstructor(cxx::ClassDecl *cd, std::size_t argCount,
                                       cc::ASTNode *at, const std::string &what);
    // Records, on each block, the locals whose destructors must run on exit.
    void recordScopeExitDestruction(cc::CompoundStmt *block,
                                    const std::vector<cc::VarDecl*> &declared);
    // Does an object of this type need a destructor call when it dies?
    bool hasDestructor(cc::Type *t);
    void declareTopLevel(const std::vector<cc::Decl*> &units);
    void analyzeDecl(cc::Decl *d);
    void analyzeClass(cxx::ClassDecl *cd);
    void analyzeFunction(cc::Function *fn);
    void analyzeStmt(cc::Stmt *s);
    void analyzeBlock(cc::CompoundStmt *block);
    void analyzeVarDecl(cc::VarDecl *vd, bool declareIt);
    cc::Type *analyzeExpr(cc::Expr *e, bool &isLValue);

    // member lookup
    cxx::ClassDecl *findClass(const std::string &name);
    // Walks the base chain: the most derived class wins, which IS name hiding.
    // `foundIn` receives the class the member was actually found in, so an
    // access diagnostic can name the right class.
    cc::Decl *findMember(cxx::ClassDecl *cd, const std::string &member,
                         cxx::ClassDecl **foundIn = 0);
    // Is `derived` the same class as `base`, or below it in the chain?
    static bool isDerivedFrom(cxx::ClassDecl *derived, cxx::ClassDecl *base);
    // The access rule, in one place: public everywhere, protected inside the
    // class or anything derived from it, private only inside the class itself.
    bool memberIsAccessible(cc::Decl *m, cxx::ClassDecl *owner) const;
    // The class a member belongs to, from the member itself.
    cxx::ClassDecl *ownerClassOf(cc::Decl *m);
    static bool sameSignature(cc::Function *a, cc::Function *b);
    static cxx::Access memberAccess(cc::Decl *m);
    static cc::Type *memberType(cc::Decl *m);
    // Pushes a scope holding cd's members, so a method body sees them unqualified.
    void pushClassScope(cxx::ClassDecl *cd);

    // helpers
    void error(cc::ASTNode *at, const std::string &msg);
    static cc::Type *stripReference(cc::Type *t);
    static std::string describe(cc::Type *t);
    static bool sameType(cc::Type *a, cc::Type *b);
    // Implicit conversion, which is sameType plus the upcasts single
    // inheritance makes free:  Derived* -> Base*  and  Derived -> Base&.
    bool canConvert(cc::Type *from, cc::Type *to);
    // canConvert, plus the one rule that needs the EXPRESSION and not just its
    // type: the literal 0 is the null pointer constant, so it initialises any
    // pointer even though its type is int.
    bool convertible(cc::Expr *fromExpr, cc::Type *from, cc::Type *to);
    static bool isNullPointerConstant(cc::Expr *e);
    // The class a type names, looking through one pointer or reference.
    cxx::ClassDecl *classOf(cc::Type *t);
    static bool isVoid(cc::Type *t);
    // std::size_t as text.  A stream rather than sprintf, so the code carries
    // no fixed-size buffer and MSVC raises no deprecation warning.
    static std::string countText(std::size_t n);
    void checkTypeIsKnown(cc::Type *t, cc::ASTNode *at, const std::string &where);
    void checkCallArgs(cc::CallExpr *call, cc::Function *fn);

    // not copyable (C++98 way: declared private, never defined)
    SemanticAnalyzer(const SemanticAnalyzer &);
    SemanticAnalyzer &operator=(const SemanticAnalyzer &);
};

#endif
