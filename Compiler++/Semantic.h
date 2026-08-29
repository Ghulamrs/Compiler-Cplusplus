// Semantic.h -- PASS 3, semantic analysis.
//
// Walks the one tree the layered parser produced, which mixes cc:: and cxx::
// nodes, so this pass is deliberately not split in two.
//
// Checks: names resolve and nothing is declared twice; reference initialisers
// and assignment targets are lvalues; types match on initialisation,
// assignment, arithmetic, calls and return; access control including protected
// through derivation; overrides, hiding and the virtual-destructor rule;
// initialiser-list membership, duplication and declaration order.
//
// C++98 only.

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

    // The resolved hierarchy, for the layout pass -- one answer to "what is
    // this class's base", with the cycles already broken.
    const std::map<std::string, cxx::ClassDecl*> &classMap() const { return classes; }

private:
    SymbolTable symbols;
    Diagnostics &diag;

    std::map<std::string, cxx::ClassDecl*> classes;

    // Context for the function being analysed.
    cc::Type *currentReturnType;
    std::string currentClass;       // empty outside a method body
    // A ctor/dtor has no return type at all, which is not the same as void.
    bool currentIsCtorOrDtor;
    int loopDepth;                  // break/continue legality

    // Types the analyzer forms itself belong to no AST node, so it owns them
    // and frees them in its destructor.  A formed type must own every node in
    // it -- borrowing a subtree the AST will delete leaves a dangling pointer.
    std::vector<cc::Type*> ownedTypes;
    cc::Type *makeBuiltin(cc::BuiltinKind k);
    cc::Type *cloneType(cc::Type *t);
    cc::Type *makePointerTo(cc::Type *t);

    // passes
    void collectClasses(const std::vector<cc::Decl*> &units);
    // Runs before anything looks a member up: member lookup walks this chain.
    void resolveBases();
    void resolveOverrides(cxx::ClassDecl *cd);
    void analyzeMemberInits(cxx::MethodDecl *ctor, cxx::ClassDecl *cd);
    void checkClassInvariants(cxx::ClassDecl *cd);
    // By argument count.  0 means the class declares no constructors, which is
    // legal and means there is nothing to call.
    cxx::MethodDecl *selectConstructor(cxx::ClassDecl *cd, std::size_t argCount,
                                       cc::ASTNode *at, const std::string &what);
    void recordScopeExitDestruction(cc::CompoundStmt *block,
                                    const std::vector<cc::VarDecl*> &declared);
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
    // Walks the base chain, most derived first -- which IS name hiding.
    // `foundIn` receives the class it was found in, for the diagnostic.
    cc::Decl *findMember(cxx::ClassDecl *cd, const std::string &member,
                         cxx::ClassDecl **foundIn = 0);
    static bool isDerivedFrom(cxx::ClassDecl *derived, cxx::ClassDecl *base);
    // public everywhere, protected inside the class or anything derived from
    // it, private only inside the class itself.
    bool memberIsAccessible(cc::Decl *m, cxx::ClassDecl *owner) const;
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
    // sameType plus the upcasts single inheritance makes free.
    bool canConvert(cc::Type *from, cc::Type *to);
    // canConvert, plus the rule needing the EXPRESSION: literal 0 is the null
    // pointer constant, though its type is int.
    bool convertible(cc::Expr *fromExpr, cc::Type *from, cc::Type *to);
    static bool isNullPointerConstant(cc::Expr *e);
    cxx::ClassDecl *classOf(cc::Type *t);   // through one pointer or reference
    static bool isVoid(cc::Type *t);
    // The builtin kind a type names, or BK_Void when it names none.
    static bool builtinKindOf(cc::Type *t, cc::BuiltinKind &out);
    // Integral promotion: anything of rank below int becomes int.
    static cc::BuiltinKind promote(cc::BuiltinKind k);
    // The usual arithmetic conversions -- the common type two operands meet in.
    static cc::BuiltinKind usualArithmetic(cc::BuiltinKind a, cc::BuiltinKind b);
    // Would converting `from` to `to` lose information?
    static bool isNarrowing(cc::BuiltinKind from, cc::BuiltinKind to);
    // Legal conversions that may lose the value are allowed but reported.
    void warnIfNarrowing(cc::Expr *e, cc::Type *from, cc::Type *to,
                         cc::ASTNode *at, const std::string &what);
    // A literal whose value fits the target is not narrowing -- otherwise
    // `short s = 1;` would warn, and nothing small would ever be assignable.
    static bool literalFitsIn(cc::Expr *e, cc::BuiltinKind to);
    // A stream rather than sprintf: no fixed buffer, and MSVC stays quiet.
    static std::string countText(std::size_t n);
    // false when the type names a class that was never declared -- in which
    // case the caller skips further checks rather than cascading.
    bool checkTypeIsKnown(cc::Type *t, cc::ASTNode *at, const std::string &where);
    void checkCallArgs(cc::CallExpr *call, cc::Function *fn);

    // not copyable (C++98 way: declared private, never defined)
    SemanticAnalyzer(const SemanticAnalyzer &);
    SemanticAnalyzer &operator=(const SemanticAnalyzer &);
};

#endif
