#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "ast.h"
#include <bits/stdc++.h>

using namespace std;

struct TypeInfo {
    string name = "error";
    bool isArray = false;
    int arraySize = -1;
    shared_ptr<TypeInfo> elementType;

    static TypeInfo Error();
    static TypeInfo Named(const string& name);
    static TypeInfo ArrayOf(const TypeInfo& base, int size = -1);
    bool isError() const;
    bool isNumeric() const;
    bool isBoolean() const;
    bool isVoid() const;
    string toString() const;
};

enum class SymbolKind {
    Variable,
    Parameter,
    Function,
    Typedef
};

struct SymbolInfo {
    string name;
    SymbolKind kind = SymbolKind::Variable;
    TypeInfo type = TypeInfo::Error();
    vector<TypeInfo> parameters;
    int scopeLevel = 0;
};

class ScopedSymbolTable {
private:
    vector<unordered_map<string, SymbolInfo>> scopes;
    vector<SymbolInfo> history;

public:
    ScopedSymbolTable();
    void enterScope();
    void exitScope();
    int level() const;
    bool declare(const SymbolInfo& symbol);
    SymbolInfo* lookup(const string& name);
    const SymbolInfo* lookup(const string& name) const;
    vector<SymbolInfo> flatten() const;
    void print(const string& filename) const;
};

class TACEmitter {
private:
    vector<string> code_;
    int tempCounter_ = 0;
    int labelCounter_ = 0;

public:
    string newTemp();
    string newLabel(const string& prefix = "L");
    void emit(const string& line);
    void emitLabel(const string& label);
    const vector<string>& code() const;
    void write(const string& filename) const;
};

struct ExprResult {
    TypeInfo type = TypeInfo::Error();
    string place;
    bool isLValue = false;
};

class SemanticAnalyzer {
private:
    ScopedSymbolTable symbols;
    TACEmitter emitter;
    vector<string> errors;
    vector<string> loopExitLabels;
    TypeInfo currentReturnType = TypeInfo::Named("void");

    static string lower(const string& text);
    static bool startsWith(const string& text, const string& prefix);
    static string suffixAfterDot(const string& label);
    static string extractId(ASTNode* node);

    string locationOf(ASTNode* node) const;
    void reportError(const string& message);
    void reportError(ASTNode* node, const string& message);

    void firstPass(ASTNode* root);
    void collectDeclList(ASTNode* node);
    void collectDecl(ASTNode* node);
    void collectTypeDecl(ASTNode* node);
    void collectFunctionSignature(ASTNode* node);

    void analyzeProgram(ASTNode* root);
    void analyzeDeclList(ASTNode* node);
    void analyzeDecl(ASTNode* node);
    void analyzeTypeDecl(ASTNode* node);
    void analyzeVarDeclList(ASTNode* node, bool emitDeclarations);
    void analyzeFunctionDecl(ASTNode* node);
    void analyzeFormalList(ASTNode* node, vector<SymbolInfo>& params);
    void analyzeStmtList(ASTNode* node);
    void analyzeStmt(ASTNode* node);
    void analyzeBlock(ASTNode* node);

    ExprResult analyzeExpr(ASTNode* node);
    ExprResult analyzeAssignment(ASTNode* node);
    ExprResult analyzeBinary(ASTNode* node, const string& op);
    ExprResult analyzeUnary(ASTNode* node);
    ExprResult analyzePrimary(ASTNode* node);
    ExprResult analyzeCall(ASTNode* node);
    ExprResult analyzeArrayAccess(ASTNode* node);
    ExprResult resolveLValue(ASTNode* node);

    TypeInfo typeFromNode(ASTNode* node) const;
    TypeInfo typeFromName(const string& name) const;
    bool compatibleAssignment(const TypeInfo& target, const TypeInfo& value) const;
    TypeInfo promoteNumeric(const TypeInfo& left, const TypeInfo& right) const;
    bool comparable(const TypeInfo& left, const TypeInfo& right) const;
    void emitDeclaration(const SymbolInfo& symbol);

public:
    bool analyze(ASTNode* root);
    const vector<string>& getErrors() const;
    void writeSymbols(const string& filename) const;
    void writeCode(const string& filename) const;
};

#endif
