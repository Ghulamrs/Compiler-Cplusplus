// Parser1.cpp
//
// C++98 only.  See Parser1.h for what this layer adds to cc::Parser.

#include "Parser1.h"
#include "Lexer.h"
#include "AST1.h"
#include <cstdlib>
#include <iostream>

namespace cxx {

// The base constructor creates the lexer and primes the first token.
Parser::Parser(const std::string &s) : cc::Parser(s) {}

std::vector<Decl*> Parser::parseTranslationUnit() {
    std::vector<Decl*> units;
    while (cur.kind != TOK_EOF) {
        Decl *d = parseDeclaration();
        if (d) units.push_back(d);
    }
    return units;
}

Decl *Parser::parseDeclaration() {
    if (cur.kind == TOK_CLASS || cur.kind == TOK_STRUCT) return parseClass();
    // variable or function declaration
    Type *t = parseType();
    if (!t) { std::cerr << "Expected type in declaration\n"; std::exit(1); }
    if (cur.kind != TOK_IDENTIFIER) { std::cerr << "Expected identifier\n"; std::exit(1); }
    std::string name = cur.text;
    advance();
    // a function at file scope:  int main() { ... }
    if (cur.kind == TOK_LPAREN) {
        return parseFunctionRest(t, name);
    }
    // simple var decl ending with ';'
    if (cur.kind == TOK_SEMI) {
        advance();
        return new VarDecl(t, name);
    }
    std::cerr << "Unsupported declaration form for '" << name << "'\n";
    std::exit(1);
    return 0;
}

// '(' [ type IDENT { ',' type IDENT } ] ')' followed by ';' or a body.
// The body is parsed by cc::Parser::parseBlock(), inherited from the C layer,
// so C++ contributes the signature and C contributes the statements.
MethodDecl *Parser::parseFunctionRest(Type *retType, const std::string &name) {
    if (cur.kind != TOK_LPAREN) { std::cerr << "Expected (\n"; std::exit(1); }
    advance();
    MethodDecl *md = new MethodDecl(retType, name);
    while (cur.kind != TOK_RPAREN && cur.kind != TOK_EOF) {
        Type *pt = parseType();
        if (!pt) { std::cerr << "Expected param type\n"; std::exit(1); }
        if (cur.kind != TOK_IDENTIFIER) { std::cerr << "Expected param name\n"; std::exit(1); }
        std::string pname = cur.text;
        advance();
        md->params.push_back(new VarDecl(pt, pname));
        if (cur.kind == TOK_COMMA) advance();
    }
    if (cur.kind != TOK_RPAREN) { std::cerr << "Expected )\n"; std::exit(1); }
    advance();
    if (cur.kind == TOK_SEMI) {          // just a declaration
        advance();
        return md;
    }
    if (cur.kind == TOK_LBRACE) {        // a definition: keep the body
        md->hasBody = true;
        parseBlock(md->body);            // inherited from cc::Parser
        return md;
    }
    std::cerr << "Expected ; or { after function " << name << "\n";
    std::exit(1);
    return 0;
}

ClassDecl *Parser::parseClass() {
    // consume 'class' or 'struct'
    advance();
    if (cur.kind != TOK_IDENTIFIER) { std::cerr << "Expected class name\n"; std::exit(1); }
    std::string cname = cur.text;
    advance();
    if (cur.kind != TOK_LBRACE) { std::cerr << "Expected { in class\n"; std::exit(1); }
    advance();
    ClassDecl *cd = new ClassDecl(cname);
    // simple member parsing until '}'
    while (cur.kind != TOK_RBRACE && cur.kind != TOK_EOF) {
        // handle access specifiers like 'public:'
        if (cur.kind == TOK_PUBLIC || cur.kind == TOK_PRIVATE || cur.kind == TOK_PROTECTED) {
            advance();
            if (cur.kind != TOK_COLON) { std::cerr << "Expected : after access\n"; std::exit(1); }
            advance();
            continue;
        }
        Decl *m = parseMemberDecl();
        if (m) cd->members.push_back(m);
    }
    if (cur.kind != TOK_RBRACE) { std::cerr << "Expected } after class\n"; std::exit(1); }
    advance();
    if (cur.kind == TOK_SEMI) advance();
    return cd;
}

Decl *Parser::parseMemberDecl() {
    Type *t = parseType();
    if (!t) { std::cerr << "Expected type in member\n"; std::exit(1); }
    if (cur.kind != TOK_IDENTIFIER) { std::cerr << "Expected member name\n"; std::exit(1); }
    std::string name = cur.text;
    advance();
    // a method if '(' follows -- same rule as a free function
    if (cur.kind == TOK_LPAREN) {
        return parseFunctionRest(t, name);
    }
    // field
    if (cur.kind == TOK_SEMI) { advance(); return new FieldDecl(t, name); }
    std::cerr << "Unsupported member form\n";
    std::exit(1);
    return 0;
}

// --- overridden extension point: the type grammar ---------------------
// C++ types are C types (int, int*, int**) plus class/qualified names and
// references.  The builtin and pointer forms come from cc::Parser; only the
// genuinely C++ parts are added here.
cc::Type *Parser::parseType() {
    // int, int*, int** ... handled entirely by the C layer
    cc::Type *t = cc::Parser::parseType();

    // qualified / class name like A::B  -- new in C++
    if (!t && cur.kind == TOK_IDENTIFIER) {
        QualifiedName *qn = parseQualifiedName();
        if (qn && qn->parts.size() > 0) {
            std::string last = qn->parts[qn->parts.size()-1];
            delete qn;
            // the * suffixes reuse the C layer's helper
            t = parsePointerSuffixes(new ClassType(last));
        } else {
            delete qn;
        }
    }

    if (!t) return 0;

    // reference suffix T& -- new in C++, and only one is legal
    if (cur.kind == TOK_AMP) {
        advance();
        t = new ReferenceType(t);
    }
    return t;
}

QualifiedName *Parser::parseQualifiedName() {
    QualifiedName *qn = new QualifiedName();
    if (cur.kind != TOK_IDENTIFIER) { delete qn; return 0; }
    qn->parts.push_back(cur.text);
    advance();
    while (cur.kind == TOK_COLONCOLON) {
        advance();
        if (cur.kind != TOK_IDENTIFIER) { std::cerr << "Expected identifier after ::\n"; std::exit(1); }
        qn->parts.push_back(cur.text);
        advance();
    }
    return qn;
}

// --- the one overridden extension point -------------------------------
// C++ primaries are C primaries plus member-access chaining.  Numbers,
// parenthesised expressions and the whole +-*/ precedence chain come from
// cc::Parser; only the identifier case is extended here.  Because
// cc::Parser::parseMulDiv() calls parsePrimary() virtually, an expression
// such as (a.b + 1) * 2 is parsed by both layers cooperatively.
cc::Expr *Parser::parsePrimary() {
    if (cur.kind == TOK_IDENTIFIER) {
        cc::Expr *e = new cc::IdentExpr(cur.text);
        advance();
        while (cur.kind == TOK_DOT || cur.kind == TOK_ARROW) {
            bool arrow = (cur.kind == TOK_ARROW);
            advance();
            if (cur.kind != TOK_IDENTIFIER) { std::cerr << "Expected member name\n"; std::exit(1); }
            std::string mem = cur.text;
            advance();
            e = new MemberAccessExpr(e, mem, arrow);
        }
        return e;
    }
    // everything else is unchanged from C
    return cc::Parser::parsePrimary();
}

} // namespace cxx
