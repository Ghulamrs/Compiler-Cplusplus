// Parser1.cpp
//
// C++98 only.  See Parser1.h for what this layer adds to cc::Parser.

#include "Parser1.h"

#include <string>

namespace cxx {

// The base constructor creates the lexer and primes the first token.
Parser::Parser(const std::string &s, Diagnostics &d)
    : cc::Parser(s, d), classBeingParsed(0) {}

// Only the class form is new; the rest goes back to the base class.
cc::Decl *Parser::parseDeclaration() {
    if (cur.kind == TOK_CLASS || cur.kind == TOK_STRUCT) return parseClass();
    // Out-of-line definitions are tried FIRST: `Point::~Point()` must not be
    // handed to a speculative parseType(), whose qualified-name rule would
    // swallow the name and report on the '~'.
    if (Decl *d = parseOutOfLineDefinition()) return d;
    if (Decl *d = parseOperatorFunction()) return d;
    return cc::Parser::parseDeclaration();
}

// A type followed by the `operator` keyword, and nothing else in the grammar
// looks like that -- so the attempt is speculative only up to that point.
Decl *Parser::parseOperatorFunction() {
    const State st = save();
    cc::Type *ret = parseType();                // virtual: knows C++ types
    if (!ret) { restore(st); return 0; }
    if (!(cur.kind == TOK_RESERVED && cur.text == "operator")) {
        delete ret;
        restore(st);
        return 0;
    }

    const int line = cur.line, col = cur.col;
    advance();                                  // consume 'operator'
    const std::string name = operatorMemberName();
    if (name.empty()) { delete ret; skipConstruct(); suppressSync = true; return 0; }

    cc::Function *fn = new cc::Function(ret, name);
    fn->line = line;
    fn->col = col;
    parseFunctionParamsAndBody(fn);
    return fn;
}

// A qualified name at file scope means a member defined outside its class.
// Telling it from an ordinary declaration needs the type, the name and then a
// '::', so the whole attempt is speculative and rewinds if it does not fit.
Decl *Parser::parseOutOfLineDefinition() {
    State st = save();

    // The return type must be parsed WITHOUT the C++ layer's qualified-name
    // rule, or `Point::Point` would be swallowed whole as a class type.  A
    // constructor and a destructor have none at all, so its absence is not a
    // failure here.
    cc::Type *ret = 0;
    switch (cur.kind) {
    case TOK_INT: case TOK_CHAR: case TOK_VOID: case TOK_SHORT: case TOK_LONG:
    case TOK_SIGNED: case TOK_UNSIGNED: case TOK_FLOAT: case TOK_DOUBLE:
    case TOK_CONST:
        ret = cc::Parser::parseType();          // builtins and pointers only
        break;
    case TOK_IDENTIFIER: {
        // A class return type looks like  Point Point::make() : two names in a
        // row.  One name followed by '::' is the qualifier itself.
        const State probe = save();
        advance();
        const bool classReturn = (cur.kind == TOK_IDENTIFIER);
        restore(probe);
        if (classReturn) ret = parseType();
        break;
    }
    default:
        break;
    }

    if (cur.kind != TOK_IDENTIFIER) { delete ret; restore(st); return 0; }
    const std::string className = cur.text;
    const int line = cur.line, col = cur.col;
    advance();
    if (cur.kind != TOK_COLONCOLON) { delete ret; restore(st); return 0; }
    advance();

    bool isDtor = false;
    if (cur.kind == TOK_TILDE) { isDtor = true; advance(); }
    if (cur.kind != TOK_IDENTIFIER) {
        errorAtCurrent("expected a member name after '::'");
        delete ret;
        return 0;
    }
    const std::string member = cur.text;
    advance();
    if (cur.kind != TOK_LPAREN) { delete ret; restore(st); return 0; }

    // A constructor is spelled Point::Point, a destructor Point::~Point.
    const bool isCtor = (!isDtor && !ret && member == className);
    MethodDecl *md = new MethodDecl(ret, isDtor ? "~" + className : member,
                                    ACC_Public);
    md->ownerClass = className;
    md->isConstructor = isCtor;
    md->isDestructor = isDtor;
    md->line = line;
    md->col = col;
    parseFunctionParamsAndBody(md);
    if (!md->body) {
        errorAtCurrent("an out-of-line definition needs a body");
    }
    return md;
}

ClassDecl *Parser::parseClass() {
    // `struct` members default to public, `class` members to private -- the
    // only difference between the two keywords in this subset.
    const Access defaultAccess = (cur.kind == TOK_STRUCT) ? ACC_Public : ACC_Private;
    const int line = cur.line, col = cur.col;
    advance();                                  // consume 'class' or 'struct'

    if (cur.kind != TOK_IDENTIFIER) {
        errorAtCurrent("expected a class name");
        return 0;
    }
    const std::string cname = cur.text;
    advance();
    classNames.insert(cname);                   // before the body, so Node *next; works

    ClassDecl *cd = new ClassDecl(cname);
    cd->line = line;
    cd->col = col;

    // optional base clause:  : [public|private|protected] Base
    if (match(TOK_COLON)) {
        // The same default the keyword sets for members.
        cd->baseAccess = defaultAccess == ACC_Public ? ACC_Public : ACC_Private;
        if (cur.kind == TOK_PUBLIC || cur.kind == TOK_PRIVATE || cur.kind == TOK_PROTECTED) {
            cd->baseAccess = (cur.kind == TOK_PUBLIC)  ? ACC_Public
                           : (cur.kind == TOK_PRIVATE) ? ACC_Private
                                                       : ACC_Protected;
            advance();
        }
        if (cur.kind != TOK_IDENTIFIER) {
            errorAtCurrent("expected a base class name");
        } else {
            cd->baseName = cur.text;
            advance();
        }
        // A deliberate limit, so say so rather than emitting a parse error.
        if (cur.kind == TOK_COMMA) {
            errorAtCurrent("multiple inheritance is not supported in this version");
            while (cur.kind != TOK_LBRACE && cur.kind != TOK_EOF) advance();
        }
    }

    if (!expect(TOK_LBRACE, "to open a class body")) return cd;

    ClassDecl *savedBeing = classBeingParsed;
    classBeingParsed = cd;

    Access access = defaultAccess;
    while (cur.kind != TOK_RBRACE && cur.kind != TOK_EOF) {
        // an access specifier:  public:  private:  protected:
        if (cur.kind == TOK_PUBLIC || cur.kind == TOK_PRIVATE || cur.kind == TOK_PROTECTED) {
            access = (cur.kind == TOK_PUBLIC)  ? ACC_Public
                   : (cur.kind == TOK_PRIVATE) ? ACC_Private
                                               : ACC_Protected;
            advance();
            expect(TOK_COLON, "after an access specifier");
            continue;
        }
        const std::size_t posBefore = lexer->tell().offset;
        suppressSync = false;
        Decl *m = parseMemberDecl(cname, access);
        if (suppressSync) { suppressSync = false; continue; }
        if (m) {
            cd->members.push_back(m);
            // Aliases for lookup; `members` owns them.
            MethodDecl *md = dynamic_cast<MethodDecl*>(m);
            if (md && md->isConstructor) cd->ctors.push_back(md);
            if (md && md->isDestructor) {
                if (cd->dtor) diag.error(md->line, md->col,
                                         "class '" + cname + "' already has a destructor");
                else cd->dtor = md;
            }
        }
        // Only a failed parse resynchronises; only a stalled one is forced.
        if (!m) synchronize();
        if (lexer->tell().offset == posBefore && cur.kind != TOK_EOF) advance();
    }

    classBeingParsed = savedBeing;

    expect(TOK_RBRACE, "to close a class body");
    expect(TOK_SEMI, "after a class definition");
    return cd;
}

// Both start with a type and a name, so the parameter list tells them apart.
// Two tokens of lookahead: IDENT '(' where IDENT is the class's own name.
bool Parser::looksLikeConstructor(const std::string &className) {
    if (cur.kind != TOK_IDENTIFIER || cur.text != className) return false;
    State st = save();
    advance();
    const bool yes = (cur.kind == TOK_LPAREN);
    restore(st);
    return yes;
}

Decl *Parser::parseMemberDecl(const std::string &className, Access access) {
    // `friend` is a grant of access, not a member: it produces nothing here.
    if (cur.kind == TOK_RESERVED && cur.text == "friend") {
        parseFriend();
        suppressSync = true;                    // parseFriend consumed the whole thing
        return 0;
    }
    if (skipReservedConstruct()) return 0;
    // `virtual` precedes the return type and is meaningful only on a method.
    const bool sawVirtual = (cur.kind == TOK_VIRTUAL);
    if (sawVirtual) advance();

    // --- destructor:  ~ClassName ( ) ---
    if (cur.kind == TOK_TILDE) {
        const int line = cur.line, col = cur.col;
        advance();
        if (cur.kind != TOK_IDENTIFIER) {
            errorAtCurrent("expected a class name after '~'");
            return 0;
        }
        const std::string dname = cur.text;
        if (dname != className) {
            errorAtCurrent("destructor name '~" + dname + "' does not match class '"
                           + className + "'");
        }
        advance();
        // No return type: a destructor has none, so retType stays 0.
        MethodDecl *md = new MethodDecl(0, "~" + className, access);
        md->ownerClass = className;
        md->isDestructor = true;
        md->isVirtual = sawVirtual;
        md->line = line;
        md->col = col;
        parseFunctionParamsAndBody(md);
        return md;
    }

    // --- constructor:  ClassName ( ... ) ---
    if (looksLikeConstructor(className)) {
        const int line = cur.line, col = cur.col;
        advance();                              // consume the class name
        MethodDecl *md = new MethodDecl(0, className, access);
        md->ownerClass = className;
        md->isConstructor = true;
        // Recorded, not rejected: that is a rule about meaning, and reporting
        // it here would resynchronise over a member that parsed fine.
        md->isVirtual = sawVirtual;
        md->line = line;
        md->col = col;
        parseFunctionParamsAndBody(md);         // the tail hook picks up  : x(1)
        return md;
    }

    if (cur.kind == TOK_CLASS || cur.kind == TOK_STRUCT) {
        errorAtCurrent("nested classes are not supported in this version");
        skipConstruct();
        suppressSync = true;            // the skip IS the recovery
        return 0;
    }

    cc::Type *t = parseType();                  // virtual: knows C++ types
    if (!t) {
        errorAtCurrent(std::string("expected a member declaration, found ")
                       + tokenName(cur.kind));
        advance();                              // guarantee progress
        return 0;
    }
    // `operator+` is a member whose name happens to be spelled with a keyword
    // and a symbol.  Everything after this point treats it as an ordinary
    // method, which is what makes overload resolution and dispatch work on it
    // without a rule of their own.
    std::string name;
    int line = cur.line, col = cur.col;
    if (cur.kind == TOK_RESERVED && cur.text == "operator") {
        advance();
        name = operatorMemberName();
        if (name.empty()) { delete t; skipConstruct(); suppressSync = true; return 0; }
    } else if (cur.kind != TOK_IDENTIFIER) {
        errorAtCurrent("expected a member name");
        delete t;
        skipConstruct();
        suppressSync = true;            // the skip IS the recovery
        return 0;
    } else {
        name = cur.text;
        advance();
    }

    if (cur.kind == TOK_LPAREN) {
        // MethodDecl IS a cc::Function, so the C layer's routine fills it.
        MethodDecl *md = new MethodDecl(t, name, access);
        md->ownerClass = className;
        md->isVirtual = sawVirtual;
        md->line = line;
        md->col = col;
        parseFunctionParamsAndBody(md);
        return md;
    }

    if (sawVirtual) {
        diag.error(line, col, "'virtual' can only be applied to a member function");
    }
    // In C the array part follows the NAME, and a field is no different.
    t = parseArraySuffixes(t);
    FieldDecl *fd = new FieldDecl(t, name, access);
    fd->ownerClass = className;
    fd->line = line;
    fd->col = col;
    expect(TOK_SEMI, ("after field " + name).c_str());
    return fd;
}

// friend <type> <name> ( params ) ;      -- a grant, defined elsewhere
// friend <type> <name> ( params ) { ... } -- a grant AND the definition, which
//                                            is hoisted to file scope
void Parser::parseFriend() {
    const int line = cur.line, col = cur.col;
    advance();                                  // consume 'friend'

    if (cur.kind == TOK_CLASS || cur.kind == TOK_STRUCT) {
        errorAtCurrent("a friend class is not supported in this version; name the function");
        skipConstruct();
        return;
    }

    cc::Type *ret = parseType();                // virtual: knows C++ types
    if (!ret) {
        errorAtCurrent("expected a return type after 'friend'");
        skipConstruct();
        return;
    }

    std::string name;
    if (cur.kind == TOK_RESERVED && cur.text == "operator") {
        advance();
        name = operatorMemberName();
        if (name.empty()) { delete ret; skipConstruct(); return; }
    } else if (cur.kind == TOK_IDENTIFIER) {
        name = cur.text;
        advance();
    } else {
        errorAtCurrent("expected a function name after 'friend'");
        delete ret;
        skipConstruct();
        return;
    }

    if (classBeingParsed) classBeingParsed->friends.push_back(name);

    cc::Function *fn = new cc::Function(ret, name);
    fn->line = line;
    fn->col = col;
    parseFunctionParamsAndBody(fn);             // handles both ';' and '{'

    // A body makes this the definition, and a definition belongs at file
    // scope.  Without one the grant stands alone and the function is defined
    // somewhere else, so there is nothing to hoist.
    if (fn->body) pending.push_back(fn);
    else delete fn;
}

// The token(s) after `operator`, as the member's name: "operator+".  An
// operator this version does not overload is refused by name here, where the
// program is still readable, rather than misparsed further down.
std::string Parser::operatorMemberName() {
    struct Entry { TokenKind kind; const char *text; };
    static const Entry table[] = {
        { TOK_PLUS, "+" }, { TOK_MINUS, "-" }, { TOK_STAR, "*" },
        { TOK_SLASH, "/" }, { TOK_PERCENT, "%" },
        { TOK_EQ, "==" }, { TOK_NE, "!=" },
        { TOK_LT, "<" }, { TOK_GT, ">" }, { TOK_LE, "<=" }, { TOK_GE, ">=" },
        { TOK_ASSIGN, "=" }
    };
    const int count = static_cast<int>(sizeof(table) / sizeof(table[0]));
    for (int i = 0; i < count; ++i) {
        if (cur.kind == table[i].kind) {
            advance();
            return std::string("operator") + table[i].text;
        }
    }
    if (cur.kind == TOK_LBRACKET) {
        errorAtCurrent("operator[] is not supported in this version");
        return std::string();
    }
    if (cur.kind == TOK_LPAREN) {
        errorAtCurrent("operator() is not supported in this version");
        return std::string();
    }
    errorAtCurrent(std::string("this operator cannot be overloaded in this version"));
    return std::string();
}

bool Parser::namesAClass(const std::string &n) const {
    return classNames.find(n) != classNames.end();
}

// C types plus class names and references; the builtin and pointer forms come
// from cc::Parser.
cc::Type *Parser::parseType() {
    // bool is C++'s, so the C layer's specifier soup knows nothing about it.
    if (cur.kind == TOK_BOOL) {
        const int line = cur.line, col = cur.col;
        advance();
        cc::Type *b = new BoolType();
        b->line = line;
        b->col = col;
        return parsePointerSuffixes(b);
    }

    cc::Type *t = cc::Parser::parseType();      // int, char, void, and T*

    // a qualified / class name like A::B -- new in C++.  An identifier is a
    // type only when it names a class; otherwise it is somebody's variable,
    // and `(x)` is a parenthesised expression rather than a cast.
    bool isTypeName = false;
    if (!t && cur.kind == TOK_IDENTIFIER) {
        isTypeName = namesAClass(cur.text);
        if (!isTypeName) {
            // Two names in a row is a declaration even when the first is not a
            // class we have seen: `Widget w;` deserves "unknown type", not a
            // cascade of expression errors.
            const State probe = save();
            delete parseQualifiedName();
            while (cur.kind == TOK_STAR) advance();
            if (cur.kind == TOK_AMP) advance();
            isTypeName = (cur.kind == TOK_IDENTIFIER);
            restore(probe);
        }
    }
    if (isTypeName) {
        const int line = cur.line, col = cur.col;
        QualifiedName *qn = parseQualifiedName();
        if (qn && !qn->parts.empty()) {
            const std::string last = qn->parts[qn->parts.size() - 1];
            delete qn;
            ClassType *ct = new ClassType(last);
            ct->line = line;
            ct->col = col;
            t = parsePointerSuffixes(ct);       // the * suffixes are the C layer's
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

// Names are not resolved here: whether `x` is a field or the base's name is a
// question about the hierarchy, which is the semantic pass's business.
void Parser::parseFunctionTail(cc::Function *fn) {
    MethodDecl *md = dynamic_cast<MethodDecl*>(fn);
    if (cur.kind != TOK_COLON) return;
    if (!md || !md->isConstructor) {
        errorAtCurrent("only a constructor may have an initialiser list");
        while (cur.kind != TOK_LBRACE && cur.kind != TOK_SEMI && cur.kind != TOK_EOF) advance();
        return;
    }
    advance();                                  // consume ':'

    for (;;) {
        if (cur.kind != TOK_IDENTIFIER) {
            errorAtCurrent("expected a member or base name in the initialiser list");
            break;
        }
        MemberInit init;
        init.name = cur.text;
        init.line = cur.line;
        init.col = cur.col;
        advance();
        if (!expect(TOK_LPAREN, "after an initialiser name")) break;
        while (cur.kind != TOK_RPAREN && cur.kind != TOK_EOF) {
            cc::Expr *a = parseExpression();
            if (!a) break;
            init.args.push_back(a);
            if (!match(TOK_COMMA)) break;
        }
        expect(TOK_RPAREN, "after initialiser arguments");
        md->memberInits.push_back(init);
        if (!match(TOK_COMMA)) break;
    }
}

// C++ adds direct initialisation, Point q(1, 2).
void Parser::parseVarInitializer(cc::VarDecl *vd) {
    if (cur.kind == TOK_LPAREN) {
        advance();
        vd->hasCtorArgs = true;
        while (cur.kind != TOK_RPAREN && cur.kind != TOK_EOF) {
            cc::Expr *a = parseExpression();
            if (!a) break;
            vd->ctorArgs.push_back(a);
            if (!match(TOK_COMMA)) break;
        }
        expect(TOK_RPAREN, "after constructor arguments");
        return;
    }
    cc::Parser::parseVarInitializer(vd);
}

QualifiedName *Parser::parseQualifiedName() {
    if (cur.kind != TOK_IDENTIFIER) return 0;
    QualifiedName *qn = new QualifiedName();
    qn->line = cur.line;
    qn->col = cur.col;
    qn->parts.push_back(cur.text);
    advance();
    while (cur.kind == TOK_COLONCOLON) {
        advance();
        if (cur.kind != TOK_IDENTIFIER) {
            errorAtCurrent("expected an identifier after '::'");
            return qn;
        }
        qn->parts.push_back(cur.text);
        advance();
    }
    return qn;
}

// Numbers, identifiers and parentheses are C's; these are not.
cc::Expr *Parser::parsePrimary() {
    const int line = cur.line, col = cur.col;

    if (cur.kind == TOK_TRUE || cur.kind == TOK_FALSE) {
        const bool v = (cur.kind == TOK_TRUE);
        advance();
        cc::Expr *e = new BoolExpr(v);
        e->line = line; e->col = col;
        return e;
    }

    if (cur.kind == TOK_THIS) {
        advance();
        cc::Expr *e = new ThisExpr();
        e->line = line; e->col = col;
        return e;
    }

    if (cur.kind == TOK_NEW) {
        advance();
        cc::Type *t = parseType();
        if (!t) { errorAtCurrent("expected a type after 'new'"); return 0; }
        NewExpr *ne = new NewExpr(t);
        ne->line = line; ne->col = col;
        if (match(TOK_LPAREN)) {                // constructor arguments
            while (cur.kind != TOK_RPAREN && cur.kind != TOK_EOF) {
                cc::Expr *a = parseExpression();
                if (!a) break;
                ne->args.push_back(a);
                if (!match(TOK_COMMA)) break;
            }
            expect(TOK_RPAREN, "after 'new' arguments");
        }
        return ne;
    }

    if (cur.kind == TOK_DELETE) {
        advance();
        cc::Expr *e = new DeleteExpr(parseUnary());
        e->line = line; e->col = col;
        return e;
    }

    return cc::Parser::parsePrimary();
}

// The inherited parsePostfix() loop asks this every turn, which is what lets
// one loop parse p.getX().y -- alternating C and C++ suffixes.
cc::Expr *Parser::parseMemberSuffix(cc::Expr *base) {
    if (cur.kind != TOK_DOT && cur.kind != TOK_ARROW) return 0;
    const bool arrow = (cur.kind == TOK_ARROW);
    const int line = cur.line, col = cur.col;
    advance();
    if (cur.kind != TOK_IDENTIFIER) {
        errorAtCurrent("expected a member name after '.' or '->'");
        return 0;
    }
    const std::string member = cur.text;
    advance();
    cc::Expr *e = new MemberAccessExpr(base, member, arrow);
    e->line = line; e->col = col;
    return e;
}

} // namespace cxx
