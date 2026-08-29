// Parser.cpp
//
// C++98 only.

#include "Parser.h"
#include "Lexer.h"
#include "AST.h"

#include <cstdlib>
#include <iostream>

namespace cc {

Parser::Parser(const std::string &s) : lexer(createLexer(s)) {
    advance();
}

Parser::~Parser() {
    delete lexer;
}

void Parser::advance() {
    cur = lexer->nextToken();
}

// --- speculation ------------------------------------------------------
// The whole of the parser's position is the lookahead token plus the lexer's
// offset, so saving and restoring those two is a complete rewind.

Parser::State Parser::save() const {
    State st;
    st.cur = cur;
    st.lexPos = lexer->tell();
    return st;
}

void Parser::restore(const State &st) {
    cur = st.cur;
    lexer->seek(st.lexPos);
}

Function *Parser::parse() {
    return parseFunction();
}

Function *Parser::parseFunction() {
    // expect: int main() { ... }
    if (cur.kind != TOK_INT) { std::cerr << "Expected int\n"; std::exit(1); }
    advance();
    if (cur.kind != TOK_IDENTIFIER) { std::cerr << "Expected identifier\n"; std::exit(1); }
    std::string name = cur.text;
    advance();
    if (cur.kind != TOK_LPAREN) { std::cerr << "Expected (\n"; std::exit(1); }
    advance();
    if (cur.kind != TOK_RPAREN) { std::cerr << "Expected )\n"; std::exit(1); }
    advance();
    Function *fn = new Function(name);
    parseBlock(fn->body);
    return fn;
}

// '{' statement* '}'  -- shared with the C++ layer's function and method bodies.
void Parser::parseBlock(std::vector<Stmt*> &out) {
    if (cur.kind != TOK_LBRACE) { std::cerr << "Expected {\n"; std::exit(1); }
    advance();
    while (cur.kind != TOK_RBRACE && cur.kind != TOK_EOF) {
        Stmt *s = parseStatement();          // virtual: a C++ layer may extend
        if (s) out.push_back(s);
    }
    if (cur.kind != TOK_RBRACE) { std::cerr << "Expected }\n"; std::exit(1); }
    advance();
}

// A statement is a return, a declaration, or an expression.  Declaration and
// expression cannot be told apart by their first token in C++ --  Point p;  vs
// p.x = 1;  -- so the declaration rule is tried first and rewound if it fails.
// Because parseType() is VIRTUAL, this one C-layer rule also declares the C++
// layer's types:  Point p;  and  int &r = p.x;  need no C++-layer statement code.
Stmt *Parser::parseStatement() {
    if (cur.kind == TOK_RETURN) return parseReturn();

    State st = save();
    Type *t = parseType();               // virtual
    if (t) {
        if (cur.kind == TOK_IDENTIFIER) return parseDeclTail(t);
        delete t;                        // a type, but not a declaration
    }
    restore(st);
    return parseExprStatement();
}

// The type is already consumed:  IDENT [ '=' expr ] ';'
DeclStmt *Parser::parseDeclTail(Type *type) {
    if (cur.kind != TOK_IDENTIFIER) { std::cerr << "Expected identifier in decl\n"; std::exit(1); }
    std::string name = cur.text;
    advance();
    Expr *init = 0;
    if (cur.kind == TOK_ASSIGN) {
        advance();
        init = parseExpression();
    }
    if (cur.kind != TOK_SEMI) { std::cerr << "Expected ; after declaration of " << name << "\n"; std::exit(1); }
    advance();
    return new DeclStmt(type, name, init);
}

ExprStmt *Parser::parseExprStatement() {
    Expr *e = parseExpression();
    if (cur.kind != TOK_SEMI) { std::cerr << "Expected ; after expression\n"; std::exit(1); }
    advance();
    return new ExprStmt(e);
}

ReturnStmt *Parser::parseReturn() {
    advance(); // consume return
    Expr *e = parseExpression();
    if (cur.kind != TOK_SEMI) { std::cerr << "Expected ; after return\n"; std::exit(1); }
    advance();
    return new ReturnStmt(e);
}

// --- type grammar -----------------------------------------------------
// C knows builtin types and pointers to them.  parseType() is virtual so the
// C++ layer can extend it with qualified names (A::B) and references (T&).

Type *Parser::parsePointerSuffixes(Type *base) {
    while (cur.kind == TOK_STAR) {
        advance();
        base = new PointerType(base);
    }
    return base;
}

Type *Parser::parseType() {
    if (cur.kind == TOK_INT) {
        advance();
        return parsePointerSuffixes(new BuiltinType("int"));
    }
    return 0;   // not a type this layer understands
}

// --- expression chain -------------------------------------------------
// Defined once here.  Because parsePrimary() is virtual, a derived layer
// gets the whole precedence chain for free and still contributes its own
// primary forms:  parseAddSub -> parseMulDiv -> parsePrimary (dispatched).

Expr *Parser::parseExpression() {
    return parseAssign();
}

// Assignment binds loosest and groups to the RIGHT:  a = b = c  is  a = (b = c).
// Whether the left side is assignable is not a grammar question -- the semantic
// pass decides that, which is why it is not checked here.
Expr *Parser::parseAssign() {
    Expr *left = parseAddSub();
    if (cur.kind == TOK_ASSIGN) {
        advance();
        Expr *right = parseAssign();
        return new BinaryExpr('=', left, right);
    }
    return left;
}

Expr *Parser::parseAddSub() {
    Expr *left = parseMulDiv();
    while (cur.kind == TOK_PLUS || cur.kind == TOK_MINUS) {
        char op = (cur.kind == TOK_PLUS) ? '+' : '-';
        advance();
        Expr *right = parseMulDiv();
        left = new BinaryExpr(op, left, right);
    }
    return left;
}

Expr *Parser::parseMulDiv() {
    Expr *left = parsePrimary();             // virtual
    while (cur.kind == TOK_STAR || cur.kind == TOK_SLASH) {
        char op = (cur.kind == TOK_STAR) ? '*' : '/';
        advance();
        Expr *right = parsePrimary();        // virtual
        left = new BinaryExpr(op, left, right);
    }
    return left;
}

Expr *Parser::parsePrimary() {
    if (cur.kind == TOK_NUMBER) {
        int v = cur.numberValue;
        advance();
        return new NumberExpr(v);
    }
    if (cur.kind == TOK_IDENTIFIER) {
        std::string n = cur.text;
        advance();
        return new IdentExpr(n);
    }
    if (cur.kind == TOK_LPAREN) {
        advance();
        Expr *e = parseExpression();
        if (cur.kind != TOK_RPAREN) { std::cerr << "Expected )\n"; std::exit(1); }
        advance();
        return e;
    }
    std::cerr << "Unexpected token in primary\n";
    std::exit(1);
    return 0;
}

} // namespace cc
