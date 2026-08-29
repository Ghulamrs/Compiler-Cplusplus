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
//   reused verbatim, or a new node derived from a cc:: base.  The test for
//   which layer a node belongs to is simply: does C already have this?
//   Declarations, variables and functions are C's, so they live here; classes,
//   fields, references and member access are C++'s, so they live in AST1.h.
//
//   ---- AST -----------------------------------------------------------
//   cc::ASTNode                                    (the single root)
//     |-- cc::Type
//     |     |-- cc::BuiltinType                    int, char, void, bool
//     |     |-- cc::PointerType                    T*
//     |     |-- cxx::ReferenceType  : public cc::Type    T&      NEW in C++
//     |     '-- cxx::ClassType      : public cc::Type    Point   NEW in C++
//     |-- cc::Expr
//     |     |-- cc::NumberExpr  | cc::IdentExpr
//     |     |-- cc::UnaryExpr   | cc::BinaryExpr | cc::CallExpr
//     |     |-- cxx::MemberAccessExpr : public cc::Expr  a.b p->q  NEW in C++
//     |     |-- cxx::ThisExpr         : public cc::Expr  this     NEW in C++
//     |     '-- cxx::NewExpr / cxx::DeleteExpr           new/delete NEW in C++
//     |-- cc::Stmt
//     |     |-- cc::CompoundStmt   { ... }         also the unit of scope
//     |     |-- cc::DeclStmt       wraps a cc::VarDecl
//     |     |-- cc::ExprStmt       | cc::ReturnStmt
//     |     |-- cc::IfStmt         | cc::WhileStmt | cc::ForStmt
//     |     '-- cc::BreakStmt      | cc::ContinueStmt
//     '-- cc::Decl
//           |-- cc::VarDecl                        int x;   Point p;
//           |-- cc::Function                       int f(int a) { ... }
//           |-- cxx::FieldDecl  : public cc::Decl      NEW in C++ (has access)
//           |-- cxx::MethodDecl : public cc::Function  NEW in C++ (has access,
//           |                                          and later virtual/ctor)
//           '-- cxx::ClassDecl  : public cc::Decl      NEW in C++
//
//   Note what is NOT in layer 2 any more: C has variables and functions, so
//   cc::VarDecl and cc::Function are used by both layers directly.  A C++
//   method is a C function that additionally knows its access and its class,
//   which is exactly what inheritance is for.
//
//   ---- Parser --------------------------------------------------------
//   cc::Parser
//     parseTranslationUnit, parseFunctionRest, parseBlock, parseDeclTail
//     the precedence chain, defined ONCE, here:
//       parseExpression -> parseAssign -> parseLogicalOr -> parseLogicalAnd
//         -> parseEquality -> parseRelational -> parseAddSub -> parseMulDiv
//         -> parseUnary -> parsePostfix -> parsePrimary
//     parsePointerSuffixes(Type*)                       T* T**  shared down
//     save() / restore()         one-token rewind, for speculation
//     virtual parseStatement()   declaration vs expression, decided by rewind
//     virtual parsePrimary()     numbers, identifiers, parentheses
//     virtual parseType()        int, char, void, bool, and T*
//     virtual parseMemberSuffix(Expr*)  returns 0 in C; the hook for  a.b  p->q
//     virtual parseDeclaration()        the hook for  class ... ;
//        ^
//        | public
//   cxx::Parser : public cc::Parser
//     parseClass, parseMemberDecl, parseQualifiedName
//     virtual parseMemberSuffix()  adds  a.b  p->q
//     virtual parseType()          asks cc:: first, then adds A::B and T&
//     virtual parsePrimary()       adds  this  and  new T
//     virtual parseDeclaration()   adds class definitions
//
//   Because the C layer's rules call these hooks VIRTUALLY, the two layers
//   parse cooperatively and yield ONE tree mixing cc:: and cxx:: nodes:
//     * parsePostfix() calls parseMemberSuffix(), so (a.b + 1) * 2 works;
//     * parseStatement() calls parseType(), so the C layer's statement rule
//       declares C++ types --  Point p;  int &r = p.x;  -- with no C++-layer
//       statement code at all.
//
//   ---- Semantic analysis ---------------------------------------------
//   SemanticAnalyzer (Semantic.h) walks that one mixed tree, using SymbolTable
//   (SymbolTable.h) for scopes.  It is deliberately NOT split in two, because
//   the tree is not: dynamic_cast tells cc:: and cxx:: nodes apart, which works
//   because every node derives from cc::ASTNode.
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
    // Where this node came from, copied off the token that started it.  A
    // semantic error can then point at source just as a syntax error does.
    int line;
    int col;

    ASTNode() : line(0), col(0) {}
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

// Builtin type like int, char, void, bool
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

// The operator sets are enums rather than characters, because  ==  and  &&  do
// not fit in a char, and because a named constant cannot be mistyped the way a
// string comparison can.
enum BinaryOp {
    BIN_Add, BIN_Sub, BIN_Mul, BIN_Div, BIN_Mod,
    BIN_Assign,
    BIN_EQ, BIN_NE, BIN_LT, BIN_GT, BIN_LE, BIN_GE,
    BIN_LAnd, BIN_LOr
};
const char *binaryOpText(BinaryOp op);
bool binaryOpIsComparison(BinaryOp op);
bool binaryOpIsLogical(BinaryOp op);

enum UnaryOp { UN_Neg, UN_Not, UN_Deref, UN_AddrOf };
const char *unaryOpText(UnaryOp op);

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

struct UnaryExpr : public Expr {
    UnaryOp op;
    Expr *operand;
    UnaryExpr(UnaryOp o, Expr *e) : op(o), operand(e) {}
    ~UnaryExpr() { delete operand; }
    void print(int indent);
};

struct BinaryExpr : public Expr {
    BinaryOp op;
    Expr *lhs;
    Expr *rhs;
    BinaryExpr(BinaryOp o, Expr *l, Expr *r) : op(o), lhs(l), rhs(r) {}
    ~BinaryExpr();
    void print(int indent);
};

// f(1, 2)  and, once the C++ layer supplies the callee,  p.getX()
struct CallExpr : public Expr {
    Expr *callee;
    std::vector<Expr*> args;
    CallExpr(Expr *c) : callee(c) {}
    ~CallExpr();
    void print(int indent);
};

// --- Declarations -----------------------------------------------------
// C has declarations, so the base and the two plain forms live in this layer.

struct Decl : public ASTNode {
    virtual ~Decl() {}
};

// int x;   int x = 1;   Point p;   int &r = p.x;
// One node for a variable wherever it appears -- at file scope on its own, or
// inside a block wrapped in a DeclStmt.
struct VarDecl : public Decl {
    Type *type;
    std::string name;
    Expr *init;                 // may be 0
    VarDecl(Type *t, const std::string &n, Expr *i) : type(t), name(n), init(i) {}
    ~VarDecl();
    void print(int indent);
};

struct Stmt;
struct CompoundStmt;

// int f(int a) { ... }   -- and, via cxx::MethodDecl, a C++ method too.
struct Function : public Decl {
    Type *retType;
    std::string name;
    std::vector<VarDecl*> params;
    CompoundStmt *body;         // 0 when this is only a declaration
    Function(Type *r, const std::string &n) : retType(r), name(n), body(0) {}
    ~Function();
    void print(int indent);
    // Lets MethodDecl add "Method"/"Class::" without repeating the rest.
    virtual void printSignature(int indent);
};

// --- Statements -------------------------------------------------------
struct Stmt : public ASTNode {};

// { ... }  -- also the unit of scope, which is why it is a node and not just
// a vector held by whoever needed one.
struct CompoundStmt : public Stmt {
    std::vector<Stmt*> body;
    ~CompoundStmt();
    void print(int indent);
};

struct DeclStmt : public Stmt {
    VarDecl *var;
    DeclStmt(VarDecl *v) : var(v) {}
    ~DeclStmt() { delete var; }
    void print(int indent);
};

struct ExprStmt : public Stmt {
    Expr *expr;
    ExprStmt(Expr *e) : expr(e) {}
    ~ExprStmt() { delete expr; }
    void print(int indent);
};

struct ReturnStmt : public Stmt {
    Expr *expr;                 // 0 for a bare  return;
    ReturnStmt(Expr *e) : expr(e) {}
    ~ReturnStmt() { delete expr; }
    void print(int indent);
};

struct IfStmt : public Stmt {
    Expr *cond;
    Stmt *thenBranch;
    Stmt *elseBranch;           // may be 0
    IfStmt(Expr *c, Stmt *t, Stmt *e) : cond(c), thenBranch(t), elseBranch(e) {}
    ~IfStmt();
    void print(int indent);
};

struct WhileStmt : public Stmt {
    Expr *cond;
    Stmt *body;
    WhileStmt(Expr *c, Stmt *b) : cond(c), body(b) {}
    ~WhileStmt();
    void print(int indent);
};

// for (init; cond; step) body   -- any of the three may be 0
struct ForStmt : public Stmt {
    Stmt *init;
    Expr *cond;
    Expr *step;
    Stmt *body;
    ForStmt(Stmt *i, Expr *c, Expr *s, Stmt *b) : init(i), cond(c), step(s), body(b) {}
    ~ForStmt();
    void print(int indent);
};

struct BreakStmt : public Stmt {
    void print(int indent);
};

struct ContinueStmt : public Stmt {
    void print(int indent);
};

} // namespace cc

#endif
