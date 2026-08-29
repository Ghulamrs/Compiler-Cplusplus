// AST1.cpp
//
// C++98 only.  See AST1.h for the inheritance layout.

#include "AST1.h"

namespace cxx {

const char *accessText(Access a) {
    switch (a) {
    case ACC_Public:    return "public";
    case ACC_Private:   return "private";
    case ACC_Protected: return "protected";
    }
    return "?";
}

// --- Types added by C++ ---

void ReferenceType::print(int indent) {
    printIndent(indent);
    std::cout << "reference to ";
    base->print(0);
}

void ClassType::print(int indent) {
    printIndent(indent);
    std::cout << "class " << className << std::endl;
}

// --- Declarations ---

void FieldDecl::print(int indent) {
    printIndent(indent);
    std::cout << "Field " << accessText(access) << " " << name << " : ";
    if (type) type->print(0);
    else std::cout << "<none>" << std::endl;
}

// Everything but this first line comes from cc::Function::print().
void MethodDecl::printSignature(int indent) {
    printIndent(indent);
    std::cout << "Method " << accessText(access) << " ";
    if (isVirtual) std::cout << "virtual ";
    if (overrides) std::cout << "overriding ";
    if (!ownerClass.empty()) std::cout << ownerClass << "::";
    std::cout << name << " returns ";
    if (retType) retType->print(0);
    else std::cout << "<none>" << std::endl;
}

ClassDecl::~ClassDecl() {
    for (std::size_t i = 0; i < members.size(); ++i) delete members[i];
}

void ClassDecl::print(int indent) {
    printIndent(indent);
    std::cout << "Class " << name;
    if (!baseName.empty()) std::cout << " : " << accessText(baseAccess) << " " << baseName;
    std::cout << std::endl;
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
    std::cout << "Member " << (isArrow ? "->" : ".") << member << std::endl;
    if (base) base->print(indent + 1);
}

void ThisExpr::print(int indent) {
    printIndent(indent);
    std::cout << "this" << std::endl;
}

NewExpr::~NewExpr() {
    delete allocType;
    for (std::size_t i = 0; i < args.size(); ++i) delete args[i];
}

void NewExpr::print(int indent) {
    printIndent(indent);
    std::cout << "New ";
    if (allocType) allocType->print(0);
    else std::cout << "<none>" << std::endl;
    for (std::size_t i = 0; i < args.size(); ++i) args[i]->print(indent + 1);
}

void DeleteExpr::print(int indent) {
    printIndent(indent);
    std::cout << "Delete" << std::endl;
    if (operand) operand->print(indent + 1);
}

} // namespace cxx
