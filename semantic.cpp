#include "semantic.h"

static string kindToString(SymbolKind kind) {
    switch (kind) {
    case SymbolKind::Variable: return "variable";
    case SymbolKind::Parameter: return "parameter";
    case SymbolKind::Function: return "function";
    case SymbolKind::Typedef: return "typedef";
    }
    return "unknown";
}

TypeInfo TypeInfo::Error() {
    return TypeInfo{};
}

TypeInfo TypeInfo::Named(const string& name) {
    TypeInfo info;
    info.name = name;
    return info;
}

TypeInfo TypeInfo::ArrayOf(const TypeInfo& base, int size) {
    TypeInfo info;
    info.name = base.name;
    info.isArray = true;
    info.arraySize = size;
    info.elementType = make_shared<TypeInfo>(base);
    return info;
}

bool TypeInfo::isError() const { return name == "error"; }
bool TypeInfo::isNumeric() const { return name == "int" || name == "float" || name == "double" || name == "char"; }
bool TypeInfo::isBoolean() const { return name == "bool"; }
bool TypeInfo::isVoid() const { return name == "void"; }

string TypeInfo::toString() const {
    if (isError()) return "error";
    string out = name;
    if (isArray) {
        out += "[";
        if (arraySize >= 0) out += to_string(arraySize);
        out += "]";
    }
    return out;
}

ScopedSymbolTable::ScopedSymbolTable() {
    enterScope();
}

void ScopedSymbolTable::enterScope() {
    scopes.push_back({});
}

void ScopedSymbolTable::exitScope() {
    if (!scopes.empty()) scopes.pop_back();
}

int ScopedSymbolTable::level() const {
    return static_cast<int>(scopes.size()) - 1;
}

bool ScopedSymbolTable::declare(const SymbolInfo& symbol) {
    if (scopes.empty()) enterScope();
    auto& current = scopes.back();
    if (current.find(symbol.name) != current.end()) return false;
    SymbolInfo copy = symbol;
    copy.scopeLevel = level();
    current[symbol.name] = copy;
    history.push_back(copy);
    return true;
}

SymbolInfo* ScopedSymbolTable::lookup(const string& name) {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) return &found->second;
    }
    return nullptr;
}

const SymbolInfo* ScopedSymbolTable::lookup(const string& name) const {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) return &found->second;
    }
    return nullptr;
}

vector<SymbolInfo> ScopedSymbolTable::flatten() const {
    vector<SymbolInfo> out = history;
    sort(out.begin(), out.end(), [](const SymbolInfo& a, const SymbolInfo& b) {
        if (a.scopeLevel != b.scopeLevel) return a.scopeLevel < b.scopeLevel;
        return a.name < b.name;
    });
    return out;
}

void ScopedSymbolTable::print(const string& filename) const {
    ofstream out(filename);
    if (!out.is_open()) return;

    out << left << setw(18) << "NOME" << " | " << setw(12) << "KIND" << " | " << setw(18) << "TIPO" << " | " << "ESCOPO" << '\n';
    for (const auto& symbol : flatten()) {
        out << left << setw(18) << symbol.name << " | " << setw(12) << kindToString(symbol.kind) << " | " << setw(18) << symbol.type.toString() << " | " << symbol.scopeLevel << '\n';
    }
}

string TACEmitter::newTemp() {
    return "t" + to_string(++tempCounter_);
}

string TACEmitter::newLabel(const string& prefix) {
    return prefix + to_string(++labelCounter_);
}

void TACEmitter::emit(const string& line) {
    code_.push_back(line);
}

void TACEmitter::emitLabel(const string& label) {
    code_.push_back(label + ":");
}

const vector<string>& TACEmitter::code() const {
    return code_;
}

void TACEmitter::write(const string& filename) const {
    ofstream out(filename);
    if (!out.is_open()) return;
    for (const auto& line : code_) out << line << '\n';
}

string SemanticAnalyzer::lower(const string& text) {
    string out = text;
    transform(out.begin(), out.end(), out.begin(), [](unsigned char c) { return static_cast<char>(tolower(c)); });
    return out;
}

bool SemanticAnalyzer::startsWith(const string& text, const string& prefix) {
    return text.rfind(prefix, 0) == 0;
}

string SemanticAnalyzer::suffixAfterDot(const string& label) {
    size_t pos = label.find('.');
    if (pos == string::npos) return "";
    return label.substr(pos + 1);
}

string SemanticAnalyzer::extractId(ASTNode* node) {
    if (!node) return "";
    if (startsWith(node->label, "ID.")) return suffixAfterDot(node->label);
    return node->label;
}

string SemanticAnalyzer::locationOf(ASTNode* node) const {
    if (!node || node->linha < 0 || node->coluna < 0) return "";
    return " [linha " + to_string(node->linha) + ", coluna " + to_string(node->coluna) + "]";
}

void SemanticAnalyzer::reportError(ASTNode* node, const string& message) {
    string full = "erro semantico" + locationOf(node) + ": " + message;
    errors.push_back(full);
}

void SemanticAnalyzer::reportError(const string& message) {
    errors.push_back(message);
}
TypeInfo SemanticAnalyzer::typeFromName(const string& name) const {
    string normalized = lower(name);
    if (normalized == "int" || normalized == "float" || normalized == "double" || normalized == "bool" || normalized == "char" || normalized == "string" || normalized == "void") {
        return TypeInfo::Named(normalized);
    }
    if (const SymbolInfo* symbol = symbols.lookup(name)) {
        if (symbol->kind == SymbolKind::Typedef) return symbol->type;
    }
    return TypeInfo::Named(normalized);
}

TypeInfo SemanticAnalyzer::typeFromNode(ASTNode* node) const {
    if (!node) return TypeInfo::Error();
    if (startsWith(node->label, "TYPE.")) return typeFromName(suffixAfterDot(node->label));
    return TypeInfo::Error();
}

bool SemanticAnalyzer::compatibleAssignment(const TypeInfo& target, const TypeInfo& value) const {
    if (target.isError() || value.isError()) return true;
    if (target.isArray || value.isArray) return target.isArray == value.isArray && target.name == value.name;
    if (target.name == value.name) return true;
    return target.isNumeric() && value.isNumeric();
}

TypeInfo SemanticAnalyzer::promoteNumeric(const TypeInfo& left, const TypeInfo& right) const {
    if (left.name == "double" || right.name == "double") return TypeInfo::Named("double");
    if (left.name == "float" || right.name == "float") return TypeInfo::Named("float");
    return TypeInfo::Named("int");
}

bool SemanticAnalyzer::comparable(const TypeInfo& left, const TypeInfo& right) const {
    if (left.isError() || right.isError()) return true;
    if (left.name == right.name) return true;
    return left.isNumeric() && right.isNumeric();
}

void SemanticAnalyzer::emitDeclaration(const SymbolInfo& symbol) {
    if (symbol.kind == SymbolKind::Function) {
        emitter.emit("function " + symbol.name + " returns " + symbol.type.toString());
        return;
    }
    if (symbol.type.isArray) {
        emitter.emit(kindToString(symbol.kind) + " " + symbol.name + " : " + symbol.type.name + "[" + (symbol.type.arraySize >= 0 ? to_string(symbol.type.arraySize) : "") + "]");
    } else {
        emitter.emit(kindToString(symbol.kind) + " " + symbol.name + " : " + symbol.type.toString());
    }
}

void SemanticAnalyzer::firstPass(ASTNode* root) {
    if (!root || root->label != "PROGRAM" || root->children.empty()) return;
    collectDeclList(root->children[0]);
}

void SemanticAnalyzer::collectDeclList(ASTNode* node) {
    if (!node || node->label != "DECL_LIST") return;
    for (auto* child : node->children) collectDecl(child);
}

void SemanticAnalyzer::collectDecl(ASTNode* node) {
    if (!node) return;
    if (node->label == "TYPE_DECL") {
        collectTypeDecl(node);
    } else if (node->label == "FUNCTION_DECL") {
        collectFunctionSignature(node);
    }
}

void SemanticAnalyzer::collectTypeDecl(ASTNode* node) {
    if (node->children.size() < 2) return;
    string alias = extractId(node->children[1]);
    SymbolInfo info;
    info.name = alias;
    info.kind = SymbolKind::Typedef;
    info.type = TypeInfo::Named(alias);
    if (!symbols.declare(info)) reportError("erro semantico: tipo '" + alias + "' ja declarado no escopo atual");
}

void SemanticAnalyzer::collectFunctionSignature(ASTNode* node) {
    if (node->children.size() < 3) return;
    string name = extractId(node->children[1]);
    SymbolInfo info;
    info.name = name;
    info.kind = SymbolKind::Function;
    info.type = typeFromNode(node->children[0]);
    if (node->children[2]) {
        for (auto* param : node->children[2]->children) {
            if (!param || param->label != "PARAM" || param->children.size() < 2) continue;
            TypeInfo paramType = typeFromNode(param->children[0]);
            if (param->children[2]) {
                if (param->children[2]->children.empty()) paramType = TypeInfo::ArrayOf(paramType);
                else if (param->children[2]->children[0] && startsWith(param->children[2]->children[0]->label, "NUMBER.")) paramType = TypeInfo::ArrayOf(paramType, stoi(suffixAfterDot(param->children[2]->children[0]->label)));
                else paramType = TypeInfo::ArrayOf(paramType);
            }
            info.parameters.push_back(paramType);
        }
    }
    if (!symbols.declare(info)) reportError("erro semantico: funcao '" + name + "' ja declarada no escopo atual");
}

void SemanticAnalyzer::analyzeProgram(ASTNode* root) {
    if (!root || root->label != "PROGRAM" || root->children.empty()) return;
    analyzeDeclList(root->children[0]);
}

void SemanticAnalyzer::analyzeDeclList(ASTNode* node) {
    if (!node || node->label != "DECL_LIST") return;
    for (auto* child : node->children) analyzeDecl(child);
}

void SemanticAnalyzer::analyzeDecl(ASTNode* node) {
    if (!node) return;
    if (node->label == "TYPE_DECL") analyzeTypeDecl(node);
    else if (node->label == "VAR_DECL_LIST") analyzeVarDeclList(node, true);
    else if (node->label == "FUNCTION_DECL") analyzeFunctionDecl(node);
}

void SemanticAnalyzer::analyzeTypeDecl(ASTNode* node) {
    if (node->children.size() < 2) return;
    string alias = extractId(node->children[1]);
    const SymbolInfo* symbol = symbols.lookup(alias);
    if (symbol) emitDeclaration(*symbol);
}

void SemanticAnalyzer::analyzeVarDeclList(ASTNode* node, bool emitDeclarations) {
    if (!node) return;
    if (node->label == "VAR_LIST") {
        for (auto* child : node->children) {
            analyzeVarDeclList(child, emitDeclarations);
        }
        return;
    }

    if (node->label != "VAR_DECL_LIST" || node->children.empty()) return;

    TypeInfo base = typeFromNode(node->children[0]->children[0]);
    for (auto* entry : node->children) {
        if (!entry || entry->children.size() < 2) continue;
        string name = extractId(entry->children[1]);
        TypeInfo declaredType = base;
        if (entry->children.size() > 2 && entry->children[2]) {
            if (entry->children[2]->children.empty()) {
                declaredType = TypeInfo::ArrayOf(base);
            } else if (entry->children[2]->children[0] && startsWith(entry->children[2]->children[0]->label, "NUMBER.")) {
                declaredType = TypeInfo::ArrayOf(base, stoi(suffixAfterDot(entry->children[2]->children[0]->label)));
            } else {
                declaredType = TypeInfo::ArrayOf(base);
            }
        }
        SymbolInfo symbol;
        symbol.name = name;
        symbol.kind = SymbolKind::Variable;
        symbol.type = declaredType;
        if (!symbols.declare(symbol)) {
            reportError("erro semantico: identificador '" + name + "' ja declarado no escopo atual");
            continue;
        }
        if (emitDeclarations) emitDeclaration(symbol);
    }
}

void SemanticAnalyzer::analyzeFormalList(ASTNode* node, vector<SymbolInfo>& params) {
    if (!node || node->label != "FORMAL_LIST") return;
    for (auto* param : node->children) {
        if (!param || param->label != "PARAM" || param->children.size() < 2) continue;
        SymbolInfo info;
        info.name = extractId(param->children[1]);
        info.kind = SymbolKind::Parameter;
        info.type = typeFromNode(param->children[0]);
        if (param->children[2]) {
            if (param->children[2]->children.empty()) info.type = TypeInfo::ArrayOf(info.type);
            else if (param->children[2]->children[0] && startsWith(param->children[2]->children[0]->label, "NUMBER.")) info.type = TypeInfo::ArrayOf(info.type, stoi(suffixAfterDot(param->children[2]->children[0]->label)));
            else info.type = TypeInfo::ArrayOf(info.type);
        }

        if (info.type.isVoid()) {
            reportError(param, "parametro '" + info.name + "' nao pode ter tipo void");
        }
        params.push_back(info);
    }
}

void SemanticAnalyzer::analyzeFunctionDecl(ASTNode* node) {
    if (node->children.size() < 5) return;
    string name = extractId(node->children[1]);
    SymbolInfo* signature = symbols.lookup(name);
    if (!signature || signature->kind != SymbolKind::Function) {
        reportError("erro semantico: assinatura da funcao '" + name + "' nao encontrada");
        return;
    }

    currentReturnType = signature->type;
    emitDeclaration(*signature);

    symbols.enterScope();
    vector<SymbolInfo> params;
    analyzeFormalList(node->children[2], params);
    for (auto& param : params) {
        if (!symbols.declare(param)) reportError("erro semantico: parametro '" + param.name + "' ja declarado na funcao '" + name + "'");
        else emitDeclaration(param);
    }
    if (node->children[3]) analyzeVarDeclList(node->children[3], true);
    if (node->children[4]) analyzeStmtList(node->children[4]);
    emitter.emit("end function " + name);
    symbols.exitScope();
    currentReturnType = TypeInfo::Named("void");
}

void SemanticAnalyzer::analyzeStmtList(ASTNode* node) {
    if (!node) return;
    if (node->label == "BLOCK") {
        analyzeBlock(node);
        return;
    }
    if (node->label != "STMT_LIST") return;
    for (auto* stmt : node->children) analyzeStmt(stmt);
}

void SemanticAnalyzer::analyzeBlock(ASTNode* node) {
    symbols.enterScope();
    for (auto* child : node->children) analyzeStmtList(child);
    symbols.exitScope();
}

void SemanticAnalyzer::analyzeStmt(ASTNode* node) {
    if (!node) return;
    if (node->label == "IF") {
        if (node->children.size() < 3) return;
        ExprResult cond = analyzeExpr(node->children[0]);
        if (!cond.type.isError() && !cond.type.isBoolean() && !cond.type.isNumeric()) {
            reportError(node->children[0], "condicao do if deve ser booleana ou numerica");
        }
        string elseLabel = emitter.newLabel("else");
        string endLabel = emitter.newLabel("endif");
        emitter.emit("ifFalse " + cond.place + " goto " + elseLabel);
        analyzeStmt(node->children[1]);
        emitter.emit("goto " + endLabel);
        emitter.emitLabel(elseLabel);
        analyzeStmt(node->children[2]);
        emitter.emitLabel(endLabel);
        return;
    }
    if (node->label == "WHILE") {
        if (node->children.size() < 2) return;
        string startLabel = emitter.newLabel("while");
        string endLabel = emitter.newLabel("endwhile");
        emitter.emitLabel(startLabel);
        loopExitLabels.push_back(endLabel);
        ExprResult cond = analyzeExpr(node->children[0]);
        if (!cond.type.isError() && !cond.type.isBoolean() && !cond.type.isNumeric()) {
            reportError(node->children[0], "condicao do while deve ser booleana ou numerica");
        }
        emitter.emit("ifFalse " + cond.place + " goto " + endLabel);
        analyzeStmt(node->children[1]);
        emitter.emit("goto " + startLabel);
        emitter.emitLabel(endLabel);
        loopExitLabels.pop_back();
        return;
    }
    if (node->label == "BREAK") {
        if (!loopExitLabels.empty()) emitter.emit("goto " + loopExitLabels.back());
        else reportError(node, "erro semantico: break fora de laco");
        return;
    }
    if (node->label == "PRINT") {
        if (!node->children.empty() && node->children[0]) {
            for (auto* expr : node->children[0]->children) {
                ExprResult result = analyzeExpr(expr);
                emitter.emit("print " + result.place);
            }
        }
        return;
    }
    if (node->label == "READ") {
        if (!node->children.empty()) {
            ExprResult target = resolveLValue(node->children[0]);
            if (!target.isLValue) reportError(node->children[0], "readln exige uma variavel ou posicao de vetor");
            emitter.emit("read " + target.place);
        }
        return;
    }
    if (node->label == "RETURN") {
        bool hasValue = !node->children.empty() && node->children[0];
        ExprResult value;
        if (hasValue) value = analyzeExpr(node->children[0]);

        if (currentReturnType.isVoid() && hasValue) {
            reportError(node, "funcao void nao deve retornar valor");
        } else if (!currentReturnType.isVoid() && !hasValue) {
            reportError(node, "funcao nao-void deve retornar valor");
        } else if (!currentReturnType.isVoid() && hasValue && !compatibleAssignment(currentReturnType, value.type)) {
            reportError(node, "tipo de retorno incompativel: esperado '" + currentReturnType.toString() + "', recebido '" + value.type.toString() + "'");
        }

        if (hasValue) emitter.emit("return " + value.place);
        else emitter.emit("return");
        return;
    }
    if (node->label == "BLOCK") {
        analyzeBlock(node);
        return;
    }
    analyzeExpr(node);
}

ExprResult SemanticAnalyzer::resolveLValue(ASTNode* node) {
    if (!node) return {};
    if (startsWith(node->label, "ID.")) {
        string name = extractId(node);
        SymbolInfo* symbol = symbols.lookup(name);
        if (!symbol) {
            reportError(node, "identificador '" + name + "' nao declarado");
            return {TypeInfo::Error(), name, false};
        }
        return {symbol->type, name, true};
    }
    if (node->label == "ARRAY") return analyzeArrayAccess(node);
    return analyzeExpr(node);
}

ExprResult SemanticAnalyzer::analyzeArrayAccess(ASTNode* node) {
    if (!node || node->children.size() < 2) return {};
    ExprResult base = resolveLValue(node->children[0]);
    ExprResult index = analyzeExpr(node->children[1]);
    if (!base.type.isArray || !base.type.elementType) {
        reportError("erro semantico: acesso indevido a vetor");
        return {TypeInfo::Error(), base.place + "[" + index.place + "]", false};
    }
    if (!index.type.isError() && index.type.name != "int") reportError(node->children[1], "indice de vetor deve ser inteiro");
    return {*base.type.elementType, base.place + "[" + index.place + "]", true};
}

ExprResult SemanticAnalyzer::analyzeCall(ASTNode* node) {
    if (!node || node->children.empty()) return {};
    string callee = extractId(node->children[0]);
    SymbolInfo* symbol = symbols.lookup(callee);
    if (!symbol || symbol->kind != SymbolKind::Function) {
        reportError("erro semantico: funcao '" + callee + "' nao declarada");
        return {TypeInfo::Error(), callee + "()", false};
    }

    vector<ExprResult> args;
    if (node->children.size() > 1 && node->children[1]) {
        for (auto* expr : node->children[1]->children) {
            args.push_back(analyzeExpr(expr));
        }
    }
    for (const auto& arg : args) emitter.emit("param " + arg.place);
    string temp = emitter.newTemp();
    emitter.emit(temp + " = call " + callee + ", " + to_string(args.size()));
    return {symbol->type, temp, false};
}

ExprResult SemanticAnalyzer::analyzeUnary(ASTNode* node) {
    if (!node || node->children.empty()) return {};
    if (node->label == "NOT") {
        ExprResult value = analyzeExpr(node->children[0]);
        string temp = emitter.newTemp();
        emitter.emit(temp + " = !" + value.place);
        return {TypeInfo::Named("bool"), temp, false};
    }
    if (node->label == "INC" || node->label == "DEC") {
        ExprResult target = resolveLValue(node->children[0]);
        string temp = emitter.newTemp();
        string op = node->label == "INC" ? "+" : "-";
        emitter.emit(temp + " = " + target.place + " " + op + " 1");
        emitter.emit(target.place + " = " + temp);
        return {target.type, temp, false};
    }
    return analyzePrimary(node);
}

ExprResult SemanticAnalyzer::analyzeBinary(ASTNode* node, const string& op) {
    if (!node || node->children.size() < 2) return {};
    ExprResult left = analyzeExpr(node->children[0]);
    ExprResult right = analyzeExpr(node->children[1]);
    string temp = emitter.newTemp();

    if (op == "||" || op == "&&") {
        emitter.emit(temp + " = " + left.place + " " + op + " " + right.place);
        return {TypeInfo::Named("bool"), temp, false};
    }
    if (op == "==" || op == "!=") {
        if (!comparable(left.type, right.type)) reportError("erro semantico: comparacao de igualdade com tipos incompativeis");
        emitter.emit(temp + " = " + left.place + " " + op + " " + right.place);
        return {TypeInfo::Named("bool"), temp, false};
    }
    if (op == "<" || op == "<=" || op == ">" || op == ">=") {
        if (!left.type.isNumeric() || !right.type.isNumeric()) reportError("erro semantico: comparacao relacional exige operandos numericos");
        emitter.emit(temp + " = " + left.place + " " + op + " " + right.place);
        return {TypeInfo::Named("bool"), temp, false};
    }
    if (op == "+" || op == "-" || op == "*" || op == "/" || op == "%") {
        if (!left.type.isNumeric() || !right.type.isNumeric()) reportError("erro semantico: operador aritmetico exige operandos numericos");
        emitter.emit(temp + " = " + left.place + " " + op + " " + right.place);
        return {promoteNumeric(left.type, right.type), temp, false};
    }
    emitter.emit(temp + " = " + left.place + " " + op + " " + right.place);
    return {TypeInfo::Error(), temp, false};
}

ExprResult SemanticAnalyzer::analyzeAssignment(ASTNode* node) {
    if (!node || node->children.size() < 2) return {};
    ExprResult left = resolveLValue(node->children[0]);
    ExprResult right = analyzeExpr(node->children[1]);
    if (!left.isLValue) reportError("erro semantico: lado esquerdo da atribuicao nao e um lvalue");
    else if (!compatibleAssignment(left.type, right.type)) reportError("erro semantico: atribuicao incompativel entre '" + left.type.toString() + "' e '" + right.type.toString() + "'");
    emitter.emit(left.place + " = " + right.place);
    return {left.type, left.place, false};
}

ExprResult SemanticAnalyzer::analyzePrimary(ASTNode* node) {
    if (!node) return {};
    if (startsWith(node->label, "ID.")) {
        string name = extractId(node);
        SymbolInfo* symbol = symbols.lookup(name);
        if (!symbol) {
            reportError(node, "identificador '" + name + "' nao declarado");
            return {TypeInfo::Error(), name, true};
        }
        return {symbol->type, name, true};
    }
    if (startsWith(node->label, "NUMBER.")) {
        string value = suffixAfterDot(node->label);
        return {value.find('.') != string::npos ? TypeInfo::Named("float") : TypeInfo::Named("int"), value, false};
    }
    if (startsWith(node->label, "LITERAL.")) return {TypeInfo::Named("string"), '"' + suffixAfterDot(node->label) + '"', false};
    if (startsWith(node->label, "CHAR.")) return {TypeInfo::Named("char"), '\'' + suffixAfterDot(node->label) + '\'', false};
    if (node->label == "TRUE" || node->label == "FALSE") return {TypeInfo::Named("bool"), lower(node->label), false};
    if (node->label == "CALL") return analyzeCall(node);
    if (node->label == "ARRAY") return analyzeArrayAccess(node);
    if (startsWith(node->label, "BOOLEAN_OP.") || startsWith(node->label, "RELATIONAL_OP.") || startsWith(node->label, "ADDITION_OP.") || startsWith(node->label, "MULTIPLICATION_OP.")) return analyzeBinary(node, suffixAfterDot(node->label));
    if (node->label == "ASSIGN") return analyzeAssignment(node);
    if (node->label == "BLOCK") {
        analyzeBlock(node);
        return {TypeInfo::Named("void"), "", false};
    }
    return {TypeInfo::Error(), node->label, false};
}

ExprResult SemanticAnalyzer::analyzeExpr(ASTNode* node) {
    if (!node) return {};
    if (node->label == "ASSIGN") return analyzeAssignment(node);
    if (node->label == "CALL") return analyzeCall(node);
    if (node->label == "ARRAY") return analyzeArrayAccess(node);
    if (node->label == "NOT" || node->label == "INC" || node->label == "DEC") return analyzeUnary(node);
    if (startsWith(node->label, "BOOLEAN_OP.") || startsWith(node->label, "RELATIONAL_OP.") || startsWith(node->label, "ADDITION_OP.") || startsWith(node->label, "MULTIPLICATION_OP.")) return analyzeBinary(node, suffixAfterDot(node->label));
    return analyzePrimary(node);
}

bool SemanticAnalyzer::analyze(ASTNode* root) {
    errors.clear();
    while (symbols.level() > 0) symbols.exitScope();
    firstPass(root);
    analyzeProgram(root);
    return errors.empty();
}

const vector<string>& SemanticAnalyzer::getErrors() const {
    return errors;
}

void SemanticAnalyzer::writeSymbols(const string& filename) const {
    symbols.print(filename);
}

void SemanticAnalyzer::writeCode(const string& filename) const {
    emitter.write(filename);
}
