// Parser1.cpp

#include "Parser.h"
#include "Lexer.h"
#include "AST.h"
#include <cstdlib>
#include <iostream>

// forward factory
extern class Lexer *createLexer(const std::string &s);

Parser::Parser(const std::string &s) {
    lexer = createLexer(s);
    advance();
}

Parser::~Parser() {
    delete lexer;
}

void Parser::advance() {
    cur = lexer->nextToken();
}

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
    // variable or function or other decl
    Type *t = parseType();
    if (!t) { std::cerr << "Expected type in declaration\n"; std::exit(1); }
    if (cur.kind != TOK_IDENTIFIER) { std::cerr << "Expected identifier\n"; std::exit(1); }
    std::string name = cur.text;
    advance();
    // simple var decl ending with ';'
    if (cur.kind == TOK_SEMI) {
        advance();
        return new VarDecl(t, name);
    }
    // TODO: function or initializer handling
    std::cerr << "Unsupported declaration form\n";
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
    // method declaration if '(' follows
    if (cur.kind == TOK_LPAREN) {
        advance();
        MethodDecl *md = new MethodDecl(t, name);
        // parse zero or more params of form 'int x'
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
        if (cur.kind != TOK_SEMI && cur.kind != TOK_LBRACE) { std::cerr << "Expected ; or { after method decl\n"; std::exit(1); }
        // skip method body for now if present
        if (cur.kind == TOK_SEMI) advance();
        else {
            // skip body until matching }
            int depth = 0;
            if (cur.kind == TOK_LBRACE) { advance(); depth = 1; }
            while (depth > 0 && cur.kind != TOK_EOF) {
                if (cur.kind == TOK_LBRACE) ++depth;
                else if (cur.kind == TOK_RBRACE) --depth;
                advance();
            }
        }
        return md;
    }
    // field
    if (cur.kind == TOK_SEMI) { advance(); return new FieldDecl(t, name); }
    std::cerr << "Unsupported member form\n";
    std::exit(1);
    return 0;
}

Type *Parser::parseType() {
    // handle builtin 'int' or qualified names or reference '&'
    if (cur.kind == TOK_INT) {
        advance();
        Type *bt = new BuiltinType("int");
        // check for reference
        if (cur.kind == TOK_AMP) { advance(); return new ReferenceType(bt); }
        return bt;
    }
    // qualified name like A::B
    if (cur.kind == TOK_IDENTIFIER) {
        QualifiedName *qn = parseQualifiedName();
        // build ClassType from last part for now
        if (qn->parts.size() > 0) {
            std::string last = qn->parts[qn->parts.size()-1];
            delete qn;
            Type *ct = new ClassType(last);
            if (cur.kind == TOK_AMP) { advance(); return new ReferenceType(ct); }
            return ct;
        }
    }
    return 0;
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

// parseExpression, parseAddSub, parseMulDiv, parsePrimary reuse earlier implementations
// but extend parsePrimary to allow member access chaining
Expr *Parser::parsePrimary() {
    if (cur.kind == TOK_NUMBER) {
        int v = cur.numberValue;
        advance();
        // assume NumberExpr exists
        return new NumberExpr(v);
    }
    if (cur.kind == TOK_IDENTIFIER) {
        Expr *e = new IdentExpr(cur.text);
        advance();
        // handle chained member access
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
