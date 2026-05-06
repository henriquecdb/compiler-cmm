#include "lexical.h"
#include "parser.h"
#include "TokenBuffer.h"
#include "ast.h"
#include <bits/stdc++.h>

using namespace std;

int main(int argc, char *argv[]) {
    vector<string> reservedKeywords = {
        "if",     "else", "while",   "for",  "return", "int",
        "float",  "char", "double",  "void", "break",  "continue",
        "switch", "case", "default", "do",   "struct", "typedef",
        "const",  "true", "false", "readln", "bool", "print",
        "string", "class", "static", "include", "define","using","vector"
    };

    string afd = "testeComp.jff";
    string testFile;

    if (argc > 1) {
        testFile = argv[1];
        cout << testFile << endl;
    } else {
        testFile = "input.txt";
    }

    Lexical lexical;
    if (!lexical.loadAfd(afd)) {
        cerr << "Nao foi possivel carregar o AFD do arquivo " << afd << endl;
        return 1;
    }

    vector<LexToken> tokens = lexical.getTokens(testFile, reservedKeywords);

    TokenBuffer tb(tokens);
    Parser parser(tb);
    ASTNode* root = parser.Program();

    cout << "\nAST" << endl;
    printAST(root);

    string svgFile = "ast.svg";
    writeASTSvg(root, svgFile);

    return 0;
}
