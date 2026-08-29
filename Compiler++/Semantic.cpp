//
//  Semantic.cpp
//  Compiler++
//
//  Created by G. R. Akhtar on 29/08/2026.
//
//  C++98 only.  See Semantic.h for how this pass spans both class layers.

#include "Semantic.h"

#include <cstddef>
#include <sstream>

SemanticAnalyzer::SemanticAnalyzer(Diagnostics &d)
    : diag(d), currentReturnType(0), currentIsCtorOrDtor(false), loopDepth(0) {}

SemanticAnalyzer::~SemanticAnalyzer() {
    for (std::size_t i = 0; i < ownedTypes.size(); ++i) delete ownedTypes[i];
}

void SemanticAnalyzer::error(cc::ASTNode *at, const std::string &msg) {
    if (at) diag.error(at->line, at->col, msg);
    else    diag.error(0, 0, msg);
}

cc::Type *SemanticAnalyzer::makeBuiltin(const std::string &name) {
    cc::Type *t = new cc::BuiltinType(name);
    ownedTypes.push_back(t);
    return t;
}

cc::Type *SemanticAnalyzer::cloneType(cc::Type *t) {
    if (!t) return 0;
    cc::BuiltinType *bt = dynamic_cast<cc::BuiltinType*>(t);
    if (bt) return new cc::BuiltinType(bt->name);
    cxx::ClassType *ct = dynamic_cast<cxx::ClassType*>(t);
    if (ct) return new cxx::ClassType(ct->className);
    cc::PointerType *pt = dynamic_cast<cc::PointerType*>(t);
    if (pt) return new cc::PointerType(cloneType(pt->base));
    cxx::ReferenceType *rt = dynamic_cast<cxx::ReferenceType*>(t);
    if (rt) return new cxx::ReferenceType(cloneType(rt->base));
    return 0;
}

cc::Type *SemanticAnalyzer::makePointerTo(cc::Type *t) {
    cc::Type *p = new cc::PointerType(cloneType(t));
    ownedTypes.push_back(p);
    return p;
}

// --- Type helpers ---

cc::Type *SemanticAnalyzer::stripReference(cc::Type *t) {
    cxx::ReferenceType *rt = dynamic_cast<cxx::ReferenceType*>(t);
    return rt ? rt->base : t;
}

std::string SemanticAnalyzer::describe(cc::Type *t) {
    if (!t) return "<none>";
    cc::BuiltinType *bt = dynamic_cast<cc::BuiltinType*>(t);
    if (bt) return bt->name;
    cxx::ClassType *ct = dynamic_cast<cxx::ClassType*>(t);
    if (ct) return ct->className;
    cc::PointerType *pt = dynamic_cast<cc::PointerType*>(t);
    if (pt) return describe(pt->base) + "*";
    cxx::ReferenceType *rt = dynamic_cast<cxx::ReferenceType*>(t);
    if (rt) return describe(rt->base) + "&";
    return "<type>";
}

std::string SemanticAnalyzer::countText(std::size_t n) {
    std::ostringstream ss;
    ss << n;
    return ss.str();
}

bool SemanticAnalyzer::isVoid(cc::Type *t) {
    cc::BuiltinType *bt = dynamic_cast<cc::BuiltinType*>(stripReference(t));
    return bt && bt->name == "void";
}

bool SemanticAnalyzer::sameType(cc::Type *a, cc::Type *b) {
    a = stripReference(a);
    b = stripReference(b);
    if (!a || !b) return true;          // an earlier error already spoke
    cc::BuiltinType *ba = dynamic_cast<cc::BuiltinType*>(a);
    cc::BuiltinType *bb = dynamic_cast<cc::BuiltinType*>(b);
    if (ba && bb) return ba->name == bb->name;
    cxx::ClassType *ca = dynamic_cast<cxx::ClassType*>(a);
    cxx::ClassType *cb = dynamic_cast<cxx::ClassType*>(b);
    if (ca && cb) return ca->className == cb->className;
    cc::PointerType *pa = dynamic_cast<cc::PointerType*>(a);
    cc::PointerType *pb = dynamic_cast<cc::PointerType*>(b);
    if (pa && pb) return sameType(pa->base, pb->base);
    return false;                       // different shapes entirely
}

cxx::ClassDecl *SemanticAnalyzer::classOf(cc::Type *t) {
    t = stripReference(t);
    cc::PointerType *pt = dynamic_cast<cc::PointerType*>(t);
    if (pt) t = pt->base;
    cxx::ClassType *ct = dynamic_cast<cxx::ClassType*>(t);
    return ct ? findClass(ct->className) : 0;
}

// A Derived object begins with its Base subobject at offset 0, so the upcast
// costs nothing.  The reverse is not allowed: a Base* need not be a Derived.
bool SemanticAnalyzer::canConvert(cc::Type *from, cc::Type *to) {
    if (sameType(from, to)) return true;
    if (!from || !to) return true;              // an earlier error already spoke

    cc::Type *f = stripReference(from);
    cc::Type *t = stripReference(to);

    // Derived* -> Base*
    cc::PointerType *pf = dynamic_cast<cc::PointerType*>(f);
    cc::PointerType *ptt = dynamic_cast<cc::PointerType*>(t);
    if (pf && ptt) {
        cxx::ClassDecl *df = classOf(pf->base);
        cxx::ClassDecl *dt = classOf(ptt->base);
        if (df && dt) return isDerivedFrom(df, dt);
        return false;
    }

    // Derived -> Base& , and Derived -> Base
    cxx::ClassDecl *cf = classOf(f);
    cxx::ClassDecl *ctd = classOf(t);
    if (cf && ctd) return isDerivedFrom(cf, ctd);

    return false;
}

// Its TYPE is int, so no type-only rule can accept it where a pointer is
// wanted -- the expression itself has to be looked at.
bool SemanticAnalyzer::isNullPointerConstant(cc::Expr *e) {
    cc::NumberExpr *n = dynamic_cast<cc::NumberExpr*>(e);
    return n && n->value == 0;
}

bool SemanticAnalyzer::convertible(cc::Expr *fromExpr, cc::Type *from, cc::Type *to) {
    if (canConvert(from, to)) return true;
    if (dynamic_cast<cc::PointerType*>(stripReference(to)) && isNullPointerConstant(fromExpr)) {
        return true;
    }
    return false;
}

// int, int*, int** need no resolution; a class name does.
void SemanticAnalyzer::checkTypeIsKnown(cc::Type *t, cc::ASTNode *at, const std::string &where) {
    if (!t) return;
    cc::PointerType *pt = dynamic_cast<cc::PointerType*>(t);
    if (pt) { checkTypeIsKnown(pt->base, at, where); return; }
    cxx::ReferenceType *rt = dynamic_cast<cxx::ReferenceType*>(t);
    if (rt) { checkTypeIsKnown(rt->base, at, where); return; }
    cxx::ClassType *ct = dynamic_cast<cxx::ClassType*>(t);
    if (ct && !findClass(ct->className)) {
        error(at, "unknown type '" + ct->className + "' in " + where);
    }
}

// --- Classes and their members ---

cxx::ClassDecl *SemanticAnalyzer::findClass(const std::string &name) {
    std::map<std::string, cxx::ClassDecl*>::iterator it = classes.find(name);
    return (it == classes.end()) ? 0 : it->second;
}

void SemanticAnalyzer::resolveBases() {
    std::map<std::string, cxx::ClassDecl*>::iterator it;
    for (it = classes.begin(); it != classes.end(); ++it) {
        cxx::ClassDecl *cd = it->second;
        if (cd->baseName.empty()) continue;
        cxx::ClassDecl *base = findClass(cd->baseName);
        if (!base) {
            error(cd, "unknown base class '" + cd->baseName + "' for class '" + cd->name + "'");
            continue;
        }
        if (base == cd) {
            error(cd, "class '" + cd->name + "' cannot inherit from itself");
            continue;
        }
        cd->base = base;
    }

    // A chain longer than the number of classes must have revisited one.
    for (it = classes.begin(); it != classes.end(); ++it) {
        cxx::ClassDecl *cd = it->second;
        std::size_t steps = 0;
        for (cxx::ClassDecl *p = cd->base; p; p = p->base) {
            if (p == cd || ++steps > classes.size()) {
                error(cd, "inheritance cycle involving class '" + cd->name + "'");
                cd->base = 0;               // break it, so later walks terminate
                break;
            }
        }
    }
}

// Most-derived first: the first match wins, which IS name hiding.
cc::Decl *SemanticAnalyzer::findMember(cxx::ClassDecl *cd, const std::string &member,
                                       cxx::ClassDecl **foundIn) {
    for (cxx::ClassDecl *c = cd; c; c = c->base) {
        for (std::size_t i = 0; i < c->members.size(); ++i) {
            cxx::FieldDecl *fd = dynamic_cast<cxx::FieldDecl*>(c->members[i]);
            if (fd && fd->name == member) { if (foundIn) *foundIn = c; return fd; }
            cxx::MethodDecl *md = dynamic_cast<cxx::MethodDecl*>(c->members[i]);
            // Neither takes part in ordinary name lookup.
            if (md && !md->isConstructor && !md->isDestructor && md->name == member) {
                if (foundIn) *foundIn = c;
                return md;
            }
        }
    }
    return 0;
}

bool SemanticAnalyzer::isDerivedFrom(cxx::ClassDecl *derived, cxx::ClassDecl *base) {
    for (cxx::ClassDecl *c = derived; c; c = c->base) {
        if (c == base) return true;
    }
    return false;
}

bool SemanticAnalyzer::memberIsAccessible(cc::Decl *m, cxx::ClassDecl *owner) const {
    const cxx::Access a = memberAccess(m);
    if (a == cxx::ACC_Public) return true;
    if (currentClass.empty()) return false;
    // const, so findClass() is off limits.
    std::map<std::string, cxx::ClassDecl*>::const_iterator it = classes.find(currentClass);
    cxx::ClassDecl *from = (it == classes.end()) ? 0 : it->second;
    if (!from) return false;
    if (from == owner) return true;                 // inside the class itself
    if (a == cxx::ACC_Protected) return isDerivedFrom(from, owner);
    return false;                                   // private, from outside
}

cxx::ClassDecl *SemanticAnalyzer::ownerClassOf(cc::Decl *m) {
    cxx::FieldDecl *fd = dynamic_cast<cxx::FieldDecl*>(m);
    if (fd) return findClass(fd->ownerClass);
    cxx::MethodDecl *md = dynamic_cast<cxx::MethodDecl*>(m);
    if (md) return findClass(md->ownerClass);
    return 0;
}

bool SemanticAnalyzer::sameSignature(cc::Function *a, cc::Function *b) {
    if (a->params.size() != b->params.size()) return false;
    for (std::size_t i = 0; i < a->params.size(); ++i) {
        if (!sameType(a->params[i]->type, b->params[i]->type)) return false;
    }
    return true;
}

cxx::Access SemanticAnalyzer::memberAccess(cc::Decl *m) {
    cxx::FieldDecl *fd = dynamic_cast<cxx::FieldDecl*>(m);
    if (fd) return fd->access;
    cxx::MethodDecl *md = dynamic_cast<cxx::MethodDecl*>(m);
    if (md) return md->access;
    return cxx::ACC_Public;
}

cc::Type *SemanticAnalyzer::memberType(cc::Decl *m) {
    cxx::FieldDecl *fd = dynamic_cast<cxx::FieldDecl*>(m);
    if (fd) return fd->type;
    cxx::MethodDecl *md = dynamic_cast<cxx::MethodDecl*>(m);
    if (md) return md->retType;
    return 0;
}

// Same name and matching signature over a base virtual is an override.  Same
// name, different signature is legal but almost always a mistake, so it warns.
void SemanticAnalyzer::resolveOverrides(cxx::ClassDecl *cd) {
    if (!cd->base) return;

    // A destructor overrides by POSITION: ~Derived and ~Base are spelled
    // differently but occupy the same vtable slot.
    if (cd->dtor) {
        for (cxx::ClassDecl *b = cd->base; b; b = b->base) {
            if (!b->dtor) continue;
            if (b->dtor->isVirtual) {
                cd->dtor->overrides = b->dtor;
                cd->dtor->isVirtual = true;
            }
            break;
        }
    }

    for (std::size_t i = 0; i < cd->members.size(); ++i) {
        cxx::MethodDecl *md = dynamic_cast<cxx::MethodDecl*>(cd->members[i]);
        if (!md || md->isConstructor || md->isDestructor) continue;
        cc::Decl *found = findMember(cd->base, md->name);
        cxx::MethodDecl *bm = dynamic_cast<cxx::MethodDecl*>(found);
        if (!bm) continue;

        if (!sameSignature(md, bm)) {
            diag.warning(md->line, md->col,
                         "'" + md->name + "' hides '" + bm->ownerClass + "::" + bm->name
                         + "' rather than overriding it; the parameter lists differ");
            continue;
        }
        if (!bm->isVirtual) continue;       // hiding a non-virtual is not an override
        if (!sameType(md->retType, bm->retType)) {
            error(md, "'" + md->name + "' overrides '" + bm->ownerClass + "::" + bm->name
                      + "' but returns " + describe(md->retType)
                      + " instead of " + describe(bm->retType));
            continue;
        }
        // Virtualness propagates down the chain whether or not `virtual` is
        // repeated.
        md->overrides = bm;
        md->isVirtual = true;
    }
}

// Most-derived first, so insert() -- which refuses a duplicate -- keeps the
// derived member.  That is name hiding, for free.
void SemanticAnalyzer::pushClassScope(cxx::ClassDecl *cd) {
    symbols.pushScope();
    for (cxx::ClassDecl *c = cd; c; c = c->base) {
        for (std::size_t i = 0; i < c->members.size(); ++i) {
            cxx::FieldDecl *fd = dynamic_cast<cxx::FieldDecl*>(c->members[i]);
            if (fd) {
                Symbol *s = new Symbol(SYM_Field, fd->name, fd, fd->type);
                if (!symbols.insert(fd->name, s)) delete s;
                continue;
            }
            cxx::MethodDecl *md = dynamic_cast<cxx::MethodDecl*>(c->members[i]);
            if (md && !md->isConstructor && !md->isDestructor) {
                Symbol *s = new Symbol(SYM_Method, md->name, md, md->retType);
                if (!symbols.insert(md->name, s)) delete s;
            }
        }
    }
}

// Only a class object whose class or some base declares one.  A pointer never
// does -- `delete` is explicit for a reason.
bool SemanticAnalyzer::hasDestructor(cc::Type *t) {
    if (!t) return false;
    if (dynamic_cast<cc::PointerType*>(t)) return false;
    if (dynamic_cast<cxx::ReferenceType*>(t)) return false;   // a reference owns nothing
    cxx::ClassType *ct = dynamic_cast<cxx::ClassType*>(t);
    if (!ct) return false;
    for (cxx::ClassDecl *c = findClass(ct->className); c; c = c->base) {
        if (c->dtor) return true;
    }
    return false;
}

// Rules about a class as a whole.
void SemanticAnalyzer::checkClassInvariants(cxx::ClassDecl *cd) {
    // A constructor runs BEFORE the object has a vtable pointer to dispatch
    // through -- it is what puts one there -- so there is nothing for
    // `virtual` to mean on it.  Cleared FIRST, so the polymorphism test below
    // is not fooled by a keyword that should never have been there.
    for (std::size_t i = 0; i < cd->ctors.size(); ++i) {
        if (cd->ctors[i]->isVirtual) {
            error(cd->ctors[i], "a constructor cannot be virtual");
            cd->ctors[i]->isVirtual = false;
        }
    }

    if (cd->dtor && !cd->dtor->params.empty()) {
        error(cd->dtor, "a destructor cannot take parameters");
    }

    // A polymorphic class is meant to be used through a base pointer, and
    // deleting through one with a non-virtual destructor runs the wrong one.
    bool polymorphic = false;
    for (cxx::ClassDecl *c = cd; c && !polymorphic; c = c->base) {
        for (std::size_t i = 0; i < c->members.size(); ++i) {
            cxx::MethodDecl *md = dynamic_cast<cxx::MethodDecl*>(c->members[i]);
            if (md && md->isVirtual && !md->isDestructor && !md->isConstructor) {
                polymorphic = true;
                break;
            }
        }
    }
    if (polymorphic && cd->dtor && !cd->dtor->isVirtual) {
        diag.warning(cd->dtor->line, cd->dtor->col,
                     "class '" + cd->name + "' has virtual functions but its destructor is not "
                     "virtual; deleting through a base pointer would not run it");
    }
}

// Each name is a field OF THIS CLASS or the base's own name.  An inherited
// field is the base constructor's job, which is why the base gets an entry.
void SemanticAnalyzer::analyzeMemberInits(cxx::MethodDecl *ctor, cxx::ClassDecl *cd) {
    std::vector<std::string> seen;
    int lastFieldIndex = -1;
    bool outOfOrder = false;

    for (std::size_t i = 0; i < ctor->memberInits.size(); ++i) {
        cxx::MemberInit &mi = ctor->memberInits[i];

        for (std::size_t j = 0; j < seen.size(); ++j) {
            if (seen[j] == mi.name) {
                diag.error(mi.line, mi.col, "'" + mi.name + "' is initialised more than once");
            }
        }
        seen.push_back(mi.name);

        // the base class, by name
        if (cd->base && mi.name == cd->base->name) {
            mi.isBase = true;
            if (i != 0) {
                diag.warning(mi.line, mi.col,
                             "the base class initialiser is written after a member, but the base "
                             "is always constructed first");
            }
            for (std::size_t a = 0; a < mi.args.size(); ++a) {
                bool lv = false;
                analyzeExpr(mi.args[a], lv);
            }
            selectConstructor(cd->base, mi.args.size(), ctor,
                              "base class '" + cd->base->name + "'");
            continue;
        }

        // otherwise a field declared in THIS class
        int fieldIndex = -1;
        cxx::FieldDecl *field = 0;
        int counter = 0;
        for (std::size_t m = 0; m < cd->members.size(); ++m) {
            cxx::FieldDecl *fd = dynamic_cast<cxx::FieldDecl*>(cd->members[m]);
            if (!fd) continue;
            if (fd->name == mi.name) { field = fd; fieldIndex = counter; break; }
            ++counter;
        }
        if (!field) {
            cxx::ClassDecl *owner = 0;
            if (cd->base && findMember(cd->base, mi.name, &owner)) {
                diag.error(mi.line, mi.col,
                           "'" + mi.name + "' is inherited from '" + owner->name
                           + "' and cannot be initialised here; initialise the base class instead");
            } else {
                diag.error(mi.line, mi.col,
                           "'" + mi.name + "' is not a member of class '" + cd->name + "'");
            }
            continue;
        }

        if (fieldIndex < lastFieldIndex) outOfOrder = true;
        lastFieldIndex = fieldIndex;

        if (mi.args.size() != 1) {
            if (mi.args.empty() && hasDestructor(field->type)) continue;   // default-construct
            if (mi.args.size() > 1) {
                diag.error(mi.line, mi.col,
                           "'" + mi.name + "' takes one initialiser value");
                continue;
            }
        }
        for (std::size_t a = 0; a < mi.args.size(); ++a) {
            bool lv = false;
            cc::Type *at = analyzeExpr(mi.args[a], lv);
            if (at && !convertible(mi.args[a], at, field->type)) {
                diag.error(mi.line, mi.col,
                           "cannot initialise '" + describe(field->type) + " " + mi.name
                           + "' from an expression of type " + describe(at));
            }
        }
    }

    // Members are constructed in DECLARATION order, never in the order the
    // list is written.  The bug only shows when one initialiser reads another.
    if (outOfOrder) {
        diag.warning(ctor->line, ctor->col,
                     "the initialiser list is written out of declaration order; members are "
                     "always constructed in the order they are declared");
    }
}

cxx::MethodDecl *SemanticAnalyzer::selectConstructor(cxx::ClassDecl *cd, std::size_t argCount,
                                                     cc::ASTNode *at, const std::string &what) {
    if (!cd) return 0;
    // No constructors at all is legal: the fields are uninitialised, as in C.
    if (cd->ctors.empty()) {
        if (argCount > 0) {
            error(at, "class '" + cd->name + "' has no constructor, so " + what
                      + " cannot take arguments");
        }
        return 0;
    }
    // By argument COUNT.  Full overload resolution is a later job.
    cxx::MethodDecl *found = 0;
    int matches = 0;
    for (std::size_t i = 0; i < cd->ctors.size(); ++i) {
        if (cd->ctors[i]->params.size() == argCount) { found = cd->ctors[i]; ++matches; }
    }
    if (matches == 0) {
        error(at, "class '" + cd->name + "' has no constructor taking "
                  + countText(argCount) + " argument(s), needed for " + what);
        return 0;
    }
    if (matches > 1) {
        error(at, "more than one constructor of class '" + cd->name
                  + "' takes the same number of arguments");
    }
    return found;
}

// --- Entry point ---

void SemanticAnalyzer::analyze(const std::vector<cc::Decl*> &units) {
    // 1) every class name, so declarations may refer to a class defined later
    collectClasses(units);
    // 2) link the hierarchy, before anything tries to look a member up
    resolveBases();
    // 3) work out which methods override which, so virtualness is known before
    //    any body is analysed
    for (std::size_t i = 0; i < units.size(); ++i) {
        cxx::ClassDecl *cd = dynamic_cast<cxx::ClassDecl*>(units[i]);
        if (cd) resolveOverrides(cd);
    }
    // 3b) whole-class rules, once virtualness is settled
    for (std::size_t i = 0; i < units.size(); ++i) {
        cxx::ClassDecl *cd = dynamic_cast<cxx::ClassDecl*>(units[i]);
        if (cd) checkClassInvariants(cd);
    }
    // 4) every top-level name, so functions may call each other in any order
    declareTopLevel(units);
    // 5) bodies
    for (std::size_t i = 0; i < units.size(); ++i) analyzeDecl(units[i]);
}

void SemanticAnalyzer::collectClasses(const std::vector<cc::Decl*> &units) {
    for (std::size_t i = 0; i < units.size(); ++i) {
        cxx::ClassDecl *cd = dynamic_cast<cxx::ClassDecl*>(units[i]);
        if (!cd) continue;
        if (findClass(cd->name)) {
            error(cd, "class '" + cd->name + "' is already defined");
            continue;
        }
        classes[cd->name] = cd;
        cc::Type *ct = new cxx::ClassType(cd->name);
        ownedTypes.push_back(ct);
        Symbol *s = new Symbol(SYM_Type, cd->name, cd, ct);
        if (!symbols.insert(cd->name, s)) {
            error(cd, "'" + cd->name + "' is already declared");
            delete s;
        }
    }
}

void SemanticAnalyzer::declareTopLevel(const std::vector<cc::Decl*> &units) {
    for (std::size_t i = 0; i < units.size(); ++i) {
        cc::Function *fn = dynamic_cast<cc::Function*>(units[i]);
        if (fn) {
            Symbol *s = new Symbol(SYM_Method, fn->name, fn, fn->retType);
            if (!symbols.insert(fn->name, s)) {
                error(fn, "function '" + fn->name + "' is already declared");
                delete s;
            }
            continue;
        }
        cc::VarDecl *vd = dynamic_cast<cc::VarDecl*>(units[i]);
        if (vd) {
            Symbol *s = new Symbol(SYM_Var, vd->name, vd, vd->type);
            if (!symbols.insert(vd->name, s)) {
                error(vd, "variable '" + vd->name + "' is already declared");
                delete s;
            }
            continue;
        }
    }
}

void SemanticAnalyzer::analyzeDecl(cc::Decl *d) {
    if (!d) return;
    cxx::ClassDecl *cd = dynamic_cast<cxx::ClassDecl*>(d);
    if (cd) { analyzeClass(cd); return; }
    // MethodDecl IS a Function, so this one test covers both.
    cc::Function *fn = dynamic_cast<cc::Function*>(d);
    if (fn) { analyzeFunction(fn); return; }
    cc::VarDecl *vd = dynamic_cast<cc::VarDecl*>(d);
    if (vd) { analyzeVarDecl(vd, false); return; }
}

void SemanticAnalyzer::analyzeClass(cxx::ClassDecl *cd) {
    // A field may not shadow a base field: legal C++, but in a teaching subset
    // it is far more often a mistake than an intention.
    if (cd->base) {
        for (std::size_t i = 0; i < cd->members.size(); ++i) {
            cxx::FieldDecl *fd = dynamic_cast<cxx::FieldDecl*>(cd->members[i]);
            if (!fd) continue;
            cxx::ClassDecl *owner = 0;
            cc::Decl *hidden = findMember(cd->base, fd->name, &owner);
            if (hidden && dynamic_cast<cxx::FieldDecl*>(hidden)) {
                diag.warning(fd->line, fd->col,
                             "field '" + fd->name + "' hides '" + owner->name
                             + "::" + fd->name + "'");
            }
        }
    }
    for (std::size_t i = 0; i < cd->members.size(); ++i) {
        cxx::FieldDecl *fd = dynamic_cast<cxx::FieldDecl*>(cd->members[i]);
        if (fd) {
            checkTypeIsKnown(fd->type, fd, "field '" + fd->name + "'");
            if (isVoid(fd->type)) error(fd, "field '" + fd->name + "' cannot have type void");
            continue;
        }
        cxx::MethodDecl *md = dynamic_cast<cxx::MethodDecl*>(cd->members[i]);
        if (md) analyzeFunction(md);
    }
}

// One routine for a free function and a method: a method IS a function, and
// differs only by having a class scope pushed underneath its parameters.
void SemanticAnalyzer::analyzeFunction(cc::Function *fn) {
    cxx::MethodDecl *md = dynamic_cast<cxx::MethodDecl*>(fn);

    checkTypeIsKnown(fn->retType, fn, "return type of '" + fn->name + "'");

    cc::Type *savedReturn = currentReturnType;
    const std::string savedClass = currentClass;
    const bool savedCtorDtor = currentIsCtorOrDtor;
    currentReturnType = fn->retType;
    currentClass = md ? md->ownerClass : std::string();
    currentIsCtorOrDtor = md && (md->isConstructor || md->isDestructor);

    if (md) pushClassScope(findClass(md->ownerClass));

    symbols.pushScope();                        // parameters
    for (std::size_t i = 0; i < fn->params.size(); ++i) {
        cc::VarDecl *p = fn->params[i];
        checkTypeIsKnown(p->type, p, "parameter '" + p->name + "'");
        if (isVoid(p->type)) error(p, "parameter '" + p->name + "' cannot have type void");
        if (p->name.empty()) continue;
        Symbol *s = new Symbol(SYM_Var, p->name, p, p->type);
        if (!symbols.insert(p->name, s)) {
            error(p, "duplicate parameter name '" + p->name + "'");
            delete s;
        }
    }

    // The initialiser list is checked INSIDE the parameter scope, because that
    // is where its arguments live:  Point(int a) : x(a) { }
    if (md && md->isConstructor) analyzeMemberInits(md, findClass(md->ownerClass));

    if (fn->body) analyzeBlock(fn->body);

    symbols.popScope();                         // parameters
    if (md) symbols.popScope();                 // class members

    currentReturnType = savedReturn;
    currentClass = savedClass;
    currentIsCtorOrDtor = savedCtorDtor;
}

// --- Statements ---

void SemanticAnalyzer::analyzeBlock(cc::CompoundStmt *block) {
    symbols.pushScope();
    std::vector<cc::VarDecl*> declared;
    for (std::size_t i = 0; i < block->body.size(); ++i) {
        analyzeStmt(block->body[i]);
        // Remember the class-typed locals, in the order they were declared.
        cc::DeclStmt *ds = dynamic_cast<cc::DeclStmt*>(block->body[i]);
        if (ds && ds->var) declared.push_back(ds->var);
    }
    recordScopeExitDestruction(block, declared);
    symbols.popScope();
}

// Exact reverse of construction.  Recording it here lets the lowering phase
// emit the calls on every path out without redoing the scoping.  With no
// exceptions, those paths are just: end of block, return, break.
void SemanticAnalyzer::recordScopeExitDestruction(cc::CompoundStmt *block,
                                                  const std::vector<cc::VarDecl*> &declared) {
    block->destroyAtExit.clear();
    for (std::size_t i = declared.size(); i > 0; --i) {
        cc::VarDecl *vd = declared[i - 1];
        if (hasDestructor(vd->type)) block->destroyAtExit.push_back(vd);
    }
}

void SemanticAnalyzer::analyzeVarDecl(cc::VarDecl *vd, bool declareIt) {
    if (!vd) return;
    checkTypeIsKnown(vd->type, vd, "declaration of '" + vd->name + "'");
    if (isVoid(vd->type)) error(vd, "variable '" + vd->name + "' cannot have type void");

    cc::Type *initType = 0;
    bool initIsLValue = false;
    if (vd->init) initType = analyzeExpr(vd->init, initIsLValue);

    // A reference binds a name to an existing object, so its initialiser must
    // denote one.  `int &s = 1;` has nothing to bind to.  This is what
    // analyzeExpr()'s isLValue result exists for.
    cxx::ReferenceType *rt = dynamic_cast<cxx::ReferenceType*>(vd->type);
    if (rt) {
        if (!vd->init) {
            error(vd, "reference '" + vd->name + "' must be initialised");
        } else if (initType && !initIsLValue) {
            error(vd, "cannot bind reference '" + vd->name + "' of type "
                      + describe(vd->type) + " to a non-lvalue initialiser");
        } else if (initType && !convertible(vd->init, initType, rt->base)) {
            error(vd, "cannot bind '" + describe(vd->type) + " " + vd->name
                      + "' to an initialiser of type " + describe(initType));
        }
    } else if (vd->init && initType && !convertible(vd->init, initType, vd->type)) {
        error(vd, "cannot initialise '" + describe(vd->type) + " " + vd->name
                  + "' from an expression of type " + describe(initType));
    }

    // A class-typed object is CONSTRUCTED.  `Point p;` needs a constructor
    // taking no arguments whenever the class declares any at all, and
    // `Point q(1, 2)` needs one taking two.
    cxx::ClassDecl *cls = 0;
    {
        cxx::ClassType *ct = dynamic_cast<cxx::ClassType*>(stripReference(vd->type));
        if (ct) cls = findClass(ct->className);
    }
    if (cls && !dynamic_cast<cxx::ReferenceType*>(vd->type)) {
        for (std::size_t i = 0; i < vd->ctorArgs.size(); ++i) {
            bool lv = false;
            analyzeExpr(vd->ctorArgs[i], lv);
        }
        if (!vd->init) {
            selectConstructor(cls, vd->ctorArgs.size(), vd,
                              "the declaration of '" + vd->name + "'");
        }
    } else if (vd->hasCtorArgs) {
        error(vd, "'" + describe(vd->type) + "' is not a class type, so '" + vd->name
                  + "' cannot be constructed with arguments");
    }

    if (!declareIt) return;
    if (symbols.lookupLocal(vd->name)) {
        error(vd, "redeclaration of '" + vd->name + "' in the same scope");
        return;
    }
    Symbol *s = new Symbol(SYM_Var, vd->name, vd, vd->type);
    if (!symbols.insert(vd->name, s)) delete s;
}

void SemanticAnalyzer::analyzeStmt(cc::Stmt *s) {
    if (!s) return;

    cc::CompoundStmt *block = dynamic_cast<cc::CompoundStmt*>(s);
    if (block) { analyzeBlock(block); return; }

    cc::DeclStmt *ds = dynamic_cast<cc::DeclStmt*>(s);
    if (ds) { analyzeVarDecl(ds->var, true); return; }

    cc::ExprStmt *es = dynamic_cast<cc::ExprStmt*>(s);
    if (es) { bool lv = false; analyzeExpr(es->expr, lv); return; }

    cc::ReturnStmt *rs = dynamic_cast<cc::ReturnStmt*>(s);
    if (rs) {
        bool lv = false;
        cc::Type *got = rs->expr ? analyzeExpr(rs->expr, lv) : 0;
        if (currentIsCtorOrDtor) {
            if (rs->expr) error(rs, "a constructor or destructor cannot return a value");
            return;
        }
        if (!currentReturnType) return;
        if (!rs->expr) {
            if (!isVoid(currentReturnType)) {
                error(rs, "return with no value in a function returning "
                          + describe(currentReturnType));
            }
        } else if (isVoid(currentReturnType)) {
            error(rs, "return with a value in a function returning void");
        } else if (got && !convertible(rs->expr, got, currentReturnType)) {
            error(rs, "returning " + describe(got) + " from a function returning "
                      + describe(currentReturnType));
        }
        return;
    }

    cc::IfStmt *is = dynamic_cast<cc::IfStmt*>(s);
    if (is) {
        bool lv = false;
        analyzeExpr(is->cond, lv);
        analyzeStmt(is->thenBranch);
        analyzeStmt(is->elseBranch);
        return;
    }

    cc::WhileStmt *ws = dynamic_cast<cc::WhileStmt*>(s);
    if (ws) {
        bool lv = false;
        analyzeExpr(ws->cond, lv);
        ++loopDepth;
        analyzeStmt(ws->body);
        --loopDepth;
        return;
    }

    cc::ForStmt *fs = dynamic_cast<cc::ForStmt*>(s);
    if (fs) {
        symbols.pushScope();            // the init declaration is scoped to the loop
        analyzeStmt(fs->init);
        bool lv = false;
        if (fs->cond) analyzeExpr(fs->cond, lv);
        if (fs->step) analyzeExpr(fs->step, lv);
        ++loopDepth;
        analyzeStmt(fs->body);
        --loopDepth;
        symbols.popScope();
        return;
    }

    if (dynamic_cast<cc::BreakStmt*>(s)) {
        if (loopDepth == 0) error(s, "'break' outside a loop");
        return;
    }
    if (dynamic_cast<cc::ContinueStmt*>(s)) {
        if (loopDepth == 0) error(s, "'continue' outside a loop");
        return;
    }
}

// --- Calls ---

void SemanticAnalyzer::checkCallArgs(cc::CallExpr *call, cc::Function *fn) {
    if (call->args.size() != fn->params.size()) {
        error(call, "'" + fn->name + "' expects " + countText(fn->params.size())
                    + " argument(s) but got " + countText(call->args.size()));
        return;
    }
    for (std::size_t i = 0; i < call->args.size(); ++i) {
        bool lv = false;
        cc::Type *at = analyzeExpr(call->args[i], lv);
        cc::Type *pt = fn->params[i]->type;
        if (!at || !pt) continue;
        // a reference parameter needs an lvalue, same rule as a variable
        cxx::ReferenceType *rt = dynamic_cast<cxx::ReferenceType*>(pt);
        if (rt && !lv) {
            error(call->args[i], "argument to reference parameter '"
                                 + fn->params[i]->name + "' must be an lvalue");
            continue;
        }
        if (!convertible(call->args[i], at, pt)) {
            error(call->args[i], "argument " + describe(at) + " does not match parameter '"
                                 + fn->params[i]->name + "' of type " + describe(pt));
        }
    }
}

// --- Expressions: one walk over a tree that mixes cc:: and cxx:: nodes ---

cc::Type *SemanticAnalyzer::analyzeExpr(cc::Expr *e, bool &isLValue) {
    isLValue = false;
    if (!e) return 0;

    // --- LAYER 2 forms, tested first: their operands are expressions ---

    cxx::MemberAccessExpr *ma = dynamic_cast<cxx::MemberAccessExpr*>(e);
    if (ma) {
        bool baseLV = false;
        cc::Type *baseType = analyzeExpr(ma->base, baseLV);
        if (!baseType) return 0;
        if (ma->isArrow) {
            cc::PointerType *pt = dynamic_cast<cc::PointerType*>(baseType);
            if (!pt) { error(ma, "'->' applied to " + describe(baseType) + ", which is not a pointer"); return 0; }
            baseType = pt->base;
        } else if (dynamic_cast<cc::PointerType*>(baseType)) {
            error(ma, "'.' applied to a pointer; did you mean '->'?");
            return 0;
        }
        cxx::ClassType *ct = dynamic_cast<cxx::ClassType*>(baseType);
        if (!ct) { error(ma, "member access on non-class type " + describe(baseType)); return 0; }
        cxx::ClassDecl *cd = findClass(ct->className);
        if (!cd) { error(ma, "unknown class '" + ct->className + "'"); return 0; }
        cxx::ClassDecl *owner = 0;
        cc::Decl *m = findMember(cd, ma->member, &owner);
        if (!m) { error(ma, "no member named '" + ma->member + "' in class '" + ct->className + "'"); return 0; }
        if (!memberIsAccessible(m, owner)) {
            error(ma, "'" + ma->member + "' is " + cxx::accessText(memberAccess(m))
                      + " in class '" + owner->name + "'");
        }
        isLValue = (dynamic_cast<cxx::FieldDecl*>(m) != 0);
        return stripReference(memberType(m));
    }

    cxx::ThisExpr *te = dynamic_cast<cxx::ThisExpr*>(e);
    if (te) {
        if (currentClass.empty()) { error(te, "'this' used outside a member function"); return 0; }
        cxx::ClassType self(currentClass);
        return makePointerTo(&self);
    }

    cxx::NewExpr *ne = dynamic_cast<cxx::NewExpr*>(e);
    if (ne) {
        checkTypeIsKnown(ne->allocType, ne, "'new' expression");
        for (std::size_t i = 0; i < ne->args.size(); ++i) {
            bool lv = false;
            analyzeExpr(ne->args[i], lv);
        }
        // new allocates AND constructs, so the arguments must match a ctor.
        cxx::ClassType *nct = dynamic_cast<cxx::ClassType*>(ne->allocType);
        if (nct) selectConstructor(findClass(nct->className), ne->args.size(), ne, "'new'");
        else if (!ne->args.empty()) {
            error(ne, "'new " + describe(ne->allocType) + "' cannot take constructor arguments");
        }
        // new T yields T*, and the T belongs to the AST node, so it is copied
        return makePointerTo(ne->allocType);
    }

    cxx::DeleteExpr *de = dynamic_cast<cxx::DeleteExpr*>(e);
    if (de) {
        bool lv = false;
        cc::Type *t = analyzeExpr(de->operand, lv);
        cc::PointerType *pt = t ? dynamic_cast<cc::PointerType*>(t) : 0;
        if (t && !pt) {
            error(de, "'delete' applied to " + describe(t) + ", which is not a pointer");
        } else if (pt) {
            // Only a virtual destructor is found through the vtable.
            cxx::ClassType *ct = dynamic_cast<cxx::ClassType*>(pt->base);
            cxx::ClassDecl *cd = ct ? findClass(ct->className) : 0;
            if (cd && cd->dtor && !cd->dtor->isVirtual) {
                bool anyDerived = false;
                std::map<std::string, cxx::ClassDecl*>::iterator it;
                for (it = classes.begin(); it != classes.end(); ++it) {
                    if (it->second != cd && isDerivedFrom(it->second, cd)) { anyDerived = true; break; }
                }
                if (anyDerived) {
                    diag.warning(de->line, de->col,
                                 "deleting through '" + describe(t) + "' whose destructor is not "
                                 "virtual; a derived object would not be destroyed correctly");
                }
            }
        }
        return makeBuiltin("void");
    }

    // --- LAYER 1 forms ---

    cc::IdentExpr *id = dynamic_cast<cc::IdentExpr*>(e);
    if (id) {
        Symbol *s = symbols.lookup(id->name);
        if (!s) { error(id, "undeclared identifier '" + id->name + "'"); return 0; }
        if (s->kind == SYM_Type) { error(id, "type '" + id->name + "' used as an expression"); return 0; }
        // May have come from a base class scope, so it needs the same access
        // check obj.member gets -- or omitting `this->` would defeat it.
        if (s->kind == SYM_Field || s->kind == SYM_Method) {
            cc::Decl *m = dynamic_cast<cc::Decl*>(s->decl);
            cxx::ClassDecl *owner = m ? ownerClassOf(m) : 0;
            if (owner && !memberIsAccessible(m, owner)) {
                error(id, "'" + id->name + "' is " + cxx::accessText(memberAccess(m))
                          + " in class '" + owner->name + "'");
                return 0;
            }
        }
        isLValue = (s->kind == SYM_Var || s->kind == SYM_Field);
        return stripReference(s->type);
    }

    if (dynamic_cast<cc::NumberExpr*>(e)) return makeBuiltin("int");

    cc::CallExpr *call = dynamic_cast<cc::CallExpr*>(e);
    if (call) {
        // Resolve the callee to a function without treating it as a value.
        cc::Function *fn = 0;
        cc::IdentExpr *cid = dynamic_cast<cc::IdentExpr*>(call->callee);
        if (cid) {
            Symbol *s = symbols.lookup(cid->name);
            if (!s) { error(cid, "undeclared function '" + cid->name + "'"); return 0; }
            fn = dynamic_cast<cc::Function*>(s->decl);
            if (!fn) { error(call, "'" + cid->name + "' is not a function"); return 0; }
        } else {
            cxx::MemberAccessExpr *cma = dynamic_cast<cxx::MemberAccessExpr*>(call->callee);
            if (!cma) { error(call, "expression is not callable"); return 0; }
            bool baseLV = false;
            cc::Type *baseType = analyzeExpr(cma->base, baseLV);
            if (!baseType) return 0;
            if (cma->isArrow) {
                cc::PointerType *pt = dynamic_cast<cc::PointerType*>(baseType);
                if (!pt) { error(cma, "'->' applied to a non-pointer"); return 0; }
                baseType = pt->base;
            }
            cxx::ClassType *ct = dynamic_cast<cxx::ClassType*>(baseType);
            if (!ct) { error(cma, "method call on non-class type " + describe(baseType)); return 0; }
            cxx::ClassDecl *cd = findClass(ct->className);
            cxx::ClassDecl *owner = 0;
            cc::Decl *m = cd ? findMember(cd, cma->member, &owner) : 0;
            if (!m) { error(cma, "no member named '" + cma->member + "' in class '" + ct->className + "'"); return 0; }
            if (!memberIsAccessible(m, owner)) {
                error(cma, "'" + cma->member + "' is " + cxx::accessText(memberAccess(m))
                           + " in class '" + owner->name + "'");
            }
            fn = dynamic_cast<cxx::MethodDecl*>(m);
            if (!fn) { error(cma, "'" + cma->member + "' is not a method"); return 0; }
        }
        checkCallArgs(call, fn);
        return stripReference(fn->retType);
    }

    cc::UnaryExpr *ue = dynamic_cast<cc::UnaryExpr*>(e);
    if (ue) {
        bool operandLV = false;
        cc::Type *t = analyzeExpr(ue->operand, operandLV);
        if (!t) return 0;
        switch (ue->op) {
        case cc::UN_Neg:
            if (!sameType(t, makeBuiltin("int"))) { error(ue, "unary '-' needs an int, got " + describe(t)); return 0; }
            return makeBuiltin("int");
        case cc::UN_Not:
            return makeBuiltin("int");
        case cc::UN_Deref: {
            cc::PointerType *pt = dynamic_cast<cc::PointerType*>(t);
            if (!pt) { error(ue, "unary '*' applied to " + describe(t) + ", which is not a pointer"); return 0; }
            isLValue = true;
            return pt->base;
        }
        case cc::UN_AddrOf: {
            if (!operandLV) { error(ue, "cannot take the address of a non-lvalue"); return 0; }
            return makePointerTo(t);
        }
        }
        return 0;
    }

    cc::BinaryExpr *be = dynamic_cast<cc::BinaryExpr*>(e);
    if (be) {
        bool lL = false, lR = false;
        cc::Type *lt = analyzeExpr(be->lhs, lL);
        cc::Type *rt = analyzeExpr(be->rhs, lR);
        if (!lt || !rt) return 0;       // the operand already reported its error

        if (be->op == cc::BIN_Assign) {
            // The other half of lvalue-ness: only an object can be assigned to.
            if (!lL) { error(be, "left side of assignment is not an lvalue"); return 0; }
            if (!convertible(be->rhs, rt, lt)) {
                error(be, "cannot assign " + describe(rt) + " to " + describe(lt));
                return 0;
            }
            isLValue = true;            // in C++ an assignment yields an lvalue
            return lt;
        }

        if (cc::binaryOpIsComparison(be->op) || cc::binaryOpIsLogical(be->op)) {
            if (!sameType(lt, rt)) {
                error(be, std::string("cannot compare ") + describe(lt) + " with " + describe(rt));
                return 0;
            }
            return makeBuiltin("int");  // no bool in the generated code yet
        }

        cc::BuiltinType *biL = dynamic_cast<cc::BuiltinType*>(lt);
        cc::BuiltinType *biR = dynamic_cast<cc::BuiltinType*>(rt);
        if (biL && biR && biL->name == "int" && biR->name == "int") return makeBuiltin("int");
        error(be, std::string("invalid operands to '") + cc::binaryOpText(be->op)
                  + "': " + describe(lt) + " and " + describe(rt));
        return 0;
    }

    error(e, "unhandled expression kind in the semantic analyzer");
    return 0;
}
