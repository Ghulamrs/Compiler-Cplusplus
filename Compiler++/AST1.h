// AST1.h
//
// LAYER 2 -- the C++ layer, namespace `cxx`.
//
//     cxx::X : public cc::X
//
// The authoritative layering model (full AST + parser diagram) lives at the
// top of AST.h.  This file declares only what C++ ADDS to C: reference and
// class types, classes and their members, qualified names, member access,
// `this`, and free-store allocation.  Everything else -- variables, functions,
// statements, the whole expression grammar -- is used directly from cc.
//
// C++98 only.

#ifndef AST1_H
#define AST1_H

#include <cstddef>
#include <string>
#include <vector>
#include <iostream>

#include "AST.h"

namespace cxx {

// These names are the SAME types as in cc, not copies.  They are pulled in so
// this layer can spell them unqualified, and so existing code that says
// cxx::Decl or cxx::VarDecl keeps meaning exactly what it did.
using cc::Type;
using cc::BuiltinType;
using cc::PointerType;
using cc::Decl;
using cc::VarDecl;
using cc::Function;

// Access control is meaningless in C, so the enum belongs to this layer.
// `protected` differs from `private` only once inheritance exists, which is
// the next feature to land.
enum Access { ACC_Public, ACC_Private, ACC_Protected };
const char *accessText(Access a);

// --- Types added by C++ -----------------------------------------------

// Reference type T&  -- does not exist in C
struct ReferenceType : public Type {
    Type *base;
    ReferenceType(Type *b) : base(b) {}
    ~ReferenceType() { delete base; }
    void print(int indent);
};

// A class / user-defined type name -- does not exist in C
struct ClassType : public Type {
    std::string className;
    ClassType(const std::string &n) : className(n) {}
    void print(int indent);
};

// --- Declarations added by C++ ----------------------------------------

// A data member.  It is a declaration with an access level and an owning
// class -- a plain cc::VarDecl knows neither.
struct FieldDecl : public Decl {
    Type *type;
    std::string name;
    Access access;
    std::string ownerClass;
    FieldDecl(Type *t, const std::string &n, Access a)
        : type(t), name(n), access(a) {}
    ~FieldDecl() { delete type; }
    void print(int indent);
};

// A member function.  A method IS a function -- same return type, name,
// parameters and body -- that additionally knows its access, its class, and
// whether it is virtual.  So it derives from cc::Function instead of
// restating it.
struct MethodDecl : public Function {
    Access access;
    std::string ownerClass;
    // Written `virtual`, OR inherited virtualness by overriding a base method.
    // In C++ a function that overrides a virtual function is itself virtual
    // whether or not the keyword is repeated, so this is set by the semantic
    // pass as well as by the parser.
    bool isVirtual;
    // The base-class method this one overrides, filled in by the semantic
    // pass; 0 when this method overrides nothing.
    MethodDecl *overrides;
    MethodDecl(Type *r, const std::string &n, Access a)
        : Function(r, n), access(a), isVirtual(false), overrides(0) {}
    // Only the first printed line differs from a plain function.
    void printSignature(int indent);
};

// class D : public B { ... };
//
// SINGLE inheritance only: one base, by design.  With one base the derived
// object is simply the base object with fields appended, so an upcast is a
// no-op and the vptr can be shared -- the whole object model stays something
// you can hold in your head.  Multiple bases would mean several subobjects at
// different offsets and pointer adjustment on every conversion, so the parser
// rejects a second base with a message that says so.
struct ClassDecl : public Decl {
    std::string name;
    std::string baseName;       // empty when the class has no base
    Access baseAccess;          // public/private/protected inheritance
    // Resolved by the semantic pass from baseName; NOT owned, and 0 until then.
    ClassDecl *base;
    std::vector<Decl*> members;
    ClassDecl(const std::string &n)
        : name(n), baseAccess(ACC_Public), base(0) {}
    ~ClassDecl();
    void print(int indent);
};

// --- Qualified name  A::B ---------------------------------------------
struct QualifiedName : public cc::ASTNode {
    std::vector<std::string> parts;
    QualifiedName() {}
    void print(int indent);
};

// --- Expressions added by C++ -----------------------------------------
// NumberExpr, IdentExpr, UnaryExpr, BinaryExpr and CallExpr are NOT
// redeclared: this layer uses the cc:: forms directly.  Only genuinely new
// forms appear here, each deriving from cc::Expr so C and C++ expressions
// share one tree:  (a.b + 1) * 2

struct MemberAccessExpr : public cc::Expr {
    cc::Expr *base;
    std::string member;
    bool isArrow;
    MemberAccessExpr(cc::Expr *b, const std::string &m, bool arrow)
        : base(b), member(m), isArrow(arrow) {}
    ~MemberAccessExpr() { delete base; }
    void print(int indent);
};

// `this` inside a method body
struct ThisExpr : public cc::Expr {
    void print(int indent);
};

// new T   -- allocation and construction; the constructor call arrives with
// the next phase, so the argument list is already here to receive it.
struct NewExpr : public cc::Expr {
    Type *allocType;
    std::vector<cc::Expr*> args;
    NewExpr(Type *t) : allocType(t) {}
    ~NewExpr();
    void print(int indent);
};

// delete p  -- destruction and release
struct DeleteExpr : public cc::Expr {
    cc::Expr *operand;
    DeleteExpr(cc::Expr *e) : operand(e) {}
    ~DeleteExpr() { delete operand; }
    void print(int indent);
};

} // namespace cxx

#endif
