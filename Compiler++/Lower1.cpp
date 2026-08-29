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
    const std::string want = mangleSignature(m->params);
    for (std::size_t s = 0; s < cl->vtable.size(); ++s) {
        // The slot holds the FINAL override, a different MethodDecl than the
        // one lookup found -- but the same NAME AND SIGNATURE.  Two virtuals
        // may share a name, and matching on the name alone picks whichever
        // was declared first.
        if (cl->vtable[s]->name == m->name &&
            mangleSignature(cl->vtable[s]->params) == want) {
            return static_cast<int>(s);
        }
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
    // An overloaded operator IS a call, so it is lowered as one: the left
    // operand is the object, the right is the single argument.  Semantic chose
    // the member; nothing is re-resolved here.
    if (cc::BinaryExpr *be = dynamic_cast<cc::BinaryExpr*>(e)) {
        if (be->resolvedOperator) {
            out = emitOperatorCall(be->resolvedOperator, be->lhs, be->rhs, be->line);
            return true;
        }
    }

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
        // An array field decays, and an object field is not register-sized:
        // for both, the value IS the address, with nothing loaded.
        if (f && (isArrayType(f->type) || isObjectType(f->type))) {
            out = addr;
            return true;
        }
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
        if (isArrayType(f->type) || isObjectType(f->type)) { out = addr; return true; }
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
        // A plain call is the C layer's to answer.
        return cc::Lowering::typeOf(e);
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

    // P ps[3];  is three objects, each constructed.
    long count = 0;
    if (ClassDecl *elem = elementClassOf(vd->type, count)) {
        const int total = layout.sizeOf(vd->type);
        const int slot = declareLocal(vd->name, total > 0 ? total : Layout::PointerSize, false);
        localTypes[vd->name] = vd->type;
        const ClassLayout *cl = layout.forClass(elem->name);
        if (cl && count > 0) {
            emitArrayConstruct(elem, fn->emitLocalAddr(slot, vd->line), count,
                               cl->size, vd->line);
        }
        return;
    }

    ClassDecl *cd = 0;
    if (!dynamic_cast<ReferenceType*>(vd->type)) {
        ClassType *ct = dynamic_cast<ClassType*>(vd->type);
        if (ct) cd = findClass(ct->className);
    }
    if (!cd) { cc::Lowering::lowerVarDecl(vd); return; }

    const int size = layout.sizeOf(vd->type);
    const int slot = declareLocal(vd->name, size > 0 ? size : Layout::PointerSize, false);
    localTypes[vd->name] = vd->type;

    // P b = a;  copies a.  A declared copy constructor is the copy if there is
    // one; otherwise the copy is memberwise, which carries the vptr and is
    // right for two objects of one class.
    if (vd->init && !vd->hasCtorArgs) {
        if (copyConstructorOf(cd)) {
            std::vector<cc::Expr*> one;
            one.push_back(vd->init);
            emitConstruct(cd, fn->emitLocalAddr(slot, vd->line), one, vd->line);
            return;
        }
        if (!isAddressable(vd->init) && !yieldsObject(vd->init)) {
            diag.error(vd->line, vd->col,
                       "an object can only be copied from another object in this version");
            return;
        }
        const IRReg src = lowerObjectValue(vd->init);
        fn->emitMemCopy(fn->emitLocalAddr(slot, vd->line), src, size, vd->line);
        return;
    }
    emitConstruct(cd, fn->emitLocalAddr(slot, vd->line), vd->ctorArgs, vd->line);
}

// A constructor taking one argument of this same class -- by reference, since
// taking it by value would need the very copy being defined.
MethodDecl *Lowering::copyConstructorOf(ClassDecl *cd) const {
    if (!cd) return 0;
    for (std::size_t i = 0; i < cd->ctors.size(); ++i) {
        MethodDecl *c = cd->ctors[i];
        if (c->params.size() != 1) continue;
        cc::Type *p = c->params[0]->type;
        ReferenceType *rt = dynamic_cast<ReferenceType*>(p);
        if (!rt) continue;
        ClassType *ct = dynamic_cast<ClassType*>(rt->base);
        if (ct && ct->className == cd->name) return c;
    }
    return 0;
}

ClassDecl *Lowering::elementClassOf(cc::Type *t, long &count) const {
    count = 1;
    cc::ArrayType *at = dynamic_cast<cc::ArrayType*>(t);
    if (!at) return 0;
    while (at) {
        count *= at->count;
        t = at->element;
        at = dynamic_cast<cc::ArrayType*>(t);
    }
    ClassType *ct = dynamic_cast<ClassType*>(t);
    return ct ? findClass(ct->className) : 0;
}

// Element 0 first, exactly as a single object is built before the next one.
void Lowering::emitArrayConstruct(ClassDecl *cd, IRReg base, long count,
                                  int elemSize, int line) {
    std::vector<cc::Expr*> none;
    for (long i = 0; i < count; ++i) {
        emitConstruct(cd, fn->emitFieldAddr(base, static_cast<int>(i) * elemSize, line),
                      none, line);
    }
}

// Reverse, for the same reason members are destroyed in reverse.
void Lowering::emitArrayDestruct(ClassDecl *cd, IRReg base, long count,
                                 int elemSize, int line) {
    for (long i = count; i > 0; --i) {
        emitDestruct(cd, fn->emitFieldAddr(base, static_cast<int>(i - 1) * elemSize, line),
                     line, true);
    }
}

// A call or an overloaded operator whose result is an object: it has no name,
// but it does have a place -- the slot the caller supplied for it.
bool Lowering::yieldsObject(cc::Expr *e) const {
    if (cc::CallExpr *c = dynamic_cast<cc::CallExpr*>(e)) {
        return c->resolved && dynamic_cast<ClassType*>(c->resolved->retType) != 0;
    }
    if (cc::BinaryExpr *b = dynamic_cast<cc::BinaryExpr*>(e)) {
        return b->resolvedOperator
            && dynamic_cast<ClassType*>(b->resolvedOperator->retType) != 0;
    }
    return false;
}

// Only something with a place in memory can be copied from.
bool Lowering::isAddressable(cc::Expr *e) const {
    if (dynamic_cast<cc::IdentExpr*>(e))    return true;
    if (dynamic_cast<MemberAccessExpr*>(e)) return true;
    if (cc::UnaryExpr *u = dynamic_cast<cc::UnaryExpr*>(e)) return u->op == cc::UN_Deref;
    return false;
}

// A global object is constructed exactly as a local one is; only where its
// storage lives differs.
void Lowering::initGlobal(cc::VarDecl *vd, IRReg addr) {
    if (ClassDecl *cd = classOfMemberType(vd->type)) {
        emitConstruct(cd, addr, vd->ctorArgs, vd->line);
        return;
    }
    cc::Lowering::initGlobal(vd, addr);
}

bool Lowering::isReferenceExpr(cc::Expr *e) {
    cc::Type *t = typeOf(e);
    return t && dynamic_cast<ReferenceType*>(t) != 0;
}

// The C layer cannot copy a class or reference type; this layer can.  The
// copy is the caller's -- whoever asked is about to own it.
cc::Type *Lowering::cloneForeignType(cc::Type *t) {
    return cloneType(t);
}

bool Lowering::isReferenceType(cc::Type *t) {
    return dynamic_cast<ReferenceType*>(t) != 0;
}

bool Lowering::isObjectType(cc::Type *t) {
    return classOfMemberType(t) != 0;
}

cc::Type *Lowering::referentType(cc::Type *t) {
    ReferenceType *rt = dynamic_cast<ReferenceType*>(t);
    return rt ? rt->base : t;
}

bool Lowering::isBoolType(cc::Type *t) {
    return dynamic_cast<BoolType*>(t) != 0;
}

// --- Calls, including the one that matters ---

// object.operatorX(argument) -- with the object passed as `this`, and the
// argument obeying the same by-reference rule every other parameter does.
// One operand at a time, each obeying the rule its parameter declares.
IRReg Lowering::lowerOperandFor(cc::Type *want, cc::Expr *e, int line) {
    if (want && isReferenceType(want)) return lowerAddress(e);
    // By value: an object's ADDRESS goes over and the VM copies it into the
    // parameter's own slot, which is the copy the callee owns.
    if (want && isObjectType(want))    return lowerObjectValue(e);
    IRReg v = lowerValue(e);
    if (want) v = convert(v, referentType(typeOf(e)), want, line);
    return v;
}

// A member operator is a method call on the left operand.  A non-member is an
// ordinary two-argument call -- and the only form that can take a class on the
// RIGHT, which is what makes  3 * v  work.
IRReg Lowering::emitOperatorCall(cc::Function *op, cc::Expr *lhsExpr,
                                 cc::Expr *rhsExpr, int line) {
    MethodDecl *asMember = dynamic_cast<MethodDecl*>(op);
    std::vector<IRReg> args;

    if (asMember) {
        args.push_back(lowerObjectValue(lhsExpr));          // `this`
        if (returnsObject(op)) args.push_back(allocReturnSlot(op, line));
        args.push_back(lowerOperandFor(op->params.empty() ? 0 : op->params[0]->type,
                                       rhsExpr, line));
        return fn->emitCall(mangleOverload(asMember->ownerClass, op->name, op->params),
                            args, true, line);
    }

    if (returnsObject(op)) args.push_back(allocReturnSlot(op, line));
    args.push_back(lowerOperandFor(op->params.size() > 0 ? op->params[0]->type : 0,
                                   lhsExpr, line));
    args.push_back(lowerOperandFor(op->params.size() > 1 ? op->params[1]->type : 0,
                                   rhsExpr, line));
    return fn->emitCall(symbolFor(op, ""), args, true, line);
}

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
                if (returnsObject(m)) args.push_back(allocReturnSlot(m, e->line));
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
    if (returnsObject(m)) args.push_back(allocReturnSlot(m, e->line));
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
            const std::string want = mangleSignature(m->params);
            for (std::size_t s = 0; s < cl->vtable.size(); ++s) {
                if (cl->vtable[s]->name == m->name &&
                    mangleSignature(cl->vtable[s]->params) == want) {
                    m = cl->vtable[s];
                    break;
                }
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
        cc::Type *want = (ctor && i < ctor->params.size()) ? ctor->params[i]->type : 0;
        IRReg v;
        // Same rule a call obeys: a reference parameter receives the object's
        // ADDRESS.  A constructor is a call, and used to be the exception.
        if (want && isReferenceType(want)) {
            v = lowerAddress(args[i]);
        } else {
            v = lowerValue(args[i]);
            if (want) v = convert(v, typeOf(args[i]), want, line);
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

        const FieldLayout *fl = findField(cd->name, fd->name);
        if (!fl) continue;

        // Whatever the initialiser list says about this member, if anything.
        const MemberInit *mi = 0;
        for (std::size_t k = 0; k < md->memberInits.size(); ++k) {
            if (!md->memberInits[k].isBase && md->memberInits[k].name == fd->name) {
                mi = &md->memberInits[k];
                break;
            }
        }
        const int line = mi ? mi->line : f->line;

        // A member that is itself a class is CONSTRUCTED, not assigned -- and
        // it is constructed even when the initialiser list never mentions it.
        if (ClassDecl *member = classOfMemberType(fd->type)) {
            const IRReg addr = fn->emitFieldAddr(loadThis(f->line), fl->offset, line);
            if (mi) {
                emitConstruct(member, addr, mi->args, line);
            } else {
                std::vector<cc::Expr*> none;
                emitConstruct(member, addr, none, line);
            }
            continue;
        }

        // A scalar member is only touched when the list names it; C leaves an
        // uninitialised variable alone and so does this.
        if (!mi || mi->args.empty()) continue;
        IRReg value = lowerValue(mi->args[0]);
        value = convert(value, typeOf(mi->args[0]), fd->type, line);
        const IRReg addr = fn->emitFieldAddr(loadThis(f->line), fl->offset, line);
        fn->emitStore(addr, value, fl->size, isFloatType(fd->type), line);
    }
}

// A field's class, when the field is an object rather than a pointer or a
// reference to one -- only an object is constructed with its container.
ClassDecl *Lowering::classOfMemberType(cc::Type *t) const {
    if (dynamic_cast<cc::PointerType*>(t)) return 0;
    if (dynamic_cast<ReferenceType*>(t))   return 0;
    ClassType *ct = dynamic_cast<ClassType*>(t);
    return ct ? findClass(ct->className) : 0;
}

// Members backwards, then the base.  The body has already run, which makes
// this the exact reverse of construction.
void Lowering::emitEpilogue(cc::Function *f) {
    MethodDecl *md = dynamic_cast<MethodDecl*>(f);
    if (!md || !md->isDestructor) return;
    ClassDecl *cd = findClass(md->ownerClass);
    if (!cd) return;

    // Members in reverse declaration order, before the base -- the exact
    // reverse of the order emitPrologue built them in.
    for (std::size_t i = cd->members.size(); i > 0; --i) {
        FieldDecl *fd = dynamic_cast<FieldDecl*>(cd->members[i - 1]);
        if (!fd) continue;
        ClassDecl *member = classOfMemberType(fd->type);
        if (!member || !classHasDestructor(member)) continue;
        const FieldLayout *fl = findField(cd->name, fd->name);
        if (!fl) continue;
        emitDestruct(member, fn->emitFieldAddr(loadThis(f->line), fl->offset, f->line),
                     f->line, true);
    }

    if (!cd->base) return;
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
        const int slot = findSlot(vd->name);
        if (slot < 0) continue;

        long count = 0;
        if (ClassDecl *elem = elementClassOf(vd->type, count)) {
            const ClassLayout *cl = layout.forClass(elem->name);
            if (cl && count > 0) {
                emitArrayDestruct(elem, fn->emitLocalAddr(slot, vd->line), count,
                                  cl->size, vd->line);
            }
            continue;
        }

        ClassDecl *cd = classOfType(vd->type);
        if (!cd) continue;
        emitDestruct(cd, fn->emitLocalAddr(slot, vd->line), vd->line, true);
    }
}

// A return leaves every open block at once, innermost first.
void Lowering::emitScopeExitsDownTo(std::size_t depth) {
    for (std::size_t i = openBlocks.size(); i > depth; --i) emitScopeExit(openBlocks[i - 1]);
}

void Lowering::emitAllOpenScopeExits() {
    for (std::size_t i = openBlocks.size(); i > 0; --i) emitScopeExit(openBlocks[i - 1]);
}

} // namespace cxx
