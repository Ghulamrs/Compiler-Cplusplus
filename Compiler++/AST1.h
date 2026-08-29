// AST1.h

#ifndef AST_H
#define AST_H

#include <string>
#include <vector>
#include <iostream>

struct ASTNode {
    virtual ~ASTNode() {}
    virtual void print(int indent = 0) = 0;
protected:
    void printIndent(int n) {
        for (int i = 0; i < n; ++i) std::cout << "  ";
    }
};

// --- Types ---
struct Type : public ASTNode {
    virtual ~Type() {}
};

struct BuiltinType : public Type {
    std::string name;
    BuiltinType(const std::string &n) : name(n) {}
    void print(int indent);
};

struct ClassType : public Type {
    std::string className;
    ClassType(const std::string &n) : className(n) {}
    void print(int indent);
};

struct ReferenceType : public Type {
    Type *base;
    ReferenceType(Type *b) : base(b) {}
    ~ReferenceType() { delete base; }
    void print(int indent);
};

// --- Declarations ---
struct Decl : public ASTNode {
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

struct MethodDecl : public Decl {
    Type *retType;
    std::string name;
    std::vector<VarDecl*> params;
    MethodDecl(Type *r, const std::string &n) : retType(r), name(n) {}
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

// --- Qualified name ---
struct QualifiedName : public ASTNode {
    std::vector<std::string> parts;
    QualifiedName() {}
    void print(int indent);
};

// --- Expressions ---
struct Expr : public ASTNode {};

struct IdentExpr : public Expr {
    std::string name;
    IdentExpr(const std::string &n) : name(n) {}
    void print(int indent);
};

struct MemberAccessExpr : public Expr {
    Expr *base;
    std::string member;
    bool isArrow;
    MemberAccessExpr(Expr *b, const std::string &m, bool arrow) : base(b), member(m), isArrow(arrow) {}
    ~MemberAccessExpr() { delete base; }
    void print(int indent);
};

// reuse NumberExpr, BinaryExpr from your existing AST if present

#endif
