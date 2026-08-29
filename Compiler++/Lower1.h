// Lower1.h -- PASS 5b, LAYER 2: lowering the C++ layer, namespace `cxx`.
//
// Derives from cc::Lowering.  Each line below is a C++ construct written out
// in terms C already had:
//
//     a method        ->  a function whose first parameter is `this`
//     T&              ->  a pointer, with one more load on every use
//     obj.field       ->  an address plus a constant offset
//     p->method()     ->  load vptr, index by a constant slot, call it
//     new T(args)     ->  alloc(sizeof T), then call the constructor
//     delete p        ->  call the destructor, then free
//     a constructor   ->  base ctor, store vptr, member inits, body
//     a destructor    ->  body, members reversed, base dtor
//     a local dying   ->  a destructor call at every exit from its block
//
// After this pass nothing about C++ is left for a code generator to know.
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
    // Types this pass forms itself -- `this`, a `new` expression.  They belong
    // to no AST node, so this class owns and frees them.
    std::vector<cc::Type*> ownedTypes;
    cc::Type *makePointerToClass(const std::string &className);
    cc::Type *cloneType(cc::Type *t);
    ClassDecl *classOfType(cc::Type *t) const;  // through one pointer or ref
    const FieldLayout *findField(const std::string &className,
                                 const std::string &member) const;
    MethodDecl *findMethod(ClassDecl *cd, const std::string &member) const;
    int vtableSlotOf(const std::string &className, MethodDecl *m) const;

    // Resolves the arrow/dot difference: p->x loads p, o.x takes o's address.
    // The one place that distinction survives.
    IRReg lowerObjectAddress(MemberAccessExpr *ma);
    IRReg loadThis(int line);   // a parameter, so a load from its slot

    // --- the hooks the C layer asks ---
    virtual bool lowerLayerValue(cc::Expr *e, IRReg &out);
    virtual bool lowerLayerAddress(cc::Expr *e, IRReg &out);
    virtual IRReg lowerCall(cc::CallExpr *e, bool wantsResult);
    virtual bool isReferenceExpr(cc::Expr *e);
    virtual void lowerDecl(cc::Decl *d);
    virtual void emitPrologue(cc::Function *f);
    virtual void emitEpilogue(cc::Function *f);
    virtual void emitScopeExit(cc::CompoundStmt *block);
    virtual void emitAllOpenScopeExits();
    virtual void lowerVarDecl(cc::VarDecl *vd);     // constructs class locals
    virtual cc::Type *typeOf(cc::Expr *e);          // the C++ forms

    // --- object lifetime ---
    void emitConstruct(ClassDecl *cd, IRReg objectAddr,
                       const std::vector<cc::Expr*> &args, int line);
    void emitDestruct(ClassDecl *cd, IRReg objectAddr, int line,
                      bool concreteType = false);
    void emitVPtrStore(ClassDecl *cd, IRReg objectAddr, int line);
    bool classHasDestructor(ClassDecl *cd) const;
};

} // namespace cxx

#endif
