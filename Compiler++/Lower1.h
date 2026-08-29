// Lower1.h
//
// PASS 5b, LAYER 2 -- lowering the C++ layer, in namespace `cxx`.
//
// It DERIVES from cc::Lowering, and it is the pass that makes the project's
// central claim literal.  Everything below is a C++ construct being written
// out in terms C already had:
//
//     a method            ->  a function whose first parameter is `this`
//     a reference T&      ->  a pointer, with one more load on every use
//     obj.field           ->  an address plus a constant offset
//     p->method()         ->  load vptr, index by a constant slot, call it
//     new T(args)         ->  alloc(sizeof T), then call the constructor
//     delete p            ->  call the destructor, then free
//     a constructor       ->  base ctor, store vptr, member inits, body
//     a destructor        ->  body, members in reverse, base dtor
//     a local going out
//       of scope          ->  a destructor call at every exit from its block
//
// After this pass there is nothing left for a code generator to know about
// C++.  That is why IR.h has no second layer.
//
// C++98 only.

#ifndef LOWER1_H
#define LOWER1_H

#include <string>
#include <vector>

#include "AST1.h"
#include "Lower.h"

namespace cxx {

class Lowering : public cc::Lowering {
public:
    Lowering(IRModule &module, const Layout &layout, Diagnostics &diag,
             const std::map<std::string, ClassDecl*> &classes);

    // Emits every class's vtable as module data, then the code.
    void lowerClasses();
    ~Lowering();

private:
    const std::map<std::string, ClassDecl*> &classes;
    // The class whose method is being lowered, so `this` and an unqualified
    // member name can be resolved; empty outside a method.
    std::string currentClass;

    ClassDecl *findClass(const std::string &name) const;
    // Types the lowering pass forms itself -- the type of `this`, the type of
    // a `new` expression.  They belong to no AST node, so this class owns them
    // and frees them in its destructor.
    std::vector<cc::Type*> ownedTypes;
    cc::Type *makePointerToClass(const std::string &className);
    cc::Type *cloneType(cc::Type *t);
    // The class a type names, through at most one pointer or reference.
    ClassDecl *classOfType(cc::Type *t) const;
    const FieldLayout *findField(const std::string &className,
                                 const std::string &member) const;
    MethodDecl *findMethod(ClassDecl *cd, const std::string &member) const;
    int vtableSlotOf(const std::string &className, MethodDecl *m) const;

    // The address of the object a member access is reaching into, with the
    // arrow/dot difference already resolved: `p->x` loads p, `o.x` takes o's
    // address.  This is the one place that distinction survives.
    IRReg lowerObjectAddress(MemberAccessExpr *ma);
    // `this` as a value -- it is a parameter, so this is a load from its slot.
    IRReg loadThis(int line);

    // --- the hooks the C layer asks ------------------------------------
    virtual bool lowerLayerValue(cc::Expr *e, IRReg &out);
    virtual bool lowerLayerAddress(cc::Expr *e, IRReg &out);
    virtual IRReg lowerCall(cc::CallExpr *e, bool wantsResult);
    virtual bool isReferenceExpr(cc::Expr *e);
    virtual void lowerDecl(cc::Decl *d);
    virtual void emitPrologue(cc::Function *f);
    virtual void emitEpilogue(cc::Function *f);
    virtual void emitScopeExit(cc::CompoundStmt *block);
    virtual void emitAllOpenScopeExits();
    // A class-typed local is CONSTRUCTED where it is declared.
    virtual void lowerVarDecl(cc::VarDecl *vd);
    // The C layer recomputes only what C can express; these are the C++ forms.
    virtual cc::Type *typeOf(cc::Expr *e);

    // --- object lifetime ------------------------------------------------
    // Calls cd's constructor on the object at `objectAddr`.
    void emitConstruct(ClassDecl *cd, IRReg objectAddr,
                       const std::vector<cc::Expr*> &args, int line);
    // Calls cd's destructor, if the class or a base has one.
    void emitDestruct(ClassDecl *cd, IRReg objectAddr, int line,
                      bool concreteType = false);
    void emitVPtrStore(ClassDecl *cd, IRReg objectAddr, int line);
    bool classHasDestructor(ClassDecl *cd) const;
};

} // namespace cxx

#endif
