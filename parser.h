#ifndef PARSER_H
#define PARSER_H

#include "TokenBuffer.h"
#include "ast.h"
#include <bits/stdc++.h>

using namespace std;

class Parser {
private:
    TokenBuffer& ts;
    LexToken currentToken;

    void advance();

    bool check(const string& tipo);
    bool isTypeToken(const string& tipo);
    bool isStmtStart(const string& tipo);
    bool isExprStart(const string& tipo);

    void printNonTerminal(const string& name);
    void syntaxError(const string& msg);

    bool match(const string& expected);
    void synchronize(const set<string>& sync);

public:
    Parser(TokenBuffer& ts);

    ASTNode* Program();
    ASTNode* DeclList();
    ASTNode* Decl();
    ASTNode* TypeDecl();
    ASTNode* Type();

    ASTNode* VarDeclList();
    ASTNode* VarDecl();
    ASTNode* VarDeclAfterType(ASTNode* typeNode, const string& firstId);
    ASTNode* ArrayOpt();

    ASTNode* FunctionDeclAfterType(ASTNode* typeNode, const string& id);

    ASTNode* FormalList();
    ASTNode* FormalListRest(ASTNode* list);
    ASTNode* Param();

    ASTNode* StmtList();
    ASTNode* Stmt();
    ASTNode* ExprList();

    ASTNode* Expr();
    ASTNode* Igual();
    ASTNode* OuExpr();
    ASTNode* EExpr();
    ASTNode* EqExpr();
    ASTNode* RelExpr();
    ASTNode* SomExpr();
    ASTNode* MultExpr();
    ASTNode* UnaryExpr();
    ASTNode* Primary();
    ASTNode* PrimaryRest(ASTNode* base);
};

#endif