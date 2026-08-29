// AST1.cpp
//
// C++98 only.  See AST1.h for the inheritance layout.

#include "AST1.h"

namespace cxx {

// --- Types added by C++ ---

// ReferenceType
void ReferenceType::print(int indent) {
    printIndent(indent);
    std::cout << "Reference to ";
    base->print(0);
}

// ClassType
void ClassType::print(int indent) {
    printIndent(indent);
    std::cout << "ClassType " << className << std::endl;
}

// --- Declarations ---

// VarDecl
void VarDecl::print(int indent) {
    printIndent(indent);
    std::cout << "VarDecl " << name << " : ";
    if (type) type->print(0);
    else std::cout << "null" << std::endl;
}

// FieldDecl
void FieldDecl::print(int indent) {
    printIndent(indent);
    std::cout << "Field " << name << " : ";
    if (type) type->print(0);
    else std::cout << "null" << std::endl;
}

// MethodDecl
MethodDecl::~MethodDecl() {
    delete retType;
    for (std::size_t i = 0; i < params.size(); ++i) delete params[i];
    for (std::size_t i = 0; i < body.size(); ++i) delete body[i];
}
void MethodDecl::print(int indent) {
    printIndent(indent);
    std::cout << "Method " << name << " returns ";
    if (retType) retType->print(0);
    else std::cout << "null" << std::endl;
    for (std::size_t i = 0; i < params.size(); ++i) {
        params[i]->print(indent + 1);
    }
    for (std::size_t i = 0; i < body.size(); ++i) {
        body[i]->print(indent + 1);
    }
}

// ClassDecl
ClassDecl::~ClassDecl() {
    for (std::size_t i = 0; i < members.size(); ++i) delete members[i];
}
void ClassDecl::print(int indent) {
    printIndent(indent);
    std::cout << "Class " << name << std::endl;
    for (std::size_t i = 0; i < members.size(); ++i) members[i]->print(indent + 1);
}

// --- Qualified name ---
void QualifiedName::print(int indent) {
    printIndent(indent);
    for (std::size_t i = 0; i < parts.size(); ++i) {
        if (i) std::cout << "::";
        std::cout << parts[i];
    }
    std::cout << std::endl;
}

// --- Expressions added by C++ ---
void MemberAccessExpr::print(int indent) {
    printIndent(indent);
    std::cout << (isArrow ? "MemberAccess -> " : "MemberAccess . ");
    base->print(0);
    std::cout << "." << member << std::endl;
}

} // namespace cxx
