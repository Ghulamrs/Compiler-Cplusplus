//
//  SymbolTable.cpp
//  Compiler++
//
//  Created by G. R. Akhtar on 29/08/2026.
//

#include "SymbolTable.h"
#include <cstdlib>

Scope::~Scope() {
    for (std::map<std::string, Symbol*>::iterator it = table.begin(); it != table.end(); ++it) {
        delete it->second;
    }
    for (size_t i = 0; i < children.size(); ++i) delete children[i];
}

bool Scope::insert(const std::string &name, Symbol *sym) {
    if (table.find(name) != table.end()) return false;
    table[name] = sym;
    return true;
}

Symbol *Scope::lookupLocal(const std::string &name) {
    std::map<std::string, Symbol*>::iterator it = table.find(name);
    if (it == table.end()) return 0;
    return it->second;
}

Symbol *Scope::lookup(const std::string &name) {
    Symbol *s = lookupLocal(name);
    if (s) return s;
    for (size_t i = 0; i < children.size(); ++i) {
        s = children[i]->lookup(name);
        if (s) return s;
    }
    return 0;
}

// SymbolTable

SymbolTable::SymbolTable() {
    // start with a global scope
    stack.push_back(new Scope());
}

SymbolTable::~SymbolTable() {
    while (!stack.empty()) {
        delete stack.back();
        stack.pop_back();
    }
}

void SymbolTable::pushScope() {
    stack.push_back(new Scope());
}

void SymbolTable::popScope() {
    if (stack.empty()) return;
    delete stack.back();
    stack.pop_back();
}

bool SymbolTable::insert(const std::string &name, Symbol *sym) {
    if (stack.empty()) return false;
    return stack.back()->insert(name, sym);
}

Symbol *SymbolTable::lookup(const std::string &name) {
    for (int i = (int)stack.size() - 1; i >= 0; --i) {
        Symbol *s = stack[i]->lookupLocal(name);
        if (s) return s;
    }
    return 0;
}

Symbol *SymbolTable::lookupLocal(const std::string &name) {
    if (stack.empty()) return 0;
    return stack.back()->lookupLocal(name);
}
