#include "parser.h"

Parser::Parser(TokenBuffer& ts) : ts(ts) {
    advance();
}

void Parser::advance() {
    currentToken = ts.advance();
}

bool Parser::check(const string& tipo) {
    return currentToken.tipo == tipo;
}

bool Parser::isTypeToken(const string& tipo) {
    return tipo == "INT" || tipo == "FLOAT" || tipo == "BOOL" || tipo == "CHAR" || tipo == "VOID" || tipo == "DOUBLE" || tipo == "STRING";
}

bool Parser::isStmtStart(const string& tipo) {
    return tipo == "IF" || tipo == "WHILE" || tipo == "BREAK" || tipo == "PRINT" || tipo == "READLN" || tipo == "RETURN" || tipo == "LBRACE" || isExprStart(tipo);
}

bool Parser::isExprStart(const string& tipo) {
    return tipo == "ID" || tipo == "NUM" || tipo == "LIT_S" || tipo == "LIT_C" || tipo == "LPARENT" || tipo == "TRUE" || tipo == "FALSE" || tipo == "NEG" || tipo == "INC" || tipo == "DEC";
}

void Parser::printNonTerminal(const string& name) {
    cout << name << endl;
}

void Parser::syntaxError(const string& msg) {
    cerr << "programa."<< currentToken.linha << ":"<< currentToken.coluna << " erro sintático. " << msg << endl;
}

bool Parser::match(const string& expected) {
    if (currentToken.tipo == expected) {
        cout << "Match: " << currentToken.tipo;
        if (!currentToken.lexema.empty()) cout << "." << currentToken.lexema;
        cout << endl;
        advance();
        return true;
    }

    syntaxError("esperado " + expected + ", encontrado " + currentToken.tipo);

    if (expected == "SEMICOLON" || expected == "RPARENT" || expected == "RBRACE" || expected == "RBRACKET") {
        return false;
    }

    if (currentToken.tipo != "EOF") advance();
    return false;
}

void Parser::synchronize(const set<string>& sync) {
    while (currentToken.tipo != "EOF" && sync.find(currentToken.tipo) == sync.end()) {
        advance();
    }
}

ASTNode* Parser::Program() {
    printNonTerminal("Program");
    ASTNode* node = new ASTNode("PROGRAM");
    node->add(DeclList());
    match("EOF");
    return node;
}

ASTNode* Parser::DeclList() {
    printNonTerminal("DeclList");
    ASTNode* node = new ASTNode("DECL_LIST");
    while (isTypeToken(currentToken.tipo) || check("TYPEDEF")) {
        node->add(Decl());
    }
    return node;
}

ASTNode* Parser::Decl() {
    printNonTerminal("Decl");

    if (check("TYPEDEF")) {
        return TypeDecl();
    }

    ASTNode* typeNode = Type();
    string id = currentToken.lexema;
    match("ID");

    if (check("LPARENT")) {
        return FunctionDeclAfterType(typeNode, id);
    }

    return VarDeclAfterType(typeNode, id);
}

ASTNode* Parser::TypeDecl() {
    printNonTerminal("TypeDecl");
    ASTNode* node = new ASTNode("TYPE_DECL");

    match("TYPEDEF");
    match("STRUCT");
    match("LBRACE");
    node->add(VarDeclList());
    match("RBRACE");

    string id = currentToken.lexema;
    match("ID");
    node->add(new ASTNode("ID." + id));
    match("SEMICOLON");

    return node;
}

ASTNode* Parser::Type() {
    printNonTerminal("Type");
    string tipo = currentToken.tipo;
    string lexema = currentToken.lexema;

    if (isTypeToken(tipo)) {
        match(tipo);
        if (lexema.empty()) lexema = tipo;
        return new ASTNode("TYPE." + lexema);
    }

    syntaxError("tipo esperado");
    advance();
    return new ASTNode("TYPE.ERROR");
}

ASTNode* Parser::VarDeclList() {
    printNonTerminal("VarDeclList");
    ASTNode* node = new ASTNode("VAR_LIST");

    while (isTypeToken(currentToken.tipo)) {
        node->add(VarDecl());
    }

    return node;
}

ASTNode* Parser::VarDecl() {
    printNonTerminal("VarDecl");
    ASTNode* typeNode = Type();
    string id = currentToken.lexema;
    match("ID");
    return VarDeclAfterType(typeNode, id);
}

ASTNode* Parser::VarDeclAfterType(ASTNode* typeNode, const string& firstId) {
    printNonTerminal("VarDeclAfterType");
    ASTNode* node = new ASTNode("VAR_DECL_LIST");

    ASTNode* first = new ASTNode("NAME_DECL");
    first->add(typeNode);
    first->add(new ASTNode("ID." + firstId));
    first->add(ArrayOpt());
    node->add(first);

    while (check("COMMA")) {
        match("COMMA");
        string id = currentToken.lexema;
        match("ID");

        ASTNode* item = new ASTNode("NAME_DECL");
        item->add(new ASTNode(typeNode ? typeNode->label : "TYPE.ERROR"));
        item->add(new ASTNode("ID." + id));
        item->add(ArrayOpt());
        node->add(item);
    }

    match("SEMICOLON");
    return node;
}

ASTNode* Parser::ArrayOpt() {
    if (!check("LBRACKET")) return nullptr;

    printNonTerminal("Array");

    ASTNode* node = new ASTNode("ARRAY");
    match("LBRACKET");

    if (check("NUM")) {
        string size = currentToken.lexema;
        match("NUM");
        node->add(new ASTNode("NUMBER." + size));
    } else if (isExprStart(currentToken.tipo)) {
        node->add(Expr());
    }

    match("RBRACKET");
    return node;
}

ASTNode* Parser::FunctionDeclAfterType(ASTNode* typeNode, const string& id) {
    printNonTerminal("FunctionDecl");
    ASTNode* node = new ASTNode("FUNCTION_DECL");
    node->add(typeNode);
    node->add(new ASTNode("ID." + id));

    match("LPARENT");
    node->add(FormalList());
    match("RPARENT");
    match("LBRACE");
    node->add(VarDeclList());
    node->add(StmtList());
    match("RBRACE");

    return node;
}

ASTNode* Parser::FormalList() {
    printNonTerminal("FormalList");
    ASTNode* node = new ASTNode("FORMAL_LIST");

    if (isTypeToken(currentToken.tipo)) {
        node->add(Param());
        FormalListRest(node);
    }

    return node;
}

ASTNode* Parser::FormalListRest(ASTNode* list) {
    printNonTerminal("FormalListRest");
    while (check("COMMA")) {
        match("COMMA");
        list->add(Param());
    }
    return list;
}

ASTNode* Parser::Param() {
    printNonTerminal("Param");
    ASTNode* node = new ASTNode("PARAM");
    node->add(Type());
    string id = currentToken.lexema;
    match("ID");
    node->add(new ASTNode("ID." + id));
    node->add(ArrayOpt());
    return node;
}

ASTNode* Parser::StmtList() {
    printNonTerminal("StmtList");
    ASTNode* node = new ASTNode("STMT_LIST");

    while (isStmtStart(currentToken.tipo) && !check("EOF") && !check("RBRACE")) {
        node->add(Stmt());
    }

    return node;
}

ASTNode* Parser::Stmt() {
    printNonTerminal("Stmt");

    if (check("IF")) {
        match("IF");
        match("LPARENT");
        ASTNode* cond = Expr();
        match("RPARENT");
        ASTNode* thenStmt = Stmt();
        match("ELSE");
        ASTNode* elseStmt = Stmt();

        ASTNode* node = new ASTNode("IF");
        node->add(cond);
        node->add(thenStmt);
        node->add(elseStmt);
        return node;
    }

    if (check("WHILE")) {
        match("WHILE");
        match("LPARENT");
        ASTNode* cond = Expr();
        match("RPARENT");
        ASTNode* body = Stmt();

        ASTNode* node = new ASTNode("WHILE");
        node->add(cond);
        node->add(body);
        return node;
    }

    if (check("BREAK")) {
        match("BREAK");
        match("SEMICOLON");
        return new ASTNode("BREAK");
    }

    if (check("PRINT")) {
        match("PRINT");
        match("LPARENT");
        ASTNode* args = ExprList();
        match("RPARENT");
        match("SEMICOLON");

        ASTNode* node = new ASTNode("PRINT");
        node->add(args);
        return node;
    }

    if (check("READLN")) {
        match("READLN");
        match("LPARENT");
        ASTNode* expr = Expr();
        match("RPARENT");
        match("SEMICOLON");

        ASTNode* node = new ASTNode("READ");
        node->add(expr);
        return node;
    }

    if (check("RETURN")) {
        match("RETURN");
        ASTNode* expr = nullptr;
        if (!check("SEMICOLON")) expr = Expr();
        match("SEMICOLON");

        ASTNode* node = new ASTNode("RETURN");
        node->add(expr);
        return node;
    }

    if (check("LBRACE")) {
        match("LBRACE");
        ASTNode* block = StmtList();
        match("RBRACE");
        return block;
    }

    if (isExprStart(currentToken.tipo)) {
        ASTNode* expr = Expr();
        match("SEMICOLON");
        return expr;
    }

    syntaxError("inicio de comando invalido");
    synchronize({"SEMICOLON", "RBRACE", "EOF"});
    if (check("SEMICOLON")) match("SEMICOLON");
    return new ASTNode("ERROR_STMT");
}

ASTNode* Parser::ExprList() {
    printNonTerminal("ExprList");
    ASTNode* node = new ASTNode("EXP_LIST");

    if (isExprStart(currentToken.tipo)) {
        node->add(Expr());
        while (check("COMMA")) {
            match("COMMA");
            node->add(Expr());
        }
    }

    return node;
}

ASTNode* Parser::Expr() {
    printNonTerminal("Expr");
    return Igual();
}

ASTNode* Parser::Igual() {
    printNonTerminal("Igual");
    ASTNode* left = OuExpr();

    if (check("ASSIGN")) {
        match("ASSIGN");
        ASTNode* right = Igual();

        ASTNode* node = new ASTNode("ASSIGN");
        node->add(left);
        node->add(right);
        return node;
    }

    return left;
}

ASTNode* Parser::OuExpr() {
    printNonTerminal("OuExpr");
    ASTNode* left = EExpr();

    while (check("OR")) {
        match("OR");
        ASTNode* right = EExpr();
        ASTNode* node = new ASTNode("BOOLEAN_OP.OR");
        node->add(left);
        node->add(right);
        left = node;
    }

    return left;
}

ASTNode* Parser::EExpr() {
    printNonTerminal("EExpr");
    ASTNode* left = EqExpr();

    while (check("AND")) {
        match("AND");
        ASTNode* right = EqExpr();
        ASTNode* node = new ASTNode("BOOLEAN_OP.AND");
        node->add(left);
        node->add(right);
        left = node;
    }

    return left;
}

ASTNode* Parser::EqExpr() {
    printNonTerminal("EqExpr");
    ASTNode* left = RelExpr();

    while (check("EQ") || check("NEQ")) {
        string op = currentToken.tipo;
        string lex = currentToken.lexema;
        match(op);
        ASTNode* right = RelExpr();

        ASTNode* node = new ASTNode("RELATIONAL_OP." + lex);
        node->add(left);
        node->add(right);
        left = node;
    }

    return left;
}

ASTNode* Parser::RelExpr() {
    printNonTerminal("RelExpr");
    ASTNode* left = SomExpr();

    while (check("LT") || check("GT") || check("LEQ") || check("GEQ")) {
        string op = currentToken.tipo;
        string lex = currentToken.lexema;
        match(op);
        ASTNode* right = SomExpr();

        ASTNode* node = new ASTNode("RELATIONAL_OP." + lex);
        node->add(left);
        node->add(right);
        left = node;
    }

    return left;
}

ASTNode* Parser::SomExpr() {
    printNonTerminal("SomExpr");
    ASTNode* left = MultExpr();

    while (check("PLUS") || check("MINUS")) {
        string op = currentToken.tipo;
        string lex = currentToken.lexema;
        match(op);
        ASTNode* right = MultExpr();

        ASTNode* node = new ASTNode(op == "PLUS" ? "ADDITION_OP.+" : "ADDITION_OP.-");
        node->add(left);
        node->add(right);
        left = node;
    }

    return left;
}

ASTNode* Parser::MultExpr() {
    printNonTerminal("MultExpr");
    ASTNode* left = UnaryExpr();

    while (check("MULT") || check("DIV") || check("MOD")) {
        string op = currentToken.tipo;
        string lex = currentToken.lexema;
        match(op);
        ASTNode* right = UnaryExpr();

        ASTNode* node = new ASTNode("MULTIPLICATION_OP." + lex);
        node->add(left);
        node->add(right);
        left = node;
    }

    return left;
}

ASTNode* Parser::UnaryExpr() {
    printNonTerminal("UnaryExpr");

    if (check("NEG") || check("INC") || check("DEC")) {
        string op = currentToken.tipo;
        string lex = currentToken.lexema;
        match(op);

        string label = "NOT";
        if (op == "INC") label = "INC";
        if (op == "DEC") label = "DEC";

        ASTNode* node = new ASTNode(label + (lex.empty() ? "" : "." + lex));
        node->add(UnaryExpr());
        return node;
    }

    return Primary();
}

ASTNode* Parser::Primary() {
    printNonTerminal("Primary");

    if (check("ID")) {
        string name = currentToken.lexema;
        match("ID");
        return PrimaryRest(new ASTNode("ID." + name));
    }

    if (check("NUM")) {
        string val = currentToken.lexema;
        match("NUM");
        return new ASTNode("NUMBER." + val);
    }

    if (check("LIT_S")) {
        string val = currentToken.lexema;
        match("LIT_S");
        return new ASTNode("LITERAL." + val);
    }

    if (check("LIT_C")) {
        string val = currentToken.lexema;
        match("LIT_C");
        return new ASTNode("CHAR." + val);
    }

    if (check("TRUE")) {
        match("TRUE");
        return new ASTNode("TRUE");
    }

    if (check("FALSE")) {
        match("FALSE");
        return new ASTNode("FALSE");
    }

    if (check("LPARENT")) {
        match("LPARENT");
        ASTNode* node = Expr();
        match("RPARENT");
        return node;
    }

    syntaxError("expressao primaria esperada");
    synchronize({"SEMICOLON", "COMMA", "RPARENT", "RBRACKET", "RBRACE", "EOF"});
    return new ASTNode("ERROR_EXP");
}

ASTNode* Parser::PrimaryRest(ASTNode* base) {
    printNonTerminal("PrimaryRest");

    while (check("LPARENT") || check("LBRACKET")) {
        if (check("LPARENT")) {
            match("LPARENT");
            ASTNode* node = new ASTNode("CALL");
            node->add(base);
            node->add(ExprList());
            match("RPARENT");
            base = node;
        } else if (check("LBRACKET")) {
            match("LBRACKET");
            ASTNode* index = Expr();
            match("RBRACKET");

            ASTNode* node = new ASTNode("ARRAY");
            node->add(base);
            node->add(index);
            base = node;
        }
    }

    return base;
}