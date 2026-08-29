// AST.h

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

struct BinaryExpr : public Expr {
    char op;
    Expr *lhs;
    Expr *rhs;
    BinaryExpr(char o, Expr *l, Expr *r) : op(o), lhs(l), rhs(r) {}
    ~BinaryExpr();
    void print(int indent);
};

struct Stmt : public ASTNode {};

struct DeclStmt : public Stmt {
    std::string type;
    std::string name;
    Expr *init;
    DeclStmt(const std::string &t, const std::string &n, Expr *i) : type(t), name(n), init(i) {}
    ~DeclStmt();
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

#endif
