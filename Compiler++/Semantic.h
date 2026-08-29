//
//  Semantic.h
//  Compiler++
//
//  Created by G. R. Akhtar on 29/08/2026.
//
//  PASS 3 -- semantic analysis, run after the layered parser has produced a
//  tree.  Like the parser, it works across BOTH class layers at once:
//
//      cc::   LAYER 1, the C layer   -- functions, statements, expressions
//      cxx::  LAYER 2, the C++ layer -- classes, fields, methods, members
//
//  The analyzer is deliberately NOT split in two: a single tree mixes cc::
//  and cxx:: nodes (see the layering model at the top of AST.h), so one walk
//  resolves both.  Node kinds are told apart with dynamic_cast, which works
//  across the layers because every node derives from cc::ASTNode.
//
//  C++98 only.

#ifndef SEMANTIC_H
#define SEMANTIC_H

#include <vector>
#include <string>

#include "AST.h"       // LAYER 1 nodes: cc::Function, cc::Expr, cc::Stmt, ...
#include "AST1.h"      // LAYER 2 nodes: cxx::Decl, cxx::ClassDecl, ...
#include "SymbolTable.h"

class SemanticAnalyzer {
public:
    SemanticAnalyzer();
    ~SemanticAnalyzer();

    // entry point: the C++ layer's translation unit (class and variable decls)
    void analyze(const std::vector<cxx::Decl*> &units);

    // entry point: a single C layer function body, e.g. from cc::Parser::parse()
    void analyzeFunction(cc::Function *f);

    // diagnostics
    bool hadError() const { return errorCount > 0; }
    int errors() const { return errorCount; }

private:
    SymbolTable symbols;
    int errorCount;

    // types created by the analyzer for expression results; owned here so the
    // callers never have to work out who deletes a temporary type.
    std::vector<cc::Type*> ownedTypes;
    cc::Type *makeInt();

    // passes
    void registerClassNames(const std::vector<cxx::Decl*> &units);
    void registerTopLevel(const std::vector<cxx::Decl*> &units);
    void registerClassMembers(cxx::ClassDecl *cd);
    void analyzeDecl(cxx::Decl *d);
    void analyzeClass(cxx::ClassDecl *cd);
    void analyzeVarDecl(cxx::VarDecl *vd);
    void analyzeFieldDecl(cxx::FieldDecl *fd);
    void analyzeMethodDecl(cxx::MethodDecl *md);
    void analyzeStmt(cc::Stmt *s);
    cc::Type *analyzeExpr(cc::Expr *e, bool &isLValue);

    // helpers
    void error(const std::string &msg);
    Symbol *lookup(const std::string &name);

    // A reference is not a distinct kind of value: `int &r` names an int
    // lvalue.  Everything that asks "what type is this expression" wants the
    // referred-to type, so references are stripped on the way out.
    static cc::Type *stripReference(cc::Type *t);
    // Renders a type for a diagnostic:  int, int*, Point, Point&
    static std::string describe(cc::Type *t);
    // Structural comparison, ignoring references on either side.
    static bool sameType(cc::Type *a, cc::Type *b);
    // Every class name mentioned in a type must have been declared.
    void checkTypeIsKnown(cc::Type *t, const std::string &where);

    // not copyable (C++98 way: declared private, never defined)
    SemanticAnalyzer(const SemanticAnalyzer &);
    SemanticAnalyzer &operator=(const SemanticAnalyzer &);
};

#endif
