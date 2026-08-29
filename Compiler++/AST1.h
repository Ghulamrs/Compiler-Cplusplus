// AST1.h
//
// LAYER 2 -- the C++ layer, namespace `cxx`.
//
//     cxx::X : public cc::X
//
// The authoritative layering model (full AST + parser diagram) lives at the
// top of AST.h.  This file declares only what C++ ADDS to C: reference and
// class types, declarations, qualified names, and member-access expressions.
// Everything else is used directly from namespace cc.
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

// The type system lives in the C layer; pull the names in so this layer can
// spell them unqualified.  These are the SAME types, not copies.
using cc::Type;
using cc::BuiltinType;
using cc::PointerType;

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

// --- Declarations (new in the C++ layer) ------------------------------
struct Decl : public cc::ASTNode {
    virtual ~Decl() {}
};

struct VarDecl : public Decl {
    Type *type;
    std::string name;
    VarDecl(Type *t, const std::string &n) : type(t), name(n) {}
    ~VarDecl() { delete type; }
    void print(int indent);
};

struct FieldDecl : public Decl {
    Type *type;
    std::string name;
    FieldDecl(Type *t, const std::string &n) : type(t), name(n) {}
    ~FieldDecl() { delete type; }
    void print(int indent);
};

// A function with a signature and, optionally, a body.  This one node covers
// both a class member  int getX();  and a free function  int main() { ... }  --
// they differ only in where they appear, not in what they hold.  The body is a
// vector of cc::Stmt, so the C layer's statements sit directly inside a C++
// layer declaration; that is the same tree-sharing that lets cxx::MemberAccessExpr
// live inside a cc::BinaryExpr.
struct MethodDecl : public Decl {
    Type *retType;
    std::string name;
    std::vector<VarDecl*> params;
    std::vector<cc::Stmt*> body;
    bool hasBody;                       // distinguishes  f();  from  f() {}
    MethodDecl(Type *r, const std::string &n) : retType(r), name(n), hasBody(false) {}
    ~MethodDecl();
    void print(int indent);
};

struct ClassDecl : public Decl {
    std::string name;
    std::vector<Decl*> members;
    ClassDecl(const std::string &n) : name(n) {}
    ~ClassDecl();
    void print(int indent);
};

// --- Qualified name  A::B ---------------------------------------------
struct QualifiedName : public cc::ASTNode {
    std::vector<std::string> parts;
    QualifiedName() {}
    void print(int indent);
};

// --- Expressions ------------------------------------------------------
// NumberExpr, IdentExpr and BinaryExpr are NOT redeclared: this layer uses
// cc::NumberExpr, cc::IdentExpr and cc::BinaryExpr directly.  Only the new
// C++ form is added, deriving from cc::Expr so C and C++ expressions share
// one tree:  (a.b + 1) * 2
struct MemberAccessExpr : public cc::Expr {
    cc::Expr *base;
    std::string member;
    bool isArrow;
    MemberAccessExpr(cc::Expr *b, const std::string &m, bool arrow) : base(b), member(m), isArrow(arrow) {}
    ~MemberAccessExpr() { delete base; }
    void print(int indent);
};

} // namespace cxx

#endif
