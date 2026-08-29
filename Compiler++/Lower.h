// Lower.h -- PASS 5b, LAYER 1: lowering the C layer, namespace `cc`.
//
// Walks the parts of the tree C already had and turns them into IR.  Where a
// construct might be a C++ one it asks a virtual hook, which here answers "not
// mine"; cxx::Lowering (Lower1.h) answers them.
//
// The pass turns on one distinction: ADDRESS versus VALUE.  An lvalue has an
// address and its value is a load from it; a non-lvalue has only a value.
// Assignment lowers its left side as an address and its right as a value.
// This is also what makes references disappear -- a reference variable holds
// an address, so lowering its address is a load, not a slot lookup.
//
// C++98 only.

#ifndef LOWER_H
#define LOWER_H

#include <map>
#include <string>
#include <vector>

#include "AST.h"
#include "Diagnostics.h"
#include "IR.h"
#include "Layout.h"

namespace cc {

class Lowering {
public:
    Lowering(IRModule &module, const Layout &layout, Diagnostics &diag);
    virtual ~Lowering();

    // Lowers a whole translation unit.
    void lowerUnit(const std::vector<Decl*> &units);

protected:
    IRModule &mod;
    const Layout &layout;
    Diagnostics &diag;

    IRFunction *fn;                     // the function being built, or 0
    std::map<std::string, int> slots;   // name -> frame slot, innermost wins
    std::vector<std::string> scopeNames;// names added, for unwinding a scope
    std::vector<int> scopeMarks;
    // Labels to jump to for `break` and `continue`, innermost last.
    std::vector<int> breakTargets;
    std::vector<int> continueTargets;
    // Innermost last.  A `return` inside nested scopes runs the destructors of
    // every one of them, walking this list.
    std::vector<CompoundStmt*> openBlocks;

    // --- declarations -------------------------------------------------
    virtual void lowerDecl(Decl *d);
    void lowerFunction(Function *f, const std::string &mangled,
                       const std::string &sourceName, bool hasThis);
    virtual void emitPrologue(Function *f);     // a ctor's base call and inits
    virtual void emitEpilogue(Function *f);     // a dtor's tail

    // --- statements ---------------------------------------------------
    virtual void lowerStmt(Stmt *s);
    void lowerBlock(CompoundStmt *block);
    void lowerIf(IfStmt *s);
    void lowerWhile(WhileStmt *s);
    void lowerFor(ForStmt *s);
    virtual void lowerVarDecl(VarDecl *vd);
    // Nothing in the C layer has a destructor, so these do nothing here.
    virtual void emitScopeExit(CompoundStmt *block);
    virtual void emitAllOpenScopeExits();

    // --- expressions --------------------------------------------------
    IRReg lowerValue(Expr *e);
    IRReg lowerAddress(Expr *e);
    IRReg lowerBinary(BinaryExpr *e);
    // Emits whatever machine operation the conversion needs, or nothing when
    // the two types already agree.  Every implicit conversion the semantic
    // pass allowed becomes a real instruction here.
    IRReg convert(IRReg value, Type *from, Type *to, int line);
    // The type an expression's operands meet in, so both sides are converted
    // before the operator runs.
    static bool arithKind(Type *t, BuiltinKind &out);
    static BuiltinKind commonKind(BuiltinKind a, BuiltinKind b);
    Type *literalType(BuiltinKind k);
    Type *commonType(BuiltinKind k);
    // Builtin types this pass forms, one per kind, owned here.
    std::map<int, Type*> builtinCache;
    IRReg lowerUnary(UnaryExpr *e);
    IRReg lowerAssign(BinaryExpr *e);
    IRReg lowerShortCircuit(BinaryExpr *e);
    virtual IRReg lowerCall(CallExpr *e, bool wantsResult);

    // --- hooks the C++ layer answers ----------------------------------
    // false when the node is not one this layer handles -- always, here.
    virtual bool lowerLayerValue(Expr *e, IRReg &out);
    virtual bool lowerLayerAddress(Expr *e, IRReg &out);

    // --- helpers ------------------------------------------------------
    int sizeOfType(Type *t) const;
    // Lowering needs sizes, not meanings, so this is a small local recompute
    // rather than a second type checker.
    virtual Type *typeOf(Expr *e);
    void pushScope();
    void popScope();
    int declareLocal(const std::string &name, int size, bool isParam);
    int findSlot(const std::string &name) const;
    virtual bool isReferenceExpr(Expr *e);  // its slot holds an address
    std::map<std::string, Type*> localTypes;
    std::map<std::string, Type*> globalTypes;

private:
    Lowering(const Lowering &);
    Lowering &operator=(const Lowering &);
};

} // namespace cc

#endif
