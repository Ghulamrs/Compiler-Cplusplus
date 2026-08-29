// Lower1.cpp
//
// C++98 only.  See Lower1.h for the table of what becomes what.

#include "Lower1.h"

#include <cstddef>

namespace cxx {

Lowering::Lowering(IRModule &module, const Layout &l, Diagnostics &d,
                   const std::map<std::string, ClassDecl*> &cls)
    : cc::Lowering(module, l, d), classes(cls) {}

// ---------------------------------------------------------------------
// Looking things up
// ---------------------------------------------------------------------

Lowering::~Lowering() {
    for (std::size_t i = 0; i < ownedTypes.size(); ++i) delete ownedTypes[i];
}

cc::Type *Lowering::makePointerToClass(const std::string &className) {
    cc::Type *t = new cc::PointerType(new ClassType(className));
    ownedTypes.push_back(t);
    return t;
}

cc::Type *Lowering::cloneType(cc::Type *t) {
    if (!t) return 0;
    if (cc::BuiltinType *bt = dynamic_cast<cc::BuiltinType*>(t)) return new cc::BuiltinType(bt->name);
    if (ClassType *ct = dynamic_cast<ClassType*>(t)) return new ClassType(ct->className);
    if (cc::PointerType *pt = dynamic_cast<cc::PointerType*>(t)) return new cc::PointerType(cloneType(pt->base));
    if (ReferenceType *rt = dynamic_cast<ReferenceType*>(t)) return new ReferenceType(cloneType(rt->base));
    return 0;
}

ClassDecl *Lowering::findClass(const std::string &name) const {
    std::map<std::string, ClassDecl*>::const_iterator it = classes.find(name);
    return (it == classes.end()) ? 0 : it->second;
}

ClassDecl *Lowering::classOfType(cc::Type *t) const {
    if (!t) return 0;
    if (ReferenceType *rt = dynamic_cast<ReferenceType*>(t)) t = rt->base;
    if (cc::PointerType *pt = dynamic_cast<cc::PointerType*>(t)) t = pt->base;
    ClassType *ct = dynamic_cast<ClassType*>(t);
    return ct ? findClass(ct->className) : 0;
}

// Offsets come from Layout, which already flattened the base chain -- an
// inherited field is in the derived class's field list at the offset it had in
// the base, because the base subobject sits at zero.
const FieldLayout *Lowering::findField(const std::string &className,
                                       const std::string &member) const {
    const ClassLayout *cl = layout.forClass(className);
    if (!cl) return 0;
    for (std::size_t i = 0; i < cl->fields.size(); ++i) {
        if (cl->fields[i].name == member) return &cl->fields[i];
    }
    return 0;
}

MethodDecl *Lowering::findMethod(ClassDecl *cd, const std::string &member) const {
    for (ClassDecl *c = cd; c; c = c->base) {
        for (std::size_t i = 0; i < c->members.size(); ++i) {
            MethodDecl *md = dynamic_cast<MethodDecl*>(c->members[i]);
            if (md && !md->isConstructor && !md->isDestructor && md->name == member) return md;
        }
    }
    return 0;
}

int Lowering::vtableSlotOf(const std::string &className, MethodDecl *m) const {
    const ClassLayout *cl = layout.forClass(className);
    if (!cl) return -1;
    for (std::size_t s = 0; s < cl->vtable.size(); ++s) {
        // Match by name: the slot holds the FINAL override, which for a base
        // pointer is a different MethodDecl than the one lookup found.
        if (cl->vtable[s]->name == m->name) return static_cast<int>(s);
    }
    return -1;
}

bool Lowering::classHasDestructor(ClassDecl *cd) const {
    for (ClassDecl *c = cd; c; c = c->base) if (c->dtor) return true;
    return false;
}

// ---------------------------------------------------------------------
// Vtables, emitted as module data
// ---------------------------------------------------------------------

void Lowering::lowerClasses() {
    std::map<std::string, ClassDecl*>::const_iterator it;
    for (it = classes.begin(); it != classes.end(); ++it) {
        const ClassLayout *cl = layout.forClass(it->first);
        if (!cl || !cl->hasVPtr) continue;
        IRVTable vt;
        vt.className = it->first;
        for (std::size_t s = 0; s < cl->vtable.size(); ++s) {
            MethodDecl *m = cl->vtable[s];
            if (m->isDestructor) vt.slots.push_back(mangleDestructor(m->ownerClass));
            else                 vt.slots.push_back(mangleFunction(m->ownerClass, m->name));
        }
        mod.vtables.push_back(vt);
    }
}

// ---------------------------------------------------------------------
// Declarations
// ---------------------------------------------------------------------

void Lowering::lowerDecl(cc::Decl *d) {
    ClassDecl *cd = dynamic_cast<ClassDecl*>(d);
    if (cd) {
        const std::string saved = currentClass;
        currentClass = cd->name;
        for (std::size_t i = 0; i < cd->members.size(); ++i) {
            MethodDecl *md = dynamic_cast<MethodDecl*>(cd->members[i]);
            if (!md || !md->body) continue;
            std::string mangled;
            if (md->isConstructor)     mangled = mangleConstructor(cd->name, md->params.size());
            else if (md->isDestructor) mangled = mangleDestructor(cd->name);
            else                       mangled = mangleFunction(cd->name, md->name);
            // A method is a function with `this` in front.  That is the whole
            // of what "member function" means once the C++ is gone.
            lowerFunction(md, mangled, cd->name + "::" + md->name, true);
        }
        currentClass = saved;
        return;
    }
    cc::Lowering::lowerDecl(d);
}

// ---------------------------------------------------------------------
// `this`, member addresses
// ---------------------------------------------------------------------

IRReg Lowering::loadThis(int line) {
    const int slot = findSlot("this");
    if (slot < 0) return fn->emitConst(0, line);
    const IRReg addr = fn->emitLocalAddr(slot, line);
    return fn->emitLoad(addr, Layout::PointerSize, line);
}

// `o.x` needs o's ADDRESS; `p->x` needs p's VALUE.  Both end up as the address
// of an object, which is the only thing the offset arithmetic below cares
// about -- so the arrow/dot distinction ends here and never reaches the IR.
IRReg Lowering::lowerObjectAddress(MemberAccessExpr *ma) {
    if (ma->isArrow) return lowerValue(ma->base);
    return lowerAddress(ma->base);
}

bool Lowering::lowerLayerAddress(cc::Expr *e, IRReg &out) {
    if (MemberAccessExpr *ma = dynamic_cast<MemberAccessExpr*>(e)) {
        const IRReg obj = lowerObjectAddress(ma);
        ClassDecl *cd = classOfType(typeOf(ma->base));
        if (!cd) {
            // The base may be `this`, or something whose type lowering did not
            // recompute; fall back to the enclosing class.
            cd = findClass(currentClass);
        }
        if (!cd) return false;
        const FieldLayout *f = findField(cd->name, ma->member);
        if (!f) return false;
        out = fn->emitFieldAddr(obj, f->offset, e->line);
        return true;
    }

    // An unqualified member name inside a method is `this->name`.
    if (cc::IdentExpr *id = dynamic_cast<cc::IdentExpr*>(e)) {
        if (findSlot(id->name) >= 0) return false;      // a real local wins
        if (currentClass.empty()) return false;
        const FieldLayout *f = findField(currentClass, id->name);
        if (!f) return false;
        out = fn->emitFieldAddr(loadThis(e->line), f->offset, e->line);
        return true;
    }
    return false;
}

bool Lowering::lowerLayerValue(cc::Expr *e, IRReg &out) {
    if (dynamic_cast<ThisExpr*>(e)) {
        out = loadThis(e->line);
        return true;
    }

    if (MemberAccessExpr *ma = dynamic_cast<MemberAccessExpr*>(e)) {
        IRReg addr = IR_NoReg;
        if (!lowerLayerAddress(e, addr)) return false;
        ClassDecl *cd = classOfType(typeOf(ma->base));
        if (!cd) cd = findClass(currentClass);
        const FieldLayout *f = cd ? findField(cd->name, ma->member) : 0;
        out = fn->emitLoad(addr, f ? f->size : Layout::IntSize, e->line);
        return true;
    }

    // An unqualified field name inside a method.
    if (cc::IdentExpr *id = dynamic_cast<cc::IdentExpr*>(e)) {
        if (findSlot(id->name) >= 0) return false;
        if (currentClass.empty()) return false;
        const FieldLayout *f = findField(currentClass, id->name);
        if (!f) return false;
        const IRReg addr = fn->emitFieldAddr(loadThis(e->line), f->offset, e->line);
        out = fn->emitLoad(addr, f->size, e->line);
        return true;
    }

    // new T(args): allocate, then construct.  Two steps, in that order, which
    // is exactly what the language promises.
    if (NewExpr *ne = dynamic_cast<NewExpr*>(e)) {
        ClassDecl *cd = classOfType(ne->allocType);
        const int size = cd ? layout.sizeOf(ne->allocType) : layout.sizeOf(ne->allocType);
        out = fn->emitAlloc(size > 0 ? size : Layout::IntSize, e->line);
        if (cd) emitConstruct(cd, out, ne->args, e->line);
        return true;
    }

    // delete p: destroy, then release.  The reverse order of `new`, and the
    // reason a non-virtual destructor on a base is a warning -- the destructor
    // reached here is whichever the vtable names.
    if (DeleteExpr *de = dynamic_cast<DeleteExpr*>(e)) {
        const IRReg ptr = lowerValue(de->operand);
        ClassDecl *cd = classOfType(typeOf(de->operand));
        if (cd) emitDestruct(cd, ptr, e->line);
        fn->emitFree(ptr, e->line);
        // `delete` has no value.  Handing back the pointer register costs no
        // instruction and keeps callers from having to special-case void.
        out = ptr;
        return true;
    }
    return false;
}

// The C layer can only work out the types C has.  A member access, a call, a
// `new` and `this` all need the class table, so they are recovered here.
cc::Type *Lowering::typeOf(cc::Expr *e) {
    if (!e) return 0;

    if (dynamic_cast<ThisExpr*>(e)) {
        if (currentClass.empty()) return 0;
        return makePointerToClass(currentClass);
    }
    if (MemberAccessExpr *ma = dynamic_cast<MemberAccessExpr*>(e)) {
        ClassDecl *cd = classOfType(typeOf(ma->base));
        if (!cd) cd = findClass(currentClass);
        if (!cd) return 0;
        for (ClassDecl *c = cd; c; c = c->base) {
            for (std::size_t i = 0; i < c->members.size(); ++i) {
                FieldDecl *fd = dynamic_cast<FieldDecl*>(c->members[i]);
                if (fd && fd->name == ma->member) return fd->type;
                MethodDecl *md = dynamic_cast<MethodDecl*>(c->members[i]);
                if (md && !md->isConstructor && !md->isDestructor && md->name == ma->member) {
                    return md->retType;
                }
            }
        }
        return 0;
    }
    if (NewExpr *ne = dynamic_cast<NewExpr*>(e)) {
        cc::Type *p = new cc::PointerType(cloneType(ne->allocType));
        ownedTypes.push_back(p);
        return p;
    }
    if (cc::CallExpr *call = dynamic_cast<cc::CallExpr*>(e)) {
        if (MemberAccessExpr *cma = dynamic_cast<MemberAccessExpr*>(call->callee)) {
            return typeOf(cma);
        }
        return 0;
    }
    // An unqualified field name inside a method.
    if (cc::IdentExpr *id = dynamic_cast<cc::IdentExpr*>(e)) {
        if (findSlot(id->name) < 0 && !currentClass.empty()) {
            for (ClassDecl *c = findClass(currentClass); c; c = c->base) {
                for (std::size_t i = 0; i < c->members.size(); ++i) {
                    FieldDecl *fd = dynamic_cast<FieldDecl*>(c->members[i]);
                    if (fd && fd->name == id->name) return fd->type;
                }
            }
        }
    }
    return cc::Lowering::typeOf(e);
}

// A class-typed local is an object, so declaring it CONSTRUCTS it, and the
// block it belongs to already knows to destroy it on the way out.
void Lowering::lowerVarDecl(cc::VarDecl *vd) {
    if (!vd) return;
    ClassDecl *cd = 0;
    if (!dynamic_cast<ReferenceType*>(vd->type)) {
        ClassType *ct = dynamic_cast<ClassType*>(vd->type);
        if (ct) cd = findClass(ct->className);
    }
    if (!cd) { cc::Lowering::lowerVarDecl(vd); return; }

    const int size = layout.sizeOf(vd->type);
    const int slot = declareLocal(vd->name, size > 0 ? size : Layout::PointerSize, false);
    localTypes[vd->name] = vd->type;
    emitConstruct(cd, fn->emitLocalAddr(slot, vd->line), vd->ctorArgs, vd->line);
}

bool Lowering::isReferenceExpr(cc::Expr *e) {
    cc::Type *t = typeOf(e);
    return t && dynamic_cast<ReferenceType*>(t) != 0;
}

// ---------------------------------------------------------------------
// Calls, including the one that matters
// ---------------------------------------------------------------------

IRReg Lowering::lowerCall(cc::CallExpr *e, bool wantsResult) {
    MemberAccessExpr *ma = dynamic_cast<MemberAccessExpr*>(e->callee);
    if (!ma) {
        // A bare name inside a method may still be a method call on `this`.
        cc::IdentExpr *id = dynamic_cast<cc::IdentExpr*>(e->callee);
        if (id && !currentClass.empty() && findSlot(id->name) < 0) {
            ClassDecl *cd = findClass(currentClass);
            MethodDecl *m = cd ? findMethod(cd, id->name) : 0;
            if (m) {
                std::vector<IRReg> args;
                args.push_back(loadThis(e->line));
                for (std::size_t i = 0; i < e->args.size(); ++i) {
                    args.push_back(lowerValue(e->args[i]));
                }
                return fn->emitCall(mangleFunction(m->ownerClass, m->name), args,
                                    wantsResult, e->line);
            }
        }
        return cc::Lowering::lowerCall(e, wantsResult);
    }

    const IRReg object = lowerObjectAddress(ma);
    ClassDecl *cd = classOfType(typeOf(ma->base));
    if (!cd) cd = findClass(currentClass);
    MethodDecl *m = cd ? findMethod(cd, ma->member) : 0;
    if (!m) {
        diag.error(e->line, e->col, "internal: method not found while lowering a call");
        return fn->emitConst(0, e->line);
    }

    // The receiver is the first argument.  Every method takes `this`, virtual
    // or not; dispatch decides WHICH function runs, not how it is called.
    std::vector<IRReg> args;
    args.push_back(object);
    for (std::size_t i = 0; i < e->args.size(); ++i) args.push_back(lowerValue(e->args[i]));

    // A virtual call through a POINTER or a REFERENCE has to be dispatched:
    // the static type is only a lower bound on what the object really is.  A
    // call on a named object is different -- `sq.area()` where sq is a Square
    // can be nothing but Square::area, so the slot lookup is pure overhead.
    // Resolving it here is the one optimisation this pass performs, and it is
    // the same one every real C++ compiler performs for the same reason.
    bool dynamicType = true;
    if (!ma->isArrow) {
        cc::Type *bt = typeOf(ma->base);
        if (bt && dynamic_cast<ClassType*>(bt) != 0) dynamicType = false;
    }

    if (m->isVirtual && dynamicType) {
        // The whole of dynamic dispatch: the slot index is a constant known at
        // compile time; the function found there is not.
        const int slot = vtableSlotOf(cd->name, m);
        if (slot >= 0) {
            const IRReg target = fn->emitVCallTarget(object, slot, e->line);
            return fn->emitCallIndirect(target, args, wantsResult, e->line);
        }
    }
    if (m->isVirtual && !dynamicType) {
        // The final override for this exact class, not the one lookup found.
        const ClassLayout *cl = layout.forClass(cd->name);
        if (cl) {
            for (std::size_t s = 0; s < cl->vtable.size(); ++s) {
                if (cl->vtable[s]->name == m->name) { m = cl->vtable[s]; break; }
            }
        }
    }
    return fn->emitCall(mangleFunction(m->ownerClass, m->name), args, wantsResult, e->line);
}

// ---------------------------------------------------------------------
// Object lifetime
// ---------------------------------------------------------------------

void Lowering::emitVPtrStore(ClassDecl *cd, IRReg objectAddr, int line) {
    const ClassLayout *cl = layout.forClass(cd->name);
    if (!cl || !cl->hasVPtr) return;
    // The vptr lives at offset 0 -- the decision that makes an upcast free.
    const IRReg vt = fn->emitGlobalAddr(mangleVTable(cd->name), line);
    const IRReg at = fn->emitFieldAddr(objectAddr, 0, line);
    fn->emitStore(at, vt, Layout::PointerSize, line);
}

void Lowering::emitConstruct(ClassDecl *cd, IRReg objectAddr,
                             const std::vector<cc::Expr*> &args, int line) {
    if (cd->ctors.empty()) {
        // No constructor to call, but a polymorphic object still needs its
        // vptr before anyone dispatches through it.
        emitVPtrStore(cd, objectAddr, line);
        return;
    }
    std::vector<IRReg> callArgs;
    callArgs.push_back(objectAddr);
    for (std::size_t i = 0; i < args.size(); ++i) callArgs.push_back(lowerValue(args[i]));
    fn->emitCall(mangleConstructor(cd->name, args.size()), callArgs, false, line);
}

// `concreteType` says the exact class is known -- a named local, not something
// reached through a pointer -- in which case the destructor is called directly
// for the same reason a method call on a named object is.
void Lowering::emitDestruct(ClassDecl *cd, IRReg objectAddr, int line, bool concreteType) {
    if (!classHasDestructor(cd)) return;
    // Find the class that actually declares one, walking up.
    ClassDecl *owner = cd;
    while (owner && !owner->dtor) owner = owner->base;
    if (!owner) return;
    if (concreteType) {
        const ClassLayout *cl = layout.forClass(cd->name);
        if (cl) {
            for (std::size_t s = 0; s < cl->vtable.size(); ++s) {
                if (cl->vtable[s]->isDestructor) { owner = findClass(cl->vtable[s]->ownerClass); break; }
            }
        }
        if (!owner) owner = cd;
    }

    std::vector<IRReg> callArgs;
    callArgs.push_back(objectAddr);
    if (owner->dtor->isVirtual && !concreteType) {
        const ClassLayout *cl = layout.forClass(cd->name);
        int slot = -1;
        if (cl) {
            for (std::size_t s = 0; s < cl->vtable.size(); ++s) {
                if (cl->vtable[s]->isDestructor) { slot = static_cast<int>(s); break; }
            }
        }
        if (slot >= 0) {
            const IRReg target = fn->emitVCallTarget(objectAddr, slot, line);
            fn->emitCallIndirect(target, callArgs, false, line);
            return;
        }
    }
    fn->emitCall(mangleDestructor(owner->name), callArgs, false, line);
}

// A constructor's preamble, in the order Layout fixed: base, vptr, members.
void Lowering::emitPrologue(cc::Function *f) {
    MethodDecl *md = dynamic_cast<MethodDecl*>(f);
    if (!md || !md->isConstructor) return;
    ClassDecl *cd = findClass(md->ownerClass);
    if (!cd) return;

    const IRReg self = loadThis(f->line);

    // 1. the base subobject
    if (cd->base) {
        bool wroteBase = false;
        for (std::size_t i = 0; i < md->memberInits.size(); ++i) {
            if (!md->memberInits[i].isBase) continue;
            emitConstruct(cd->base, self, md->memberInits[i].args, f->line);
            wroteBase = true;
            break;
        }
        if (!wroteBase) {
            std::vector<cc::Expr*> none;
            emitConstruct(cd->base, self, none, f->line);
        }
    }

    // 2. the vptr -- AFTER the base, so that while the base constructor ran the
    //    object still dispatched as a base, and BEFORE the members, so the
    //    body can call its own virtuals.
    emitVPtrStore(cd, self, f->line);

    // 3. members, in DECLARATION order regardless of how the list was written
    for (std::size_t i = 0; i < cd->members.size(); ++i) {
        FieldDecl *fd = dynamic_cast<FieldDecl*>(cd->members[i]);
        if (!fd) continue;
        for (std::size_t k = 0; k < md->memberInits.size(); ++k) {
            MemberInit &mi = md->memberInits[k];
            if (mi.isBase || mi.name != fd->name || mi.args.empty()) continue;
            const FieldLayout *fl = findField(cd->name, fd->name);
            if (!fl) break;
            const IRReg value = lowerValue(mi.args[0]);
            const IRReg addr = fn->emitFieldAddr(loadThis(f->line), fl->offset, mi.line);
            fn->emitStore(addr, value, fl->size, mi.line);
            break;
        }
    }
}

// A destructor's tail: members backwards, then the base.  The body has already
// run by the time this is emitted, which is what makes the order the exact
// reverse of construction.
void Lowering::emitEpilogue(cc::Function *f) {
    MethodDecl *md = dynamic_cast<MethodDecl*>(f);
    if (!md || !md->isDestructor) return;
    ClassDecl *cd = findClass(md->ownerClass);
    if (!cd || !cd->base) return;
    if (!classHasDestructor(cd->base)) return;

    ClassDecl *owner = cd->base;
    while (owner && !owner->dtor) owner = owner->base;
    if (!owner) return;
    std::vector<IRReg> args;
    args.push_back(loadThis(f->line));
    // A base destructor is always called directly, never through the vtable:
    // the derived part is already gone, so there is nothing to dispatch to.
    fn->emitCall(mangleDestructor(owner->name), args, false, f->line);
}

// Destructors for the locals this block owns, in the order the semantic pass
// recorded -- reverse of construction.
void Lowering::emitScopeExit(cc::CompoundStmt *block) {
    for (std::size_t i = 0; i < block->destroyAtExit.size(); ++i) {
        cc::VarDecl *vd = block->destroyAtExit[i];
        ClassDecl *cd = classOfType(vd->type);
        if (!cd) continue;
        const int slot = findSlot(vd->name);
        if (slot < 0) continue;
        emitDestruct(cd, fn->emitLocalAddr(slot, vd->line), vd->line, true);
    }
}

// A `return` leaves every open block at once, so every one of them runs its
// destructors -- innermost first.
void Lowering::emitAllOpenScopeExits() {
    for (std::size_t i = openBlocks.size(); i > 0; --i) emitScopeExit(openBlocks[i - 1]);
}

} // namespace cxx
