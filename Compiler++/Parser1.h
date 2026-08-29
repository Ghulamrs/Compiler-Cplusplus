// Parser1.h

#ifndef PARSER_H
#define PARSER_H

#include "Token.h"
#include "AST.h"
#include <string>
#include <vector>

class Parser {
public:
    Parser(const std::string &s);
    ~Parser();
    std::vector<Decl*> parseTranslationUnit();

private:
    void advance();
    Token cur;
    class Lexer *lexer; // replace void* with typed pointer

    // parsing helpers
    Decl *parseDeclaration();
    ClassDecl *parseClass();
    Decl *parseMemberDecl();
    Type *parseType();
    QualifiedName *parseQualifiedName();
    Expr *parseMemberAccess(Expr *base);

    // reuse expression parsing from before
    Expr *parseExpression();
    Expr *parseAddSub();
    Expr *parseMulDiv();
    Expr *parsePrimary();
};

#endif
