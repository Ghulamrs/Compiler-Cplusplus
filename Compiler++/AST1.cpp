// AST1.cpp

#include "AST1.h"

// BuiltinType
void BuiltinType::print(int indent) {
    printIndent(indent);
    std::cout << "Type " << name << std::endl;
}

// ClassType
void ClassType::print(int indent) {
    printIndent(indent);
    std::cout << "ClassType " << className << std::endl;
}

// ReferenceType
void ReferenceType::print(int indent) {
    printIndent(indent);
    std::cout << "Ref to ";
    base->print(0);
}

// VarDecl
void VarDecl::print(int indent) {
    printIndent(indent);
    std::cout << "VarDecl ";
    std::cout << name << " : ";
    type->print(0);
}

// FieldDecl
void FieldDecl::print(int indent) {
    printIndent(indent);
    std::cout << "Field ";
    std::cout << name << " : ";
    type->print(0);
}

// MethodDecl
MethodDecl::~MethodDecl() {
    delete retType;
    for (size_t i = 0; i < params.size(); ++i) delete params[i];
}
void MethodDecl::print(int indent) {
    printIndent(indent);
    std::cout << "Method " << name << " returns ";
    retType->print(0);
    for (size_t i = 0; i < params.size(); ++i) {
        params[i]->print(indent + 1);
    }
}

// ClassDecl
ClassDecl::~ClassDecl() {
    for (size_t i = 0; i < members.size(); ++i) delete members[i];
}
void ClassDecl::print(int indent) {
    printIndent(indent);
    std::cout << "Class " << name << std::endl;
    for (size_t i = 0; i < members.size(); ++i) members[i]->print(indent + 1);
}

// QualifiedName
void QualifiedName::print(int indent) {
    printIndent(indent);
    for (size_t i = 0; i < parts.size(); ++i) {
        if (i) std::cout << "::";
        std::cout << parts[i];
    }
    std::cout << std::endl;
}

// IdentExpr
void IdentExpr::print(int indent) {
    printIndent(indent);
    std::cout << name << std::endl;
}

// MemberAccessExpr
void MemberAccessExpr::print(int indent) {
    printIndent(indent);
    std::cout << (isArrow ? "MemberAccess -> " : "MemberAccess . ");
    base->print(0);
    std::cout << "." << member << std::endl;
}
