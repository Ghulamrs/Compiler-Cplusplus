// AST.h
//
// ============================ LAYERING MODEL ============================
//
//   Compiler++ is built as TWO LAYERS, and the second inherits the first:
//
//                       cxx::X : public cc::X
//
//   cc::   LAYER 1, the C layer   -- AST.h  / AST.cpp  / Parser.h  / Parser.cpp
//   cxx::  LAYER 2, the C++ layer -- AST1.h / AST1.cpp / Parser1.h / Parser1.cpp
//
//   Nothing in layer 2 duplicates layer 1.  A C++ node is either a cc:: node
//   reused verbatim, or a new node derived from a cc:: base.
//
//   ---- AST -----------------------------------------------------------
//   cc::ASTNode                                    (the single root)
//     |-- cc::Type
//     |     |-- cc::BuiltinType                    int
//     |     |-- cc::PointerType                    T*
//     |     |-- cxx::ReferenceType  : public cc::Type    T&      NEW in C++
//     |     '-- cxx::ClassType      : public cc::Type    Point   NEW in C++
//     |-- cc::Expr
//     |     |-- cc::NumberExpr | cc::IdentExpr | cc::BinaryExpr
//     |     '-- cxx::MemberAccessExpr : public cc::Expr  a.b p->q  NEW in C++
//     |-- cc::Stmt
//     |     |-- cc::DeclStmt   |-- cc::ExprStmt
//     |     '-- cc::ReturnStmt
//     |-- cc::Function
//     |-- cxx::Decl : public cc::ASTNode                         NEW in C++
//     |     |-- cxx::VarDecl   |-- cxx::FieldDecl
//     |     |-- cxx::MethodDecl '-- cxx::ClassDecl
//     '-- cxx::QualifiedName : public cc::ASTNode   A::B         NEW in C++
//
//   ---- Parser --------------------------------------------------------
//   cc::Parser
//     parse, parseFunction, parseBlock, parseDeclTail, parseReturn
//     parseExpression -> parseAssign -> parseAddSub -> parseMulDiv
//                                                       (defined ONCE, here)
//     parsePointerSuffixes(Type*)                       T* T**  shared down
//     save() / restore()         one-token-set rewind, for speculation
//     virtual parseStatement()   declaration vs expression, decided by rewind
//     virtual parsePrimary()     numbers, identifiers, parentheses
//     virtual parseType()        int, int*, int**
//        ^
//        | public
//   cxx::Parser : public cc::Parser
//     parseTranslationUnit, parseClass, parseMemberDecl, parseFunctionRest,
//     parseQualifiedName
//     virtual parsePrimary()     adds  a.b  p->q , else defers to cc::
//     virtual parseType()        asks cc:: first, then adds A::B and T&
//
//   Because cc::Parser::parseMulDiv() calls parsePrimary() VIRTUALLY, an
//   expression such as (a.b + 1) * 2 is parsed by both layers cooperatively
//   and yields one tree mixing cc:: and cxx:: nodes.  The same trick runs the
//   other way in parseStatement(), which calls parseType() virtually: the C
//   layer's statement rule declares C++ types --  Point p;  int &r = p.x;  --
//   without the C++ layer writing any statement code at all.
//
//   ---- Semantic analysis ---------------------------------------------
//   SemanticAnalyzer (Semantic.h) walks that one mixed tree, using
//   SymbolTable (SymbolTable.h) for scopes.  It is deliberately NOT split in
//   two, because the tree is not: dynamic_cast tells cc:: and cxx:: nodes
//   apart, which works because every node derives from cc::ASTNode.
//
//   C++98 has no `override` keyword: a derived layer re-declares the function
//   with an EXACTLY matching signature, or it silently hides instead.
//
// ========================================================================
//
// LAYER 1 -- the C layer, namespace `cc`.
//
// C++98 only.
#ifndef AST_H
#define AST_H

#include <cstddef>
#include <string>
#include <vector>
#include <iostream>

namespace cc {

struct ASTNode {
    virtual ~ASTNode() {}
    virtual void print(int indent = 0) = 0;
protected:
    void printIndent(int n) {
        for (int i = 0; i < n; ++i) std::cout << "  ";
    }
};

// --- Types ------------------------------------------------------------
// The type system starts in the C layer: C already has builtin types and
// pointers.  The C++ layer adds references (T&) and class types on top.

struct Type : public ASTNode {
    virtual ~Type() {}
};

// Builtin type like int
struct BuiltinType : public Type {
    std::string name;
    BuiltinType(const std::string &n) : name(n) {}
    void print(int indent);
};

// Pointer type T*
struct PointerType : public Type {
    Type *base;
    PointerType(Type *b) : base(b) {}
    ~PointerType() { delete base; }
    void print(int indent);
};

// --- Expressions ------------------------------------------------------
struct Expr : public ASTNode {};

struct NumberExpr : public Expr {
    int value;
    NumberExpr(int v) : value(v) {}
    void print(int indent);
};

struct IdentExpr : public Expr {
    std::string name;
    IdentExpr(const std::string &n) : name(n) {}
    void print(int indent);
};

// Binary operator, including assignment:  op is one of  + - * /  or  '='
struct BinaryExpr : public Expr {
    char op;
    Expr *lhs;
    Expr *rhs;
    BinaryExpr(char o, Expr *l, Expr *r) : op(o), lhs(l), rhs(r) {}
    ~BinaryExpr();
    void print(int indent);
};

// --- Statements -------------------------------------------------------
struct Stmt : public ASTNode {};

// A declaration inside a function body:  int a = 1;   Point p;   int &r = p.x;
// The type is a Type* rather than a string, because the C++ layer declares
// things C cannot spell -- references and class types.  parseType() is virtual,
// so the C layer's statement rule builds these for both layers.
struct DeclStmt : public Stmt {
    Type *type;
    std::string name;
    Expr *init;         // may be 0:  Point p;  has no initialiser
    DeclStmt(Type *t, const std::string &n, Expr *i) : type(t), name(n), init(i) {}
    ~DeclStmt();
    void print(int indent);
};

// An expression used as a statement:  p.x = 1;
struct ExprStmt : public Stmt {
    Expr *expr;
    ExprStmt(Expr *e) : expr(e) {}
    ~ExprStmt();
    void print(int indent);
};

struct ReturnStmt : public Stmt {
    Expr *expr;
    ReturnStmt(Expr *e) : expr(e) {}
    ~ReturnStmt();
    void print(int indent);
};

struct Function : public ASTNode {
    std::string name;
    std::vector<Stmt*> body;
    Function(const std::string &n) : name(n) {}
    ~Function();
    void print(int indent);
};

} // namespace cc

#endif
