// Parser1.cpp
//
// C++98 only.  See Parser1.h for what this layer adds to cc::Parser.

#include "Parser1.h"

#include <string>

namespace cxx {

// The base constructor creates the lexer and primes the first token.
Parser::Parser(const std::string &s, Diagnostics &d) : cc::Parser(s, d) {}

// --- declarations -----------------------------------------------------
// Only the class form is new.  Variables and functions are C's, so they are
// handed straight back to the base class rather than reimplemented.
cc::Decl *Parser::parseDeclaration() {
    if (cur.kind == TOK_CLASS || cur.kind == TOK_STRUCT) return parseClass();
    return cc::Parser::parseDeclaration();
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

    ClassDecl *cd = new ClassDecl(cname);
    cd->line = line;
    cd->col = col;

    // optional base clause:  : [public|private|protected] Base
    if (match(TOK_COLON)) {
        // `struct D : B` inherits publicly, `class D : B` privately -- the same
        // default the keyword sets for members.
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
        // Single inheritance is a deliberate limit of this subset, so say so
        // plainly rather than letting it surface as a confusing parse error.
        if (cur.kind == TOK_COMMA) {
            errorAtCurrent("multiple inheritance is not supported; "
                           "this compiler allows a single base class");
            while (cur.kind != TOK_LBRACE && cur.kind != TOK_EOF) advance();
        }
    }

    if (!expect(TOK_LBRACE, "to open a class body")) return cd;

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
        const int before = diag.errorCount();
        Decl *m = parseMemberDecl(cname, access);
        if (m) cd->members.push_back(m);
        if (diag.errorCount() != before) synchronize();
    }

    expect(TOK_RBRACE, "to close a class body");
    expect(TOK_SEMI, "after a class definition");
    return cd;
}

// A member is a field or a method.  Both start with a type and a name, so the
// parameter list is what tells them apart -- and the method case reuses the C
// layer's parseFunctionRest(), which also parses the body.
Decl *Parser::parseMemberDecl(const std::string &className, Access access) {
    // `virtual` precedes the return type and is meaningful only on a method.
    const bool sawVirtual = (cur.kind == TOK_VIRTUAL);
    const int virtualLine = cur.line, virtualCol = cur.col;
    if (sawVirtual) advance();

    cc::Type *t = parseType();                  // virtual: knows C++ types
    if (!t) {
        errorAtCurrent(std::string("expected a member declaration, found ")
                       + tokenName(cur.kind));
        advance();                              // guarantee progress
        return 0;
    }
    if (cur.kind != TOK_IDENTIFIER) {
        errorAtCurrent("expected a member name");
        delete t;
        return 0;
    }
    const int line = cur.line, col = cur.col;
    const std::string name = cur.text;
    advance();

    if (cur.kind == TOK_LPAREN) {
        // A method is a C function that also knows its access and its class.
        // parseFunctionRest() fills in a cc::Function; because MethodDecl IS a
        // cc::Function, the same call populates the derived node.
        MethodDecl *md = new MethodDecl(t, name, access);
        md->ownerClass = className;
        md->isVirtual = sawVirtual;
        md->line = line;
        md->col = col;
        parseFunctionParamsAndBody(md);
        return md;
    }

    if (sawVirtual) {
        diag.error(virtualLine, virtualCol, "'virtual' can only be applied to a member function");
    }
    FieldDecl *fd = new FieldDecl(t, name, access);
    fd->ownerClass = className;
    fd->line = line;
    fd->col = col;
    expect(TOK_SEMI, ("after field " + name).c_str());
    return fd;
}

// --- overridden extension point: the type grammar ---------------------
// C++ types are C types (int, int*, int**) plus class/qualified names and
// references.  The builtin and pointer forms come from cc::Parser; only the
// genuinely C++ parts are added here.
cc::Type *Parser::parseType() {
    cc::Type *t = cc::Parser::parseType();      // int, char, void, bool, and T*

    // a qualified / class name like A::B -- new in C++
    if (!t && cur.kind == TOK_IDENTIFIER) {
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

// --- overridden extension point: primary expressions ------------------
// Numbers, identifiers and parentheses are C's.  This layer adds the two
// primary forms C has no notion of.
cc::Expr *Parser::parsePrimary() {
    const int line = cur.line, col = cur.col;

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

// --- overridden extension point: the postfix hook ---------------------
// The inherited parsePostfix() loop asks this on every turn.  Returning 0 in
// the C layer and a node here is what lets ONE loop parse  p.getX().y  --
// alternating between a C form (the call) and a C++ form (the member access)
// without either layer knowing about the other's suffix.
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
