// AST.cpp
//
// C++98 only.

#include "AST.h"

namespace cc {

// --- Types ---

// BuiltinType
void BuiltinType::print(int indent) {
    printIndent(indent);
    std::cout << "Type " << name << std::endl;
}

// PointerType
void PointerType::print(int indent) {
    printIndent(indent);
    std::cout << "Pointer to ";
    base->print(0);
}

// --- Expressions ---

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
    delete type;
    delete init;
}
void DeclStmt::print(int indent) {
    printIndent(indent);
    std::cout << "Decl " << name << " : ";
    if (type) type->print(0);
    else std::cout << "null" << std::endl;
    if (init) {
        printIndent(indent + 1);
        std::cout << "= ";
        init->print(0);
    }
}

// ExprStmt
ExprStmt::~ExprStmt() {
    delete expr;
}
void ExprStmt::print(int indent) {
    printIndent(indent);
    std::cout << "ExprStmt ";
    if (expr) expr->print(0);
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
    for (std::size_t i = 0; i < body.size(); ++i) delete body[i];
}
void Function::print(int indent) {
    printIndent(indent);
    std::cout << "Function " << name << std::endl;
    for (std::size_t i = 0; i < body.size(); ++i) body[i]->print(indent + 1);
}

} // namespace cc
