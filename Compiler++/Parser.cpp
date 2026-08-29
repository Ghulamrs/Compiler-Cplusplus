// Parser.cpp

#include "Parser.h"
#include "Lexer.h"
#include "AST.h"

#include <cstdlib>
#include <iostream>

// forward declare factory from Token.cpp
extern class Lexer *createLexer(const std::string &s);

Parser::Parser(const std::string &s) {
    lexerPtr = createLexer(s);
    advance();
}

Parser::~Parser() {
    // lexer created with new in Token.cpp; delete via cast
    delete (Lexer*)lexerPtr;
}

void Parser::advance() {
    Lexer *lx = (Lexer*)lexerPtr;
    cur = lx->nextToken();
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
    if (cur.kind != TOK_LBRACE) { std::cerr << "Expected {\n"; std::exit(1); }
    advance();

    Function *fn = new Function(name);
    while (cur.kind != TOK_RBRACE && cur.kind != TOK_EOF) {
        Stmt *s = parseStatement();
        if (s) fn->body.push_back(s);
    }

    if (cur.kind != TOK_RBRACE) { std::cerr << "Expected }\n"; std::exit(1); }
    advance();
    return fn;
}

Stmt *Parser::parseStatement() {
    if (cur.kind == TOK_INT) return parseDecl();
    if (cur.kind == TOK_RETURN) return parseReturn();
    std::cerr << "Unknown statement\n";
    std::exit(1);
    return 0;
}

DeclStmt *Parser::parseDecl() {
    // int IDENT = expr ;
    advance(); // consume int
    if (cur.kind != TOK_IDENTIFIER) { std::cerr << "Expected identifier in decl\n"; std::exit(1); }
    std::string name = cur.text;
    advance();
    if (cur.kind != TOK_ASSIGN) { std::cerr << "Expected = in decl\n"; std::exit(1); }
    advance();
    Expr *e = parseExpression();
    if (cur.kind != TOK_SEMI) { std::cerr << "Expected ;\n"; std::exit(1); }
    advance();
    return new DeclStmt("int", name, e);
}

ReturnStmt *Parser::parseReturn() {
    advance(); // consume return
    Expr *e = parseExpression();
    if (cur.kind != TOK_SEMI) { std::cerr << "Expected ; after return\n"; std::exit(1); }
    advance();
    return new ReturnStmt(e);
}

Expr *Parser::parseExpression() {
    return parseAddSub();
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
    Expr *left = parsePrimary();
    while (cur.kind == TOK_STAR || cur.kind == TOK_SLASH) {
        char op = (cur.kind == TOK_STAR) ? '*' : '/';
        advance();
        Expr *right = parsePrimary();
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
