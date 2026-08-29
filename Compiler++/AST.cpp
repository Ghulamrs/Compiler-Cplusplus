// AST.cpp

#include "AST.h"

// NumberExpr
void NumberExpr::print(int indent) {
    printIndent(indent);
    std::cout << value;
    std::cout << std::endl;
}

// IdentExpr
void IdentExpr::print(int indent) {
    printIndent(indent);
    std::cout << name << std::endl;
}

// BinaryExpr
BinaryExpr::~BinaryExpr() {
    delete lhs;
    delete rhs;
}
void BinaryExpr::print(int indent) {
    printIndent(indent);
    std::cout << "(";
    // print inline for readability
    lhs->print(0);
    std::cout << " " << op << " ";
    rhs->print(0);
    std::cout << ")" << std::endl;
}

// DeclStmt
DeclStmt::~DeclStmt() {
    delete init;
}
void DeclStmt::print(int indent) {
    printIndent(indent);
    std::cout << "Decl " << type << " " << name << " = ";
    if (init) init->print(0);
    else std::cout << "null" << std::endl;
}

// ReturnStmt
ReturnStmt::~ReturnStmt() {
    delete expr;
}
void ReturnStmt::print(int indent) {
    printIndent(indent);
    std::cout << "Return ";
    if (expr) expr->print(0);
    else std::cout << "null" << std::endl;
}

// Function
Function::~Function() {
    for (size_t i = 0; i < body.size(); ++i) delete body[i];
}
void Function::print(int indent) {
    printIndent(indent);
    std::cout << "Function " << name << std::endl;
    for (size_t i = 0; i < body.size(); ++i) body[i]->print(indent + 1);
}
