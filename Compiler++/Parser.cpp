// Parser.cpp
//
// C++98 only.

#include "Parser.h"

#include <cstdlib>
#include <string>

namespace cc {

Parser::Parser(const std::string &s, Diagnostics &d)
    : lexer(createLexer(s)), diag(d) {
    advance();
}

Parser::~Parser() {
    delete lexer;
}

void Parser::advance() {
    cur = lexer->nextToken();
}

// --- error reporting and recovery -------------------------------------

void Parser::errorAtCurrent(const std::string &msg) {
    diag.error(cur.line, cur.col, msg);
}

bool Parser::expect(TokenKind k, const char *context) {
    if (cur.kind == k) { advance(); return true; }
    errorAtCurrent(std::string("expected ") + tokenName(k) + " " + context
                   + ", found " + tokenName(cur.kind));
    return false;
}

bool Parser::match(TokenKind k) {
    if (cur.kind != k) return false;
    advance();
    return true;
}

// Skip to a token that plausibly begins something new.
void Parser::synchronize() {
    while (cur.kind != TOK_EOF) {
        if (cur.kind == TOK_SEMI) { advance(); return; }
        switch (cur.kind) {
        case TOK_RBRACE:
        case TOK_CLASS:
        case TOK_STRUCT:
        case TOK_IF:
        case TOK_WHILE:
        case TOK_FOR:
        case TOK_RETURN:
        case TOK_INT:
        case TOK_CHAR:
        case TOK_VOID:
        case TOK_BOOL:
            return;
        default:
            advance();
        }
    }
}

// --- speculation ------------------------------------------------------

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

// --- translation unit -------------------------------------------------

std::vector<Decl*> Parser::parseTranslationUnit() {
    std::vector<Decl*> units;
    while (cur.kind != TOK_EOF) {
        const std::size_t posBefore = lexer->tell().offset;
        Decl *d = parseDeclaration();       // virtual
        if (d) units.push_back(d);
        // Only a FAILED parse resynchronises: one that produced a node left the
        // parser somewhere sensible even if it also reported.  The progress
        // check is the backstop that makes this loop terminate regardless.
        if (!d) synchronize();
        if (lexer->tell().offset == posBefore && cur.kind != TOK_EOF) advance();
    }
    return units;
}

Function *Parser::parseSingleFunction() {
    Type *t = parseType();
    if (!t) { errorAtCurrent("expected a return type"); return 0; }
    if (cur.kind != TOK_IDENTIFIER) {
        errorAtCurrent("expected a function name");
        delete t;
        return 0;
    }
    const std::string name = cur.text;
    advance();
    return parseFunctionRest(t, name);
}

// --- declarations -----------------------------------------------------

Decl *Parser::parseDeclaration() {
    Type *t = parseType();              // virtual
    if (!t) {
        errorAtCurrent(std::string("expected a declaration, found ") + tokenName(cur.kind));
        advance();                      // guarantee progress
        return 0;
    }
    if (cur.kind != TOK_IDENTIFIER) {
        errorAtCurrent("expected a name in declaration");
        delete t;
        return 0;
    }
    const int line = cur.line, col = cur.col;
    const std::string name = cur.text;
    advance();

    Decl *d = 0;
    if (cur.kind == TOK_LPAREN) d = parseFunctionRest(t, name);
    else                        d = parseVarDeclTail(t, name, line, col);
    if (d) { d->line = line; d->col = col; }
    return d;
}

// '(' [ type IDENT { ',' type IDENT } ] ')' then ';' or a body.
Function *Parser::parseFunctionRest(Type *retType, const std::string &name) {
    Function *fn = new Function(retType, name);
    parseFunctionParamsAndBody(fn);
    return fn;
}

void Parser::parseFunctionParamsAndBody(Function *fn) {
    if (!expect(TOK_LPAREN, "in function declaration")) return;

    while (cur.kind != TOK_RPAREN && cur.kind != TOK_EOF) {
        Type *pt = parseType();                     // virtual
        if (!pt) { errorAtCurrent("expected a parameter type"); break; }
        std::string pname;
        int pline = cur.line, pcol = cur.col;
        if (cur.kind == TOK_IDENTIFIER) { pname = cur.text; advance(); }
        VarDecl *param = new VarDecl(pt, pname, 0);
        param->line = pline;
        param->col = pcol;
        fn->params.push_back(param);
        if (!match(TOK_COMMA)) break;
    }
    expect(TOK_RPAREN, "after parameter list");

    parseFunctionTail(fn);                          // virtual: C++ adds  : x(1)

    if (match(TOK_SEMI)) return;                    // a declaration only
    if (cur.kind == TOK_LBRACE) {
        fn->body = parseBlock();
        return;
    }
    errorAtCurrent("expected ';' or '{' after function " + fn->name);
}

// IDENT already consumed:  [ '=' expr ] ';'
VarDecl *Parser::parseVarDeclTail(Type *type, const std::string &name,
                                  int nameLine, int nameCol) {
    VarDecl *vd = new VarDecl(type, name, 0);
    vd->line = nameLine;
    vd->col = nameCol;
    parseVarInitializer(vd);            // virtual: C++ adds  (args)
    expect(TOK_SEMI, ("after declaration of " + name).c_str());
    return vd;
}

void Parser::parseVarInitializer(VarDecl *vd) {
    if (match(TOK_ASSIGN)) vd->init = parseExpression();
}

// --- statements -------------------------------------------------------

CompoundStmt *Parser::parseBlock() {
    CompoundStmt *block = new CompoundStmt();
    block->line = cur.line;
    block->col = cur.col;
    if (!expect(TOK_LBRACE, "to open a block")) return block;
    while (cur.kind != TOK_RBRACE && cur.kind != TOK_EOF) {
        const std::size_t posBefore = lexer->tell().offset;
        Stmt *s = parseStatement();         // virtual
        if (s) block->body.push_back(s);
        if (!s) synchronize();
        if (lexer->tell().offset == posBefore && cur.kind != TOK_EOF) advance();
    }
    expect(TOK_RBRACE, "to close a block");
    return block;
}

// Declaration and expression share a first token -- Point p; vs p.x = 1; -- so
// the declaration rule is tried first and rewound if it fails.  parseType() is
// VIRTUAL, so this one C rule also declares the C++ layer's types.
Stmt *Parser::parseStatement() {
    const int line = cur.line, col = cur.col;
    Stmt *s = 0;

    switch (cur.kind) {
    case TOK_LBRACE:   s = parseBlock(); break;
    case TOK_IF:       s = parseIf(); break;
    case TOK_WHILE:    s = parseWhile(); break;
    case TOK_FOR:      s = parseFor(); break;
    case TOK_RETURN:   s = parseReturn(); break;
    case TOK_BREAK:    advance(); expect(TOK_SEMI, "after break"); s = new BreakStmt(); break;
    case TOK_CONTINUE: advance(); expect(TOK_SEMI, "after continue"); s = new ContinueStmt(); break;
    case TOK_SEMI:     advance(); return 0;         // an empty statement
    default: {
        State st = save();
        Type *t = parseType();                      // virtual
        if (t) {
            if (cur.kind == TOK_IDENTIFIER) {
                const std::string name = cur.text;
                const int nameLine = cur.line, nameCol = cur.col;
                advance();
                s = new DeclStmt(parseVarDeclTail(t, name, nameLine, nameCol));
                break;
            }
            delete t;                               // a type, but not a declaration
        }
        restore(st);
        s = parseExprStatement();
        break;
    }
    }

    if (s) { s->line = line; s->col = col; }
    return s;
}

Stmt *Parser::parseIf() {
    advance();                                      // consume 'if'
    expect(TOK_LPAREN, "after 'if'");
    Expr *cond = parseExpression();
    expect(TOK_RPAREN, "after if condition");
    Stmt *thenBranch = parseStatement();
    Stmt *elseBranch = 0;
    if (match(TOK_ELSE)) elseBranch = parseStatement();
    return new IfStmt(cond, thenBranch, elseBranch);
}

Stmt *Parser::parseWhile() {
    advance();                                      // consume 'while'
    expect(TOK_LPAREN, "after 'while'");
    Expr *cond = parseExpression();
    expect(TOK_RPAREN, "after while condition");
    return new WhileStmt(cond, parseStatement());
}

Stmt *Parser::parseFor() {
    advance();                                      // consume 'for'
    expect(TOK_LPAREN, "after 'for'");
    Stmt *init = 0;
    if (cur.kind != TOK_SEMI) init = parseStatement();   // consumes its own ';'
    else advance();
    Expr *cond = 0;
    if (cur.kind != TOK_SEMI) cond = parseExpression();
    expect(TOK_SEMI, "after for condition");
    Expr *step = 0;
    if (cur.kind != TOK_RPAREN) step = parseExpression();
    expect(TOK_RPAREN, "after for clauses");
    return new ForStmt(init, cond, step, parseStatement());
}

Stmt *Parser::parseReturn() {
    advance();                                      // consume 'return'
    Expr *e = 0;
    if (cur.kind != TOK_SEMI) e = parseExpression();
    expect(TOK_SEMI, "after return");
    return new ReturnStmt(e);
}

Stmt *Parser::parseExprStatement() {
    Expr *e = parseExpression();
    if (!e) { advance(); return 0; }                // guarantee progress
    expect(TOK_SEMI, "after expression statement");
    return new ExprStmt(e);
}

// --- the type grammar -------------------------------------------------

Type *Parser::parsePointerSuffixes(Type *base) {
    while (cur.kind == TOK_STAR) {
        advance();
        base = new PointerType(base);
    }
    return base;
}

Type *Parser::parseType() {
    match(TOK_CONST);                               // accepted, not enforced
    const char *name = 0;
    switch (cur.kind) {
    case TOK_INT:  name = "int"; break;
    case TOK_CHAR: name = "char"; break;
    case TOK_VOID: name = "void"; break;
    case TOK_BOOL: name = "bool"; break;
    default: return 0;                              // not a type this layer knows
    }
    const int line = cur.line, col = cur.col;
    advance();
    Type *t = new BuiltinType(name);
    t->line = line;
    t->col = col;
    return parsePointerSuffixes(t);
}

// --- the precedence chain ---------------------------------------------
// Each level parses the one below and loops on its own operators, so reading
// top to bottom is reading the precedence table.

Expr *Parser::parseExpression() {
    return parseAssign();
}

// Loosest, and groups RIGHT: a = b = c is a = (b = c).  Whether the left side
// is assignable is the semantic pass's question, not the grammar's.
Expr *Parser::parseAssign() {
    Expr *left = parseLogicalOr();
    if (left && cur.kind == TOK_ASSIGN) {
        const int line = cur.line, col = cur.col;
        advance();
        Expr *right = parseAssign();
        Expr *e = new BinaryExpr(BIN_Assign, left, right);
        e->line = line; e->col = col;
        return e;
    }
    return left;
}

Expr *Parser::parseLogicalOr() {
    Expr *left = parseLogicalAnd();
    while (left && cur.kind == TOK_OROR) {
        const int line = cur.line, col = cur.col;
        advance();
        Expr *e = new BinaryExpr(BIN_LOr, left, parseLogicalAnd());
        e->line = line; e->col = col;
        left = e;
    }
    return left;
}

Expr *Parser::parseLogicalAnd() {
    Expr *left = parseEquality();
    while (left && cur.kind == TOK_ANDAND) {
        const int line = cur.line, col = cur.col;
        advance();
        Expr *e = new BinaryExpr(BIN_LAnd, left, parseEquality());
        e->line = line; e->col = col;
        left = e;
    }
    return left;
}

Expr *Parser::parseEquality() {
    Expr *left = parseRelational();
    while (left && (cur.kind == TOK_EQ || cur.kind == TOK_NE)) {
        const BinaryOp op = (cur.kind == TOK_EQ) ? BIN_EQ : BIN_NE;
        const int line = cur.line, col = cur.col;
        advance();
        Expr *e = new BinaryExpr(op, left, parseRelational());
        e->line = line; e->col = col;
        left = e;
    }
    return left;
}

Expr *Parser::parseRelational() {
    for (Expr *left = parseAddSub(); ; ) {
        BinaryOp op;
        switch (cur.kind) {
        case TOK_LT: op = BIN_LT; break;
        case TOK_GT: op = BIN_GT; break;
        case TOK_LE: op = BIN_LE; break;
        case TOK_GE: op = BIN_GE; break;
        default: return left;
        }
        if (!left) return left;
        const int line = cur.line, col = cur.col;
        advance();
        Expr *e = new BinaryExpr(op, left, parseAddSub());
        e->line = line; e->col = col;
        left = e;
    }
}

Expr *Parser::parseAddSub() {
    Expr *left = parseMulDiv();
    while (left && (cur.kind == TOK_PLUS || cur.kind == TOK_MINUS)) {
        const BinaryOp op = (cur.kind == TOK_PLUS) ? BIN_Add : BIN_Sub;
        const int line = cur.line, col = cur.col;
        advance();
        Expr *e = new BinaryExpr(op, left, parseMulDiv());
        e->line = line; e->col = col;
        left = e;
    }
    return left;
}

Expr *Parser::parseMulDiv() {
    for (Expr *left = parseUnary(); ; ) {
        BinaryOp op;
        switch (cur.kind) {
        case TOK_STAR:    op = BIN_Mul; break;
        case TOK_SLASH:   op = BIN_Div; break;
        case TOK_PERCENT: op = BIN_Mod; break;
        default: return left;
        }
        if (!left) return left;
        const int line = cur.line, col = cur.col;
        advance();
        Expr *e = new BinaryExpr(op, left, parseUnary());
        e->line = line; e->col = col;
        left = e;
    }
}

Expr *Parser::parseUnary() {
    UnaryOp op;
    switch (cur.kind) {
    case TOK_MINUS: op = UN_Neg; break;
    case TOK_NOT:   op = UN_Not; break;
    case TOK_STAR:  op = UN_Deref; break;
    case TOK_AMP:   op = UN_AddrOf; break;
    default: return parsePostfix();
    }
    const int line = cur.line, col = cur.col;
    advance();
    Expr *e = new UnaryExpr(op, parseUnary());      // unary operators nest
    e->line = line; e->col = col;
    return e;
}

// Calls belong to C, member access does not -- so it comes through a virtual
// hook.  One loop handles both in any order, which is what parses p.getX().y
Expr *Parser::parsePostfix() {
    Expr *e = parsePrimary();                       // virtual
    while (e) {
        if (cur.kind == TOK_LPAREN) { e = parseCallSuffix(e); continue; }
        Expr *m = parseMemberSuffix(e);             // virtual; 0 in the C layer
        if (m) { e = m; continue; }
        break;
    }
    return e;
}

Expr *Parser::parseCallSuffix(Expr *callee) {
    const int line = cur.line, col = cur.col;
    advance();                                      // consume '('
    CallExpr *call = new CallExpr(callee);
    call->line = line;
    call->col = col;
    while (cur.kind != TOK_RPAREN && cur.kind != TOK_EOF) {
        Expr *a = parseExpression();
        if (!a) break;
        call->args.push_back(a);
        if (!match(TOK_COMMA)) break;
    }
    expect(TOK_RPAREN, "after call arguments");
    return call;
}

Expr *Parser::parsePrimary() {
    const int line = cur.line, col = cur.col;
    if (cur.kind == TOK_NUMBER) {
        const int v = cur.numberValue;
        advance();
        Expr *e = new NumberExpr(v);
        e->line = line; e->col = col;
        return e;
    }
    if (cur.kind == TOK_IDENTIFIER) {
        const std::string n = cur.text;
        advance();
        Expr *e = new IdentExpr(n);
        e->line = line; e->col = col;
        return e;
    }
    if (cur.kind == TOK_LPAREN) {
        advance();
        Expr *e = parseExpression();
        expect(TOK_RPAREN, "after parenthesised expression");
        return e;
    }
    errorAtCurrent(std::string("expected an expression, found ") + tokenName(cur.kind));
    return 0;
}

// This subset introduces member access with classes, in the layer above.
Expr *Parser::parseMemberSuffix(Expr *) {
    return 0;
}

void Parser::parseFunctionTail(Function *) {
}

} // namespace cc
