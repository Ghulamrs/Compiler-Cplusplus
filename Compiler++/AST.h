// AST.h -- LAYER 1, the C layer, namespace `cc`.
//
// ============================ LAYERING MODEL ============================
//
//   Two layers, and the second inherits the first:   cxx::X : public cc::X
//
//     cc::   AST.h  / AST.cpp  / Parser.h  / Parser.cpp    -- what C has
//     cxx::  AST1.h / AST1.cpp / Parser1.h / Parser1.cpp   -- what C++ adds
//
//   Layer 2 never duplicates layer 1.  The test for where a node belongs is
//   simply: does C already have this?
//
//   cc::ASTNode                                   (the single root)
//     |-- cc::Type
//     |     |-- cc::BuiltinType | cc::PointerType        int, T*
//     |     '-- cxx::ReferenceType | cxx::ClassType      T&, Point
//     |-- cc::Expr
//     |     |-- cc::NumberExpr | cc::IdentExpr
//     |     |-- cc::UnaryExpr | cc::BinaryExpr | cc::CallExpr
//     |     '-- cxx::MemberAccessExpr | cxx::ThisExpr
//     |         cxx::NewExpr | cxx::DeleteExpr
//     |-- cc::Stmt
//     |     |-- cc::CompoundStmt      { ... }, also the unit of scope
//     |     |-- cc::DeclStmt | cc::ExprStmt | cc::ReturnStmt
//     |     |-- cc::IfStmt | cc::WhileStmt | cc::ForStmt
//     |     '-- cc::BreakStmt | cc::ContinueStmt
//     '-- cc::Decl
//           |-- cc::VarDecl | cc::Function
//           |-- cxx::FieldDecl  : cc::Decl        adds access
//           |-- cxx::MethodDecl : cc::Function    adds access, virtual, ctor
//           '-- cxx::ClassDecl  : cc::Decl
//
//   Parser: cc::Parser holds the grammar; cxx::Parser derives from it and
//   answers the virtual hooks -- parseDeclaration, parseStatement, parseType,
//   parsePrimary, parseMemberSuffix, parseFunctionTail, parseVarInitializer.
//   The hooks run both ways: parsePostfix() asks parseMemberSuffix(), so
//   p.getX().y parses in one loop; parseStatement() asks parseType(), so a C
//   rule declares C++ types (Point p;) with no C++ statement code.
//
//   The result is ONE tree mixing both namespaces, which is why the semantic
//   pass is not split in two -- it walks the tree and uses dynamic_cast.
//
//   Passes: Parser -> Semantic (+ SymbolTable) -> Layout -> Lower (+ IR).
//
//   C++98 only, everywhere.  No `override` keyword exists, so a derived layer
//   re-declares a virtual with an exactly matching signature or silently hides.
//
// ========================================================================

#ifndef AST_H
#define AST_H

#include <cstddef>
#include <string>
#include <vector>
#include <iostream>

namespace cc {

struct ASTNode {
    int line;                   // copied off the token that started this node
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

// Enums rather than characters: == and && do not fit in a char.
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

struct CallExpr : public Expr {
    Expr *callee;
    std::vector<Expr*> args;
    CallExpr(Expr *c) : callee(c) {}
    ~CallExpr();
    void print(int indent);
};

// --- Declarations -----------------------------------------------------

struct Decl : public ASTNode {
    virtual ~Decl() {}
};

// One node for a variable wherever it appears: at file scope, or inside a
// block wrapped in a DeclStmt.
struct VarDecl : public Decl {
    Type *type;
    std::string name;
    Expr *init;                 // for  = expr ; 0 otherwise
    // Direct initialisation, Point q(1, 2) -- an alternative to init, never both.
    std::vector<Expr*> ctorArgs;
    bool hasCtorArgs;           // true even for  Point q();
    VarDecl(Type *t, const std::string &n, Expr *i)
        : type(t), name(n), init(i), hasCtorArgs(false) {}
    ~VarDecl();
    void print(int indent);
};

struct Stmt;
struct CompoundStmt;

struct Function : public Decl {
    Type *retType;
    std::string name;
    std::vector<VarDecl*> params;
    CompoundStmt *body;         // 0 when this is only a declaration
    Function(Type *r, const std::string &n) : retType(r), name(n), body(0) {}
    ~Function();
    void print(int indent);
    virtual void printSignature(int indent);
    virtual void printBodyPrefix(int indent);   // a ctor's initialiser list
};

// --- Statements -------------------------------------------------------
struct Stmt : public ASTNode {};

// Also the unit of scope, which is why it is a node.
struct CompoundStmt : public Stmt {
    std::vector<Stmt*> body;
    // Class-typed locals in REVERSE declaration order: the destructors the
    // lowering phase runs on every path out of this block.  Filled in by the
    // semantic pass; aliases into the block's own DeclStmts, not owned.
    std::vector<VarDecl*> destroyAtExit;
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

// any of init, cond and step may be 0
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
