#include "lexical.h"
#include <bits/stdc++.h>

using namespace std;

int main(int argc, char *argv[]) {
    vector<string> reservedKeywords = {
        "if",     "else", "while",   "for",  "return", "int",
        "float",  "char", "double",  "void", "break",  "continue",
        "switch", "case", "default", "do",   "struct", "typedef",
        "const",  "true", "false"
    };

    string afd = "testeComp.jff";;
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

    if (!lexical.run(testFile, reservedKeywords)) {
        cerr << "Nao foi possivel abrir input.txt" << endl;
        return 1;
    }

    return 0;
}
