// Lower.cpp
//
// C++98 only.  See Lower.h for the address/value idea this pass turns on.

#include "Lower.h"

#include <cstddef>

namespace cc {

Lowering::Lowering(IRModule &module, const Layout &l, Diagnostics &d)
    : mod(module), layout(l), diag(d), fn(0), currentReturnType(0) {}

Lowering::~Lowering() {
    for (std::map<int, Type*>::iterator it = builtinCache.begin();
         it != builtinCache.end(); ++it) {
        delete it->second;
    }
}

// --- Scopes and slots ---

void Lowering::pushScope() {
    scopeMarks.push_back(static_cast<int>(scopeNames.size()));
}

void Lowering::popScope() {
    if (scopeMarks.empty()) return;
    const int mark = scopeMarks.back();
    scopeMarks.pop_back();
    while (static_cast<int>(scopeNames.size()) > mark) {
        slots.erase(scopeNames.back());
        localTypes.erase(scopeNames.back());
        scopeNames.pop_back();
    }
}

int Lowering::declareLocal(const std::string &name, int size, bool isParam) {
    const int slot = fn->addLocal(name, size, isParam);
    slots[name] = slot;
    scopeNames.push_back(name);
    return slot;
}

int Lowering::findSlot(const std::string &name) const {
    std::map<std::string, int>::const_iterator it = slots.find(name);
    return (it == slots.end()) ? -1 : it->second;
}

int Lowering::sizeOfType(Type *t) const {
    const int s = layout.sizeOf(t);
    return s > 0 ? s : Layout::IntSize;
}

// Types the lowering pass forms for literals and for the common type of a
// binary operator.  Owned here, because they belong to no AST node.
Type *Lowering::literalType(BuiltinKind k) {
    std::map<int, Type*>::iterator it = builtinCache.find(static_cast<int>(k));
    if (it != builtinCache.end()) return it->second;
    Type *t = new BuiltinType(k);
    builtinCache[static_cast<int>(k)] = t;
    return t;
}

Type *Lowering::commonType(BuiltinKind k) { return literalType(k); }

// The same rule the semantic pass applied, restated where lowering needs it.
BuiltinKind Lowering::commonKind(BuiltinKind a, BuiltinKind b) {
    if (a == BK_Double || b == BK_Double) return BK_Double;
    if (a == BK_Float  || b == BK_Float)  return BK_Float;
    if (builtinRank(a) < builtinRank(BK_Int)) a = BK_Int;
    if (builtinRank(b) < builtinRank(BK_Int)) b = BK_Int;
    if (a == b) return a;
    const int ra = builtinRank(a), rb = builtinRank(b);
    if (ra != rb) return (ra > rb) ? a : b;
    return builtinIsSigned(a) ? b : a;
}

bool Lowering::isFloatType(Type *t) {
    BuiltinKind k;
    return arithKind(t, k) && builtinIsFloating(k);
}

bool Lowering::arithKind(Type *t, BuiltinKind &out) {
    BuiltinType *bt = dynamic_cast<BuiltinType*>(t);
    if (!bt || !builtinIsArithmetic(bt->kind)) return false;
    out = bt->kind;
    return true;
}

// A conversion is never free: a narrower integer must be truncated, a wider one
// sign- or zero-extended, and int and float do not even share a register file
// on most machines.  Emitting them explicitly is what stops a size mismatch
// slipping silently into the code generator.
IRReg Lowering::convert(IRReg value, Type *from, Type *to, int line) {
    BuiltinKind kf, kt;
    if (!arithKind(from, kf) || !arithKind(to, kt)) return value;
    if (kf == kt) return value;

    const bool ff = builtinIsFloating(kf);
    const bool ft = builtinIsFloating(kt);

    if (ff && ft) {
        return fn->emitConvert(IR_FloatResize, value, builtinSize(kt), IR_NoReg, line);
    }
    if (!ff && ft) {
        // Integer to floating; the source's signedness decides the instruction.
        return fn->emitConvert(IR_IntToFloat, value,
                               builtinIsSigned(kf) ? 0 : 1, IR_NoReg, line);
    }
    if (ff && !ft) {
        const IRReg asInt = fn->emitConvert(IR_FloatToInt, value,
                                            builtinSize(kt), IR_NoReg, line);
        return asInt;
    }
    // Integer to integer: resize, sign-extending only from a signed source.
    return fn->emitConvert(IR_IntResize, value, builtinSize(kt),
                           builtinIsSigned(kf) ? 1 : 0, line);
}

// --- types, recomputed cheaply ---------------------------------------

Type *Lowering::typeOf(Expr *e) {
    if (!e) return 0;
    if (NumberExpr *n = dynamic_cast<NumberExpr*>(e)) return literalType(n->kind);
    if (FloatExpr *f = dynamic_cast<FloatExpr*>(e))   return literalType(f->kind);
    if (IdentExpr *id = dynamic_cast<IdentExpr*>(e)) {
        std::map<std::string, Type*>::iterator it = localTypes.find(id->name);
        if (it != localTypes.end()) return it->second;
        it = globalTypes.find(id->name);
        if (it != globalTypes.end()) return it->second;
        return 0;
    }
    if (UnaryExpr *u = dynamic_cast<UnaryExpr*>(e)) {
        if (u->op == UN_Deref) {
            Type *base = typeOf(u->operand);
            if (PointerType *pt = dynamic_cast<PointerType*>(base)) return pt->base;
        }
        return 0;
    }
    if (BinaryExpr *b = dynamic_cast<BinaryExpr*>(e)) {
        if (b->op == BIN_Assign) return typeOf(b->lhs);
        // p + n and p - n stay pointers, which is what makes a[i] load the
        // right width.
        if (b->op == BIN_Add || b->op == BIN_Sub) {
            Type *lt = typeOf(b->lhs);
            if (dynamic_cast<PointerType*>(lt)) return lt;
            Type *rt = typeOf(b->rhs);
            if (b->op == BIN_Add && dynamic_cast<PointerType*>(rt)) return rt;
        }
        return 0;
    }
    return 0;
}

bool Lowering::isReferenceExpr(Expr *) {
    return false;               // C has no references
}

bool Lowering::isReferenceType(Type *) {
    return false;
}

// --- Declarations ---

void Lowering::lowerUnit(const std::vector<Decl*> &units) {
    // Globals first, so a function body can refer to any of them.
    for (std::size_t i = 0; i < units.size(); ++i) {
        VarDecl *vd = dynamic_cast<VarDecl*>(units[i]);
        if (vd) {
            mod.globals.push_back(IRGlobal(vd->name, sizeOfType(vd->type)));
            globalTypes[vd->name] = vd->type;
        }
    }
    // Record every function first, bodiless declarations included: an argument
    // must be converted to its parameter's type, and a native is declared and
    // never defined.
    for (std::size_t i = 0; i < units.size(); ++i) {
        Function *f = dynamic_cast<Function*>(units[i]);
        if (f) functions[f->name] = f;
    }
    for (std::size_t i = 0; i < units.size(); ++i) lowerDecl(units[i]);
}

void Lowering::lowerDecl(Decl *d) {
    Function *f = dynamic_cast<Function*>(d);
    if (f && f->body) lowerFunction(f, f->name, f->name, false);
}

void Lowering::lowerFunction(Function *f, const std::string &mangled,
                             const std::string &sourceName, bool hasThis) {
    IRFunction *irf = new IRFunction(mangled, sourceName);
    mod.functions.push_back(irf);

    IRFunction *savedFn = fn;
    Type *savedReturn = currentReturnType;
    fn = irf;
    currentReturnType = f->retType;
    // A fresh naming environment: nothing from the caller's scope is visible.
    std::map<std::string, int> savedSlots;
    savedSlots.swap(slots);
    std::map<std::string, Type*> savedTypes;
    savedTypes.swap(localTypes);

    pushScope();

    // An ordinary first parameter, once the C++ is erased.
    if (hasThis) {
        declareLocal("this", Layout::PointerSize, true);
        ++irf->paramCount;
    }
    for (std::size_t i = 0; i < f->params.size(); ++i) {
        VarDecl *p = f->params[i];
        const std::string pname = p->name.empty() ? "_" : p->name;
        declareLocal(pname, sizeOfType(p->type), true);
        localTypes[pname] = p->type;
        ++irf->paramCount;
    }
    irf->returnsValue = (f->retType != 0);

    emitPrologue(f);                        // virtual: a constructor's preamble
    if (f->body) lowerBlock(f->body);
    // A body that already returned emitted its tail on the path that left.
    if (!irf->endsWithTerminator()) {
        emitEpilogue(f);                    // virtual: a destructor's tail
        irf->emitReturn(IR_NoReg, f->line);
    }

    popScope();
    fn = savedFn;
    currentReturnType = savedReturn;
    slots.swap(savedSlots);
    localTypes.swap(savedTypes);
}

void Lowering::emitPrologue(Function *) {}
void Lowering::emitEpilogue(Function *) {}
void Lowering::emitScopeExit(CompoundStmt *) {}
void Lowering::emitAllOpenScopeExits() {}

// --- Statements ---

void Lowering::lowerBlock(CompoundStmt *block) {
    pushScope();
    openBlocks.push_back(block);
    for (std::size_t i = 0; i < block->body.size(); ++i) lowerStmt(block->body[i]);
    // If the block already returned, its destructors ran on that path.
    if (!fn->endsWithTerminator()) emitScopeExit(block);
    openBlocks.pop_back();
    popScope();
}

void Lowering::lowerStmt(Stmt *s) {
    if (!s) return;

    if (CompoundStmt *b = dynamic_cast<CompoundStmt*>(s)) { lowerBlock(b); return; }
    if (DeclStmt *ds = dynamic_cast<DeclStmt*>(s))        { lowerVarDecl(ds->var); return; }

    if (ExprStmt *es = dynamic_cast<ExprStmt*>(s)) {
        // The value is discarded, but the call still happens.
        if (CallExpr *call = dynamic_cast<CallExpr*>(es->expr)) lowerCall(call, false);
        else if (es->expr) lowerValue(es->expr);
        return;
    }

    if (ReturnStmt *rs = dynamic_cast<ReturnStmt*>(s)) {
        IRReg v = IR_NoReg;
        if (rs->expr) {
            v = lowerValue(rs->expr);
            v = convert(v, typeOf(rs->expr), currentReturnType, rs->line);
        }
        // Everything this return leaves is torn down first.
        emitAllOpenScopeExits();
        fn->emitReturn(v, rs->line);
        return;
    }

    if (IfStmt *is = dynamic_cast<IfStmt*>(s))    { lowerIf(is); return; }
    if (WhileStmt *ws = dynamic_cast<WhileStmt*>(s)) { lowerWhile(ws); return; }
    if (ForStmt *fs = dynamic_cast<ForStmt*>(s))  { lowerFor(fs); return; }

    if (dynamic_cast<BreakStmt*>(s)) {
        if (!breakTargets.empty()) fn->emitJump(breakTargets.back(), s->line);
        return;
    }
    if (dynamic_cast<ContinueStmt*>(s)) {
        if (!continueTargets.empty()) fn->emitJump(continueTargets.back(), s->line);
        return;
    }
}

void Lowering::lowerVarDecl(VarDecl *vd) {
    if (!vd) return;
    const int size = sizeOfType(vd->type);
    const int slot = declareLocal(vd->name, size, false);
    localTypes[vd->name] = vd->type;
    if (vd->init) {
        // A reference stores the ADDRESS of what it binds to -- the whole of
        // what a reference becomes.  The DECLARED type decides that, not the
        // initialiser: `Base& r = obj;` binds to obj, it does not copy it.
        IRReg value;
        if (isReferenceType(vd->type)) {
            value = lowerAddress(vd->init);
        } else {
            value = lowerValue(vd->init);
            value = convert(value, typeOf(vd->init), vd->type, vd->line);
        }
        const IRReg addr = fn->emitLocalAddr(slot, vd->line);
        fn->emitStore(addr, value, size, isFloatType(vd->type), vd->line);
    }
}

void Lowering::lowerIf(IfStmt *s) {
    const int elseLabel = fn->newLabel();
    const int endLabel = s->elseBranch ? fn->newLabel() : elseLabel;

    const IRReg cond = lowerValue(s->cond);
    fn->emitBranchZero(cond, elseLabel, s->line);
    lowerStmt(s->thenBranch);
    if (s->elseBranch) {
        fn->emitJump(endLabel, s->line);
        fn->emitLabel(elseLabel);
        lowerStmt(s->elseBranch);
        fn->emitLabel(endLabel);
    } else {
        fn->emitLabel(elseLabel);
    }
}

void Lowering::lowerWhile(WhileStmt *s) {
    const int top = fn->newLabel();
    const int done = fn->newLabel();

    fn->emitLabel(top);
    const IRReg cond = lowerValue(s->cond);
    fn->emitBranchZero(cond, done, s->line);

    breakTargets.push_back(done);
    continueTargets.push_back(top);
    lowerStmt(s->body);
    continueTargets.pop_back();
    breakTargets.pop_back();

    fn->emitJump(top, s->line);
    fn->emitLabel(done);
}

void Lowering::lowerFor(ForStmt *s) {
    // The init declaration belongs to the loop.
    pushScope();
    if (s->init) lowerStmt(s->init);

    const int top = fn->newLabel();
    const int step = fn->newLabel();
    const int done = fn->newLabel();

    fn->emitLabel(top);
    if (s->cond) {
        const IRReg cond = lowerValue(s->cond);
        fn->emitBranchZero(cond, done, s->line);
    }

    breakTargets.push_back(done);
    continueTargets.push_back(step);        // continue runs the step first
    lowerStmt(s->body);
    continueTargets.pop_back();
    breakTargets.pop_back();

    fn->emitLabel(step);
    if (s->step) lowerValue(s->step);
    fn->emitJump(top, s->line);
    fn->emitLabel(done);
    popScope();
}

// --- Expressions ---

IRReg Lowering::lowerAddress(Expr *e) {
    if (!e) return IR_NoReg;

    IRReg out = IR_NoReg;
    if (lowerLayerAddress(e, out)) return out;      // virtual: a.b, this, ...

    if (IdentExpr *id = dynamic_cast<IdentExpr*>(e)) {
        const int slot = findSlot(id->name);
        if (slot >= 0) {
            const IRReg addr = fn->emitLocalAddr(slot, e->line);
            // A reference's slot holds another object's address, so the address
            // OF the reference is the value IN its slot.
            if (isReferenceExpr(e)) return fn->emitLoad(addr, Layout::PointerSize, false, e->line);
            return addr;
        }
        return fn->emitGlobalAddr(id->name, e->line);
    }

    if (UnaryExpr *u = dynamic_cast<UnaryExpr*>(e)) {
        if (u->op == UN_Deref) return lowerValue(u->operand);   // *p: p's value
    }

    if (BinaryExpr *b = dynamic_cast<BinaryExpr*>(e)) {
        // An assignment is an lvalue; its address is the left side's.
        if (b->op == BIN_Assign) { lowerAssign(b); return lowerAddress(b->lhs); }
    }

    diag.error(e->line, e->col, "internal: expression has no address to lower");
    return fn->emitConst(0, e->line);
}

IRReg Lowering::lowerValue(Expr *e) {
    if (!e) return IR_NoReg;

    IRReg out = IR_NoReg;
    if (lowerLayerValue(e, out)) return out;        // virtual: this, new, a.b

    if (NumberExpr *n = dynamic_cast<NumberExpr*>(e)) {
        return fn->emitConst(n->value, e->line);
    }
    if (FloatExpr *f = dynamic_cast<FloatExpr*>(e)) {
        return fn->emitFConst(f->value, e->line);
    }
    if (StringExpr *str = dynamic_cast<StringExpr*>(e)) {
        return fn->emitStringAddr(mod.internString(str->value), e->line);
    }

    if (IdentExpr *id = dynamic_cast<IdentExpr*>(e)) {
        const IRReg addr = lowerAddress(e);
        Type *t = typeOf(e);
        (void)id;
        return fn->emitLoad(addr, t ? sizeOfType(t) : Layout::IntSize,
                            isFloatType(t), e->line);
    }

    if (CallExpr *call = dynamic_cast<CallExpr*>(e)) return lowerCall(call, true);
    if (UnaryExpr *u = dynamic_cast<UnaryExpr*>(e))  return lowerUnary(u);
    if (BinaryExpr *b = dynamic_cast<BinaryExpr*>(e)) return lowerBinary(b);

    diag.error(e->line, e->col, "internal: unhandled expression in lowering");
    return fn->emitConst(0, e->line);
}

IRReg Lowering::lowerUnary(UnaryExpr *e) {
    switch (e->op) {
    case UN_Neg: {
        BuiltinKind k;
        const bool flt = arithKind(typeOf(e->operand), k) && builtinIsFloating(k);
        return fn->emitUnary(flt ? IR_FNeg : IR_Neg, lowerValue(e->operand), e->line);
    }
    case UN_Not:    return fn->emitUnary(IR_LogicalNot, lowerValue(e->operand), e->line);
    case UN_AddrOf: return lowerAddress(e->operand);        // &x IS the address
    case UN_Deref: {
        const IRReg addr = lowerValue(e->operand);
        Type *t = typeOf(e);
        return fn->emitLoad(addr, t ? sizeOfType(t) : Layout::IntSize,
                            isFloatType(t), e->line);
    }
    }
    return IR_NoReg;
}

// Both operands are converted to the type they meet in before the operator
// runs, and the operator chosen depends on that type: integer, unsigned and
// floating arithmetic are three different machine operations.
IRReg Lowering::lowerBinary(BinaryExpr *e) {
    if (e->op == BIN_Assign) return lowerAssign(e);
    if (e->op == BIN_LAnd || e->op == BIN_LOr) return lowerShortCircuit(e);

    Type *lt = typeOf(e->lhs);
    Type *rt = typeOf(e->rhs);
    IRReg a = lowerValue(e->lhs);
    IRReg b = lowerValue(e->rhs);

    // Pointer arithmetic counts objects, not bytes, so the integer side is
    // scaled by the pointee's size before the add.  This is the whole of what
    // makes a[i] reach element i rather than byte i.
    PointerType *pl = dynamic_cast<PointerType*>(lt);
    PointerType *pr = dynamic_cast<PointerType*>(rt);
    if ((pl || pr) && (e->op == BIN_Add || e->op == BIN_Sub)) {
        if (pl && pr) {
            // p - q: the byte difference divided by the element size.
            const IRReg diff = fn->emitBinary(IR_Sub, a, b, e->line);
            const int step = sizeOfType(pl->base);
            if (step <= 1) return diff;
            const IRReg by = fn->emitConst(step, e->line);
            return fn->emitBinary(IR_Div, diff, by, e->line);
        }
        PointerType *p = pl ? pl : pr;
        IRReg &index = pl ? b : a;
        const int step = sizeOfType(p->base);
        if (step > 1) {
            const IRReg by = fn->emitConst(step, e->line);
            index = fn->emitBinary(IR_Mul, index, by, e->line);
        }
        return fn->emitBinary(e->op == BIN_Add ? IR_Add : IR_Sub, a, b, e->line);
    }

    BuiltinKind kl, kr;
    BuiltinKind common = BK_Int;
    if (arithKind(lt, kl) && arithKind(rt, kr)) {
        common = commonKind(kl, kr);
        a = convert(a, lt, commonType(common), e->line);
        b = convert(b, rt, commonType(common), e->line);
    }
    const bool flt = builtinIsFloating(common);
    const bool uns = !builtinIsSigned(common);

    IROp op = IR_Add;
    switch (e->op) {
    case BIN_Add: op = flt ? IR_FAdd : IR_Add; break;
    case BIN_Sub: op = flt ? IR_FSub : IR_Sub; break;
    case BIN_Mul: op = flt ? IR_FMul : IR_Mul; break;
    case BIN_Div: op = flt ? IR_FDiv : (uns ? IR_UDiv : IR_Div); break;
    case BIN_Mod: op = uns ? IR_UMod : IR_Mod; break;
    case BIN_EQ:  op = flt ? IR_FCmpEQ : IR_CmpEQ; break;
    case BIN_NE:  op = flt ? IR_FCmpNE : IR_CmpNE; break;
    case BIN_LT:  op = flt ? IR_FCmpLT : (uns ? IR_UCmpLT : IR_CmpLT); break;
    case BIN_GT:  op = flt ? IR_FCmpGT : (uns ? IR_UCmpGT : IR_CmpGT); break;
    case BIN_LE:  op = flt ? IR_FCmpLE : (uns ? IR_UCmpLE : IR_CmpLE); break;
    case BIN_GE:  op = flt ? IR_FCmpGE : (uns ? IR_UCmpGE : IR_CmpGE); break;
    default: break;
    }
    return fn->emitBinary(op, a, b, e->line);
}

IRReg Lowering::lowerAssign(BinaryExpr *e) {
    // Right side first: the order the language leaves open, and the one that
    // keeps the address live for the shortest time.
    IRReg value = lowerValue(e->rhs);
    Type *t = typeOf(e->lhs);
    value = convert(value, typeOf(e->rhs), t, e->line);
    const IRReg addr = lowerAddress(e->lhs);
    fn->emitStore(addr, value, t ? sizeOfType(t) : Layout::IntSize,
                  isFloatType(t), e->line);
    return value;
}

// The right side must not run when the left already decides -- a control-flow
// fact, so it lowers to branches rather than an operator.
IRReg Lowering::lowerShortCircuit(BinaryExpr *e) {
    const int slot = fn->addLocal("__sc", Layout::IntSize, false);
    const int done = fn->newLabel();

    const IRReg left = lowerValue(e->lhs);
    IRReg addr = fn->emitLocalAddr(slot, e->line);
    fn->emitStore(addr, left, Layout::IntSize, false, e->line);

    if (e->op == BIN_LAnd) fn->emitBranchZero(left, done, e->line);
    else                   fn->emitBranchNZ(left, done, e->line);

    const IRReg right = lowerValue(e->rhs);
    addr = fn->emitLocalAddr(slot, e->line);
    fn->emitStore(addr, right, Layout::IntSize, false, e->line);

    fn->emitLabel(done);
    addr = fn->emitLocalAddr(slot, e->line);
    return fn->emitLoad(addr, Layout::IntSize, false, e->line);
}

IRReg Lowering::lowerCall(CallExpr *e, bool wantsResult) {
    IdentExpr *callee = dynamic_cast<IdentExpr*>(e->callee);
    if (!callee) {
        diag.error(e->line, e->col, "internal: unsupported callee in lowering");
        return fn->emitConst(0, e->line);
    }
    std::map<std::string, Function*>::const_iterator it = functions.find(callee->name);
    Function *target = (it == functions.end()) ? 0 : it->second;
    return fn->emitCall(callee->name, lowerArgs(e, target, 0), wantsResult, e->line);
}

// Each argument is converted to its parameter's declared type.  Without this a
// double handed to an int parameter would arrive as raw bits.  `skip` is 1 when
// the callee is a method and `this` already occupies the first slot.
std::vector<IRReg> Lowering::lowerArgs(CallExpr *e, Function *target, std::size_t skip) {
    std::vector<IRReg> args;
    for (std::size_t i = 0; i < e->args.size(); ++i) {
        const std::size_t p = i + skip;
        Type *want = (target && p < target->params.size()) ? target->params[p]->type : 0;
        IRReg v;
        if (want && isReferenceType(want)) {
            // A reference parameter receives the object's address, never a
            // copy of its bytes.
            v = lowerAddress(e->args[i]);
        } else {
            v = lowerValue(e->args[i]);
            if (want) v = convert(v, typeOf(e->args[i]), want, e->line);
        }
        args.push_back(v);
    }
    return args;
}

bool Lowering::lowerLayerValue(Expr *, IRReg &)   { return false; }
bool Lowering::lowerLayerAddress(Expr *, IRReg &) { return false; }

} // namespace cc
