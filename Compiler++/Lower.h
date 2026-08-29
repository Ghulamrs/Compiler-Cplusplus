// Lower.h
//
// PASS 5b, LAYER 1 -- lowering the C layer, in namespace `cc`.
//
// This is the last place the two-layer design does its trick, and the place it
// does the most work.  cc::Lowering walks the parts of the tree C already had
// -- arithmetic, control flow, locals, calls -- and turns them into IR.  Where
// a construct might be a C++ one it asks a VIRTUAL hook, which in this layer
// answers "not mine".  cxx::Lowering (Lower1.h) derives from this class and
// answers those hooks with member access, `this`, virtual dispatch, `new` and
// `delete`.
//
// The hooks are shaped around a single idea that runs through the whole pass:
//
//     ADDRESS versus VALUE.
//
// Every expression can be asked for its address (lowerAddress) or its value
// (lowerValue).  An lvalue -- a variable, a field, *p -- has an address, and
// its value is a load from it.  A non-lvalue -- 1, a + b, a call -- has only a
// value.  Assignment lowers its left side as an address and its right as a
// value.  That one distinction is what the semantic pass's isLValue result was
// for, and it is what makes references disappear: a reference variable holds an
// address, so lowering its ADDRESS is a load, not a local-slot lookup.
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
    virtual ~Lowering() {}

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
    // Blocks currently being lowered, innermost last.  A `return` in the
    // middle of nested scopes has to run the destructors of every one of them,
    // and this is the list it walks.
    std::vector<CompoundStmt*> openBlocks;

    // --- declarations -------------------------------------------------
    virtual void lowerDecl(Decl *d);
    void lowerFunction(Function *f, const std::string &mangled,
                       const std::string &sourceName, bool hasThis);
    // Called after the parameters are in place and before the body, so a
    // constructor can emit its base call, vptr store and member initialisers.
    virtual void emitPrologue(Function *f);
    // Called before every `return` and at the end of the body.
    virtual void emitEpilogue(Function *f);

    // --- statements ---------------------------------------------------
    virtual void lowerStmt(Stmt *s);
    void lowerBlock(CompoundStmt *block);
    void lowerIf(IfStmt *s);
    void lowerWhile(WhileStmt *s);
    void lowerFor(ForStmt *s);
    virtual void lowerVarDecl(VarDecl *vd);
    // Destructor calls for the locals a block owns.  Nothing in the C layer
    // has a destructor, so this does nothing here.
    virtual void emitScopeExit(CompoundStmt *block);
    // Destructors for every block still open, innermost first -- what a
    // `return` in the middle of nested scopes has to run.
    virtual void emitAllOpenScopeExits();

    // --- expressions --------------------------------------------------
    // The two halves of the address/value distinction.
    IRReg lowerValue(Expr *e);
    IRReg lowerAddress(Expr *e);
    IRReg lowerBinary(BinaryExpr *e);
    IRReg lowerUnary(UnaryExpr *e);
    IRReg lowerAssign(BinaryExpr *e);
    IRReg lowerShortCircuit(BinaryExpr *e);
    virtual IRReg lowerCall(CallExpr *e, bool wantsResult);

    // --- hooks the C++ layer answers ----------------------------------
    // Each returns false when the node is not one this layer handles, which is
    // always the case in the C layer.  `out` receives the register on success.
    virtual bool lowerLayerValue(Expr *e, IRReg &out);
    virtual bool lowerLayerAddress(Expr *e, IRReg &out);

    // --- helpers ------------------------------------------------------
    int sizeOfType(Type *t) const;
    // The declared type of an expression, as the semantic pass would give it.
    // Lowering needs sizes, not meanings, so this is a small local recompute
    // rather than a second full type checker.
    virtual Type *typeOf(Expr *e);
    void pushScope();
    void popScope();
    int declareLocal(const std::string &name, int size, bool isParam);
    int findSlot(const std::string &name) const;
    // Is this expression a reference variable?  Then its slot holds an address.
    virtual bool isReferenceExpr(Expr *e);

    // Records the type the semantic pass gave each declaration, so lowering can
    // ask for a size without redoing analysis.
    std::map<std::string, Type*> localTypes;
    std::map<std::string, Type*> globalTypes;

private:
    Lowering(const Lowering &);
    Lowering &operator=(const Lowering &);
};

} // namespace cc

#endif
