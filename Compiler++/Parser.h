// Parser.h

#ifndef PARSER_H
#define PARSER_H

#include "Token.h"
#include "AST.h"
#include <string>

class Parser {
public:
    Parser(const std::string &s);
    ~Parser();
    Function *parse();

private:
    // lexer instance
    void advance();
    Token cur;

    // parsing helpers
    Function *parseFunction();
    Stmt *parseStatement();
    DeclStmt *parseDecl();
    ReturnStmt *parseReturn();
    Expr *parseExpression();
    Expr *parseAddSub();
    Expr *parseMulDiv();
    Expr *parsePrimary();

    // internal lexer pointer
    void *lexerPtr;
};

#endif
