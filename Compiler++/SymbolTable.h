//symboltable.h

#ifndef SYMBOLTABLE_H
#define SYMBOLTABLE_H

#include <string>
#include <map>
#include <vector>

// The AST lives in two namespaces: the C layer is `cc`, the C++ layer is
// `cxx` (see the layering model at the top of AST.h).  Only the C layer's
// root and type nodes are needed here, so they are forward declared.
namespace cc {
    struct ASTNode;
    struct Type;
}

enum SymbolKind {
    SYM_Var,
    SYM_Type,
    SYM_Method,
    SYM_Field
};

struct Symbol {
    SymbolKind kind;
    std::string name;
    // Any AST node may own a symbol: cxx::Decl for declarations, cc::Function
    // for C-layer functions.  cc::ASTNode is the common root of both.
    cc::ASTNode *decl;  // pointer to declaration node (optional)
    cc::Type *type;     // associated type (for vars/fields/methods return type)
    Symbol(SymbolKind k, const std::string &n, cc::ASTNode *d, cc::Type *t)
        : kind(k), name(n), decl(d), type(t) {}
};

class Scope {
public:
    Scope() {}
    ~Scope();
    bool insert(const std::string &name, Symbol *sym);
    Symbol *lookupLocal(const std::string &name);
    Symbol *lookup(const std::string &name);
    void pushChild(Scope *s) { children.push_back(s); }
private:
    std::map<std::string, Symbol*> table;
    std::vector<Scope*> children;
};

class SymbolTable {
public:
    SymbolTable();
    ~SymbolTable();
    void pushScope();
    void popScope();
    bool insert(const std::string &name, Symbol *sym);
    Symbol *lookup(const std::string &name);
    Symbol *lookupLocal(const std::string &name);
private:
    std::vector<Scope*> stack;
};

#endif
