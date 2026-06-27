#include "lexical.h"
#include "parser.h"
#include "TokenBuffer.h"
#include "ast.h"
#include "semantic.h"
#include <bits/stdc++.h>

using namespace std;

int runSim(int argc, char* argv[]);

int main(int argc, char *argv[]) {
    vector<string> reservedKeywords = {
        "if",     "else", "while",   "for",  "return", "int",
        "float",  "char", "double",  "void", "break",  "continue",
        "switch", "case", "default", "do",   "struct", "typedef",
        "const",  "true", "false", "readln", "bool", "print",
        "string", "class", "static", "include", "define","using"
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

    SemanticAnalyzer semantic;
    bool semanticOk = semantic.analyze(root);
    semantic.writeSymbols("semantic_symbols.txt");
    semantic.writeCode("3_code.txt");

    char *simArgv[] = {
        argv[0],
        const_cast<char*>("3_code.txt")
    };

    runSim(2, simArgv);

    if (!semanticOk) {
        cerr << "analise semantica concluida com erros, consulte semantic_symbols.txt e 3_code.txt." << endl;
        for (const auto& error : semantic.getErrors()) {
            cerr << error << endl;
        }
    }

    return 0;
}
