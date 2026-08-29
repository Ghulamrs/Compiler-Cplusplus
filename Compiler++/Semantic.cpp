//
//  Semantic.cpp
//  Compiler++
//
//  Created by G. R. Akhtar on 29/08/2026.
//
//  C++98 only.  See Semantic.h for how this pass spans both class layers.

#include "Semantic.h"

#include <cstddef>
#include <cstdlib>
#include <iostream>

SemanticAnalyzer::SemanticAnalyzer() : errorCount(0) {}

SemanticAnalyzer::~SemanticAnalyzer() {
    for (std::size_t i = 0; i < ownedTypes.size(); ++i) delete ownedTypes[i];
}

void SemanticAnalyzer::error(const std::string &msg) {
    ++errorCount;
    std::cout.flush();          // keep diagnostics in step with the AST dump
    std::cerr << "Semantic error: " << msg << std::endl;
}

Symbol *SemanticAnalyzer::lookup(const std::string &name) {
    return symbols.lookup(name);
}

// Expression results need a type object that outlives the call but belongs to
// nobody in the AST.  The analyzer keeps them and frees them in its destructor.
cc::Type *SemanticAnalyzer::makeInt() {
    cc::Type *t = new cc::BuiltinType("int");
    ownedTypes.push_back(t);
    return t;
}

// ---------------------------------------------------------------------
// Type helpers
// ---------------------------------------------------------------------

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

// int, int*, int** need no resolution; a class name does.
void SemanticAnalyzer::checkTypeIsKnown(cc::Type *t, const std::string &where) {
    if (!t) return;
    cc::PointerType *pt = dynamic_cast<cc::PointerType*>(t);
    if (pt) { checkTypeIsKnown(pt->base, where); return; }
    cxx::ReferenceType *rt = dynamic_cast<cxx::ReferenceType*>(t);
    if (rt) { checkTypeIsKnown(rt->base, where); return; }
    cxx::ClassType *ct = dynamic_cast<cxx::ClassType*>(t);
    if (ct) {
        Symbol *s = lookup(ct->className);
        if (!s || s->kind != SYM_Type) {
            error("Unknown type '" + ct->className + "' in " + where);
        }
    }
}

// ---------------------------------------------------------------------
// LAYER 2 entry point: a C++ translation unit
// ---------------------------------------------------------------------

void SemanticAnalyzer::analyze(const std::vector<cxx::Decl*> &units) {
    // 1) register top-level types (classes) so references to them can resolve
    registerClassNames(units);

    // 2) register other top-level declarations, and every class's members
    registerTopLevel(units);

    // 3) analyze each declaration (member resolution, method parameters)
    for (std::size_t i = 0; i < units.size(); ++i) {
        analyzeDecl(units[i]);
    }
}

void SemanticAnalyzer::registerClassNames(const std::vector<cxx::Decl*> &units) {
    for (std::size_t i = 0; i < units.size(); ++i) {
        cxx::ClassDecl *cd = dynamic_cast<cxx::ClassDecl*>(units[i]);
        if (cd) {
            // register the class name as a type symbol
            cc::Type *ct = new cxx::ClassType(cd->name);
            ownedTypes.push_back(ct);
            Symbol *s = new Symbol(SYM_Type, cd->name, cd, ct);
            if (!symbols.insert(cd->name, s)) {
                error("Duplicate top-level name: " + cd->name);
                delete s;
            }
        }
    }
}

void SemanticAnalyzer::registerTopLevel(const std::vector<cxx::Decl*> &units) {
    for (std::size_t i = 0; i < units.size(); ++i) {
        // VarDecl at top level
        cxx::VarDecl *vd = dynamic_cast<cxx::VarDecl*>(units[i]);
        if (vd) {
            Symbol *s = new Symbol(SYM_Var, vd->name, vd, vd->type);
            if (!symbols.insert(vd->name, s)) {
                error("Duplicate top-level variable: " + vd->name);
                delete s;
            }
            continue;
        }
        // a free function at file scope:  int main() { ... }
        cxx::MethodDecl *md = dynamic_cast<cxx::MethodDecl*>(units[i]);
        if (md) {
            Symbol *s = new Symbol(SYM_Method, md->name, md, md->retType);
            if (!symbols.insert(md->name, s)) {
                error("Duplicate top-level function: " + md->name);
                delete s;
            }
            continue;
        }
        // Classes were named above; now register their members
        cxx::ClassDecl *cd = dynamic_cast<cxx::ClassDecl*>(units[i]);
        if (cd) {
            registerClassMembers(cd);
        }
    }
}

void SemanticAnalyzer::registerClassMembers(cxx::ClassDecl *cd) {
    // members are recorded under qualified names like Class::member
    for (std::size_t i = 0; i < cd->members.size(); ++i) {
        cxx::FieldDecl *fd = dynamic_cast<cxx::FieldDecl*>(cd->members[i]);
        if (fd) {
            std::string fullname = cd->name + "::" + fd->name;
            Symbol *s = new Symbol(SYM_Field, fullname, fd, fd->type);
            if (!symbols.insert(fullname, s)) {
                error("Duplicate member " + fd->name + " in class " + cd->name);
                delete s;
            }
            continue;
        }
        cxx::MethodDecl *md = dynamic_cast<cxx::MethodDecl*>(cd->members[i]);
        if (md) {
            std::string fullname = cd->name + "::" + md->name;
            Symbol *s = new Symbol(SYM_Method, fullname, md, md->retType);
            if (!symbols.insert(fullname, s)) {
                error("Duplicate method " + md->name + " in class " + cd->name);
                delete s;
            }
            continue;
        }
    }
}

void SemanticAnalyzer::analyzeDecl(cxx::Decl *d) {
    if (!d) return;
    // dispatch by dynamic cast
    cxx::ClassDecl *cd = dynamic_cast<cxx::ClassDecl*>(d);
    if (cd) { analyzeClass(cd); return; }
    cxx::VarDecl *vd = dynamic_cast<cxx::VarDecl*>(d);
    if (vd) { analyzeVarDecl(vd); return; }
    cxx::MethodDecl *md = dynamic_cast<cxx::MethodDecl*>(d);
    if (md) { analyzeMethodDecl(md); return; }
    // other decls ignored for now
}

void SemanticAnalyzer::analyzeClass(cxx::ClassDecl *cd) {
    for (std::size_t i = 0; i < cd->members.size(); ++i) {
        cxx::FieldDecl *fd = dynamic_cast<cxx::FieldDecl*>(cd->members[i]);
        if (fd) { analyzeFieldDecl(fd); continue; }
        cxx::MethodDecl *md = dynamic_cast<cxx::MethodDecl*>(cd->members[i]);
        if (md) { analyzeMethodDecl(md); continue; }
    }
}

void SemanticAnalyzer::analyzeVarDecl(cxx::VarDecl *vd) {
    if (!vd) return;
    checkTypeIsKnown(vd->type, "declaration of " + vd->name);
}

void SemanticAnalyzer::analyzeFieldDecl(cxx::FieldDecl *fd) {
    if (!fd) return;
    checkTypeIsKnown(fd->type, "field " + fd->name);
}

void SemanticAnalyzer::analyzeMethodDecl(cxx::MethodDecl *md) {
    // parameters live in their own scope
    symbols.pushScope();
    for (std::size_t i = 0; i < md->params.size(); ++i) {
        cxx::VarDecl *p = md->params[i];
        Symbol *s = new Symbol(SYM_Var, p->name, p, p->type);
        if (!symbols.insert(p->name, s)) {
            error("Duplicate parameter name: " + p->name);
            delete s;
        }
    }
    checkTypeIsKnown(md->retType, "return type of " + md->name);
    for (std::size_t i = 0; i < md->params.size(); ++i) {
        checkTypeIsKnown(md->params[i]->type, "parameter " + md->params[i]->name);
    }
    for (std::size_t i = 0; i < md->body.size(); ++i) {
        analyzeStmt(md->body[i]);
    }
    symbols.popScope();
}

// ---------------------------------------------------------------------
// LAYER 1 entry point: a C function body
// ---------------------------------------------------------------------

void SemanticAnalyzer::analyzeFunction(cc::Function *f) {
    if (!f) return;
    Symbol *s = new Symbol(SYM_Method, f->name, f, 0);
    if (!symbols.insert(f->name, s)) {
        error("Duplicate top-level function: " + f->name);
        delete s;
    }
    symbols.pushScope();
    for (std::size_t i = 0; i < f->body.size(); ++i) {
        analyzeStmt(f->body[i]);
    }
    symbols.popScope();
}

void SemanticAnalyzer::analyzeStmt(cc::Stmt *s) {
    if (!s) return;

    // A declaration:  int a = 1;   Point p;   int &r = p.x;
    cc::DeclStmt *ds = dynamic_cast<cc::DeclStmt*>(s);
    if (ds) {
        checkTypeIsKnown(ds->type, "declaration of " + ds->name);

        cc::Type *initType = 0;
        bool initIsLValue = false;
        if (ds->init) {
            initType = analyzeExpr(ds->init, initIsLValue);
        }

        // THE REFERENCE RULE.  `int &r = p.x;` binds a name to an existing
        // object, so the initialiser must denote one -- an lvalue.  `int &s = 1;`
        // has nothing to bind to: 1 is a value, not an object.  This is the one
        // check that needs analyzeExpr()'s isLValue result, and it is why that
        // result is threaded through every expression form.
        cxx::ReferenceType *rt = dynamic_cast<cxx::ReferenceType*>(ds->type);
        if (rt) {
            if (!ds->init) {
                error("Reference '" + ds->name + "' must be initialised");
            } else if (initType && !initIsLValue) {
                error("Cannot bind reference '" + ds->name + "' of type "
                      + describe(ds->type) + " to a non-lvalue initialiser");
            } else if (initType && !sameType(rt->base, initType)) {
                error("Cannot bind '" + describe(ds->type) + " " + ds->name
                      + "' to an initialiser of type " + describe(initType));
            }
        } else if (ds->init && initType && !sameType(ds->type, initType)) {
            error("Cannot initialise '" + describe(ds->type) + " " + ds->name
                  + "' from an expression of type " + describe(initType));
        }

        if (symbols.lookupLocal(ds->name)) {
            error("Redeclaration of '" + ds->name + "' in the same scope");
            return;
        }
        Symbol *sym = new Symbol(SYM_Var, ds->name, ds, ds->type);
        if (!symbols.insert(ds->name, sym)) {
            error("Redeclaration of '" + ds->name + "'");
            delete sym;
        }
        return;
    }

    // An expression used as a statement:  p.x = 1;
    cc::ExprStmt *es = dynamic_cast<cc::ExprStmt*>(s);
    if (es) {
        bool isLValue = false;
        analyzeExpr(es->expr, isLValue);
        return;
    }

    // return expr;
    cc::ReturnStmt *rs = dynamic_cast<cc::ReturnStmt*>(s);
    if (rs) {
        if (rs->expr) {
            bool isLValue = false;
            analyzeExpr(rs->expr, isLValue);
        }
        return;
    }
}

// ---------------------------------------------------------------------
// Expressions: one walk over a tree that mixes cc:: and cxx:: nodes
// ---------------------------------------------------------------------

cc::Type *SemanticAnalyzer::analyzeExpr(cc::Expr *e, bool &isLValue) {
    isLValue = false;
    if (!e) return 0;

    // Member access  base.member / base->member  -- a LAYER 2 node.  It is
    // tested before IdentExpr because its base is itself an expression.
    cxx::MemberAccessExpr *ma = dynamic_cast<cxx::MemberAccessExpr*>(e);
    if (ma) {
        bool baseIsLValue = false;
        cc::Type *baseType = analyzeExpr(ma->base, baseIsLValue);
        if (!baseType) return 0;
        // p->m means p is a pointer; strip one level before looking inside
        if (ma->isArrow) {
            cc::PointerType *pt = dynamic_cast<cc::PointerType*>(baseType);
            if (!pt) {
                error("'->' applied to a non-pointer");
                return 0;
            }
            baseType = pt->base;
        }
        cxx::ClassType *ct = dynamic_cast<cxx::ClassType*>(baseType);
        if (!ct) {
            error("Member access on non-class type");
            return 0;
        }
        std::string fullname = ct->className + "::" + ma->member;
        Symbol *s = symbols.lookup(fullname);
        if (!s) {
            error("Member " + ma->member + " not found in class " + ct->className);
            return 0;
        }
        isLValue = (s->kind == SYM_Field);
        return stripReference(s->type);
    }

    // Identifier -- a LAYER 1 node
    cc::IdentExpr *id = dynamic_cast<cc::IdentExpr*>(e);
    if (id) {
        Symbol *s = lookup(id->name);
        if (!s) {
            error("Undefined identifier: " + id->name);
            return 0;
        }
        if (s->kind == SYM_Type) {
            error("Type used as expression: " + id->name);
            return 0;
        }
        if (s->kind == SYM_Var || s->kind == SYM_Field) {
            isLValue = true;
            return stripReference(s->type);
        }
        return stripReference(s->type);
    }

    // Number literal
    cc::NumberExpr *num = dynamic_cast<cc::NumberExpr*>(e);
    if (num) {
        return makeInt();
    }

    // Binary expression, including assignment
    cc::BinaryExpr *be = dynamic_cast<cc::BinaryExpr*>(e);
    if (be) {
        bool lL = false, lR = false;
        cc::Type *lt = analyzeExpr(be->lhs, lL);
        cc::Type *rt = analyzeExpr(be->rhs, lR);
        if (!lt || !rt) return 0;   // the operand already reported its error

        if (be->op == '=') {
            // THE ASSIGNMENT RULE, the other half of lvalue-ness: only an
            // object can be assigned to.  `1 = p.x;` is rejected here.
            if (!lL) {
                error("Left side of assignment is not an lvalue");
                return 0;
            }
            if (!sameType(lt, rt)) {
                error("Cannot assign " + describe(rt) + " to " + describe(lt));
                return 0;
            }
            // in C++ an assignment yields the left operand, still an lvalue
            isLValue = true;
            return lt;
        }

        cc::BuiltinType *biL = dynamic_cast<cc::BuiltinType*>(lt);
        cc::BuiltinType *biR = dynamic_cast<cc::BuiltinType*>(rt);
        if (biL && biR && biL->name == "int" && biR->name == "int") {
            return makeInt();
        }
        error("Type error in binary expression: "
              + describe(lt) + " " + std::string(1, be->op) + " " + describe(rt));
        return 0;
    }

    error("Unhandled expression kind in semantic analyzer");
    return 0;
}
