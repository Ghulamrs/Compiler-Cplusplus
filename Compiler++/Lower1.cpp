// Lower1.cpp
//
// C++98 only.  See Lower1.h for the table of what becomes what.

#include "Lower1.h"

#include <cstddef>

namespace cxx {

Lowering::Lowering(IRModule &module, const Layout &l, Diagnostics &d,
                   const std::map<std::string, ClassDecl*> &cls)
    : cc::Lowering(module, l, d), classes(cls), cachedBool(0) {}

// --- Looking things up ---

Lowering::~Lowering() {
    for (std::size_t i = 0; i < ownedTypes.size(); ++i) delete ownedTypes[i];
}

cc::Type *Lowering::boolType() {
    if (!cachedBool) {
        cachedBool = new BoolType();
        ownedTypes.push_back(cachedBool);
    }
    return cachedBool;
}

cc::Type *Lowering::makePointerToClass(const std::string &className) {
    cc::Type *t = new cc::PointerType(new ClassType(className));
    ownedTypes.push_back(t);
    return t;
}

cc::Type *Lowering::cloneType(cc::Type *t) {
    if (!t) return 0;
    if (cc::BuiltinType *bt = dynamic_cast<cc::BuiltinType*>(t)) return new cc::BuiltinType(bt->kind);
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

// Layout already flattened the base chain: an inherited field sits at the
// offset it had in the base, because the base subobject is at zero.
const FieldLayout *Lowering::findField(const std::string &className,
                                       const std::string &member) const {
    const ClassLayout *cl = layout.forClass(className);
    if (!cl) return 0;
    for (std::size_t i = 0; i < cl->fields.size(); ++i) {
        if (cl->fields[i].name == member) return &cl->fields[i];
    }
    return 0;
}

// The semantic pass already chose the overload; this finds one by name only,
// for the cases where no call is involved.
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
        // The slot holds the FINAL override, a different MethodDecl than the
        // one lookup found.
        if (cl->vtable[s]->name == m->name) return static_cast<int>(s);
    }
    return -1;
}

bool Lowering::classHasDestructor(ClassDecl *cd) const {
    for (ClassDecl *c = cd; c; c = c->base) if (c->dtor) return true;
    return false;
}

// --- Vtables, emitted as module data ---

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
            else vt.slots.push_back(mangleOverload(m->ownerClass, m->name, m->params));
        }
        mod.vtables.push_back(vt);
    }
}

// --- Declarations ---

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
            else                       mangled = mangleOverload(cd->name, md->name, md->params);
            // A function with `this` in front -- all "member function" means
            // once the C++ is gone.
            lowerFunction(md, mangled, cd->name + "::" + md->name, true);
        }
        currentClass = saved;
        return;
    }
    cc::Lowering::lowerDecl(d);
}

// --- `this`, member addresses ---

IRReg Lowering::loadThis(int line) {
    const int slot = findSlot("this");
    if (slot < 0) return fn->emitConst(0, line);
    const IRReg addr = fn->emitLocalAddr(slot, line);
    return fn->emitLoad(addr, Layout::PointerSize, false, line);
}

// o.x needs o's ADDRESS, p->x needs p's VALUE.  Both end as an object address,
// so the arrow/dot distinction ends here and never reaches the IR.
IRReg Lowering::lowerObjectAddress(MemberAccessExpr *ma) {
    if (ma->isArrow) return lowerValue(ma->base);
    return lowerAddress(ma->base);
}

bool Lowering::lowerLayerAddress(cc::Expr *e, IRReg &out) {
    if (MemberAccessExpr *ma = dynamic_cast<MemberAccessExpr*>(e)) {
        const IRReg obj = lowerObjectAddress(ma);
        ClassDecl *cd = classOfType(typeOf(ma->base));
        // The base may be `this`, or a type lowering did not recompute.
        if (!cd) cd = findClass(currentClass);
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

    if (BoolExpr *b = dynamic_cast<BoolExpr*>(e)) {
        out = fn->emitConst(b->value ? 1 : 0, e->line);
        return true;
    }

    if (MemberAccessExpr *ma = dynamic_cast<MemberAccessExpr*>(e)) {
        IRReg addr = IR_NoReg;
        if (!lowerLayerAddress(e, addr)) return false;
        ClassDecl *cd = classOfType(typeOf(ma->base));
        if (!cd) cd = findClass(currentClass);
        const FieldLayout *f = cd ? findField(cd->name, ma->member) : 0;
        out = fn->emitLoad(addr, f ? f->size : Layout::IntSize,
                           f && isFloatType(f->type), e->line);
        return true;
    }

    // An unqualified field name inside a method.
    if (cc::IdentExpr *id = dynamic_cast<cc::IdentExpr*>(e)) {
        if (findSlot(id->name) >= 0) return false;
        if (currentClass.empty()) return false;
        const FieldLayout *f = findField(currentClass, id->name);
        if (!f) return false;
        const IRReg addr = fn->emitFieldAddr(loadThis(e->line), f->offset, e->line);
        out = fn->emitLoad(addr, f->size, isFloatType(f->type), e->line);
        return true;
    }

    // Allocate, then construct -- two steps, in the order the language says.
    if (NewExpr *ne = dynamic_cast<NewExpr*>(e)) {
        ClassDecl *cd = classOfType(ne->allocType);
        const int size = cd ? layout.sizeOf(ne->allocType) : layout.sizeOf(ne->allocType);
        out = fn->emitAlloc(size > 0 ? size : Layout::IntSize, e->line);
        if (cd) emitConstruct(cd, out, ne->args, e->line);
        return true;
    }

    // Destroy, then release: the reverse of `new`.  The destructor reached is
    // whichever the vtable names, hence the non-virtual-destructor warning.
    if (DeleteExpr *de = dynamic_cast<DeleteExpr*>(e)) {
        const IRReg ptr = lowerValue(de->operand);
        ClassDecl *cd = classOfType(typeOf(de->operand));
        if (cd) emitDestruct(cd, ptr, e->line);
        fn->emitFree(ptr, e->line);
        out = ptr;              // delete has no value; reusing ptr emits nothing
        return true;
    }
    return false;
}

// Member access, calls, `new` and `this` need the class table.
cc::Type *Lowering::typeOf(cc::Expr *e) {
    if (!e) return 0;

    if (dynamic_cast<BoolExpr*>(e)) return boolType();
    // A comparison yields bool, so a value stored from one is one byte wide.
    if (cc::BinaryExpr *be = dynamic_cast<cc::BinaryExpr*>(e)) {
        if (cc::binaryOpIsComparison(be->op) || cc::binaryOpIsLogical(be->op)) {
            return boolType();
        }
    }
    if (cc::UnaryExpr *ue = dynamic_cast<cc::UnaryExpr*>(e)) {
        if (ue->op == cc::UN_Not) return boolType();
    }

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

// Declaring one CONSTRUCTS it; the block already knows to destroy it.
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

// The C layer cannot copy a class or reference type; this layer can.
cc::Type *Lowering::cloneForeignType(cc::Type *t) {
    cc::Type *c = cloneType(t);
    if (c) ownedTypes.push_back(c);
    return c ? c : t;
}

bool Lowering::isReferenceType(cc::Type *t) {
    return dynamic_cast<ReferenceType*>(t) != 0;
}

bool Lowering::isBoolType(cc::Type *t) {
    return dynamic_cast<BoolType*>(t) != 0;
}

// --- Calls, including the one that matters ---

IRReg Lowering::lowerCall(cc::CallExpr *e, bool wantsResult) {
    MemberAccessExpr *ma = dynamic_cast<MemberAccessExpr*>(e->callee);
    if (!ma) {
        // A bare name inside a method may still be a method call on `this`.
        cc::IdentExpr *id = dynamic_cast<cc::IdentExpr*>(e->callee);
        if (id && !currentClass.empty() && findSlot(id->name) < 0) {
            ClassDecl *cd = findClass(currentClass);
            MethodDecl *m = dynamic_cast<MethodDecl*>(e->resolved);
            if (!m && cd) m = findMethod(cd, id->name);
            if (m) {
                std::vector<IRReg> args;
                args.push_back(loadThis(e->line));
                const std::vector<IRReg> rest = lowerArgs(e, m, 0);
                args.insert(args.end(), rest.begin(), rest.end());
                return fn->emitCall(mangleOverload(m->ownerClass, m->name, m->params),
                                    args, wantsResult, e->line);
            }
        }
        return cc::Lowering::lowerCall(e, wantsResult);
    }

    const IRReg object = lowerObjectAddress(ma);
    ClassDecl *cd = classOfType(typeOf(ma->base));
    if (!cd) cd = findClass(currentClass);
    // The overload was decided during analysis; using it here keeps one answer.
    MethodDecl *m = dynamic_cast<MethodDecl*>(e->resolved);
    if (!m && cd) m = findMethod(cd, ma->member);
    if (!m) {
        diag.error(e->line, e->col, "internal: method not found while lowering a call");
        return fn->emitConst(0, e->line);
    }

    // Every method takes `this`, virtual or not: dispatch decides WHICH
    // function runs, not how it is called.
    std::vector<IRReg> args;
    args.push_back(object);
    const std::vector<IRReg> rest = lowerArgs(e, m, 0);
    args.insert(args.end(), rest.begin(), rest.end());

    // Through a pointer or reference the static type is only a lower bound, so
    // the call must dispatch.  On a named object it cannot be anything but that
    // class's override, so the slot lookup is pure overhead.  This is the one
    // optimisation the pass performs, and every real compiler performs it too.
    bool dynamicType = true;
    if (!ma->isArrow) {
        cc::Type *bt = typeOf(ma->base);
        if (bt && dynamic_cast<ClassType*>(bt) != 0) dynamicType = false;
    }

    if (m->isVirtual && dynamicType) {
        // All of dynamic dispatch: the slot is a compile-time constant, the
        // function found there is not.
        const int slot = vtableSlotOf(cd->name, m);
        if (slot >= 0) {
            const IRReg target = fn->emitVCallTarget(object, slot, e->line);
            return fn->emitCallIndirect(target, args, wantsResult, e->line);
        }
    }
    if (m->isVirtual && !dynamicType) {
        // The final override for this exact class.
        const ClassLayout *cl = layout.forClass(cd->name);
        if (cl) {
            for (std::size_t s = 0; s < cl->vtable.size(); ++s) {
                if (cl->vtable[s]->name == m->name) { m = cl->vtable[s]; break; }
            }
        }
    }
    return fn->emitCall(mangleOverload(m->ownerClass, m->name, m->params), args,
                        wantsResult, e->line);
}

// --- Object lifetime ---

void Lowering::emitVPtrStore(ClassDecl *cd, IRReg objectAddr, int line) {
    const ClassLayout *cl = layout.forClass(cd->name);
    if (!cl || !cl->hasVPtr) return;
    // Offset 0 -- the decision that makes an upcast free.
    const IRReg vt = fn->emitGlobalAddr(mangleVTable(cd->name), line);
    const IRReg at = fn->emitFieldAddr(objectAddr, 0, line);
    fn->emitStore(at, vt, Layout::PointerSize, false, line);
}

void Lowering::emitConstruct(ClassDecl *cd, IRReg objectAddr,
                             const std::vector<cc::Expr*> &args, int line) {
    if (cd->ctors.empty()) {
        // No constructor, but a polymorphic object still needs its vptr.
        emitVPtrStore(cd, objectAddr, line);
        return;
    }
    MethodDecl *ctor = 0;
    for (std::size_t i = 0; i < cd->ctors.size(); ++i) {
        if (cd->ctors[i]->params.size() == args.size()) { ctor = cd->ctors[i]; break; }
    }
    std::vector<IRReg> callArgs;
    callArgs.push_back(objectAddr);
    for (std::size_t i = 0; i < args.size(); ++i) {
        IRReg v = lowerValue(args[i]);
        if (ctor && i < ctor->params.size()) {
            v = convert(v, typeOf(args[i]), ctor->params[i]->type, line);
        }
        callArgs.push_back(v);
    }
    fn->emitCall(mangleConstructor(cd->name, args.size()), callArgs, false, line);
}

// `concreteType`: the exact class is known (a named local), so the destructor
// is called directly, for the same reason a method call on one is.
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

// In the order Layout fixed: base, vptr, members.
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

    // 2. the vptr: AFTER the base, so the base ctor still dispatched as a
    //    base; BEFORE the members, so the body can call its own virtuals.
    emitVPtrStore(cd, self, f->line);

    // 3. members, in DECLARATION order however the list was written
    for (std::size_t i = 0; i < cd->members.size(); ++i) {
        FieldDecl *fd = dynamic_cast<FieldDecl*>(cd->members[i]);
        if (!fd) continue;
        for (std::size_t k = 0; k < md->memberInits.size(); ++k) {
            MemberInit &mi = md->memberInits[k];
            if (mi.isBase || mi.name != fd->name || mi.args.empty()) continue;
            const FieldLayout *fl = findField(cd->name, fd->name);
            if (!fl) break;
            IRReg value = lowerValue(mi.args[0]);
            value = convert(value, typeOf(mi.args[0]), fd->type, mi.line);
            const IRReg addr = fn->emitFieldAddr(loadThis(f->line), fl->offset, mi.line);
            fn->emitStore(addr, value, fl->size, isFloatType(fd->type), mi.line);
            break;
        }
    }
}

// Members backwards, then the base.  The body has already run, which makes
// this the exact reverse of construction.
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
    // Always direct, never through the vtable: the derived part is gone.
    fn->emitCall(mangleDestructor(owner->name), args, false, f->line);
}

// In the order the semantic pass recorded: reverse of construction.
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

// A return leaves every open block at once, innermost first.
void Lowering::emitAllOpenScopeExits() {
    for (std::size_t i = openBlocks.size(); i > 0; --i) emitScopeExit(openBlocks[i - 1]);
}

} // namespace cxx
