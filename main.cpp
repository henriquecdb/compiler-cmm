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

    string afd;
    if (argc > 1) {
        afd = argv[1];
    } else {
        ifstream testComp("testeComp.jff");

        if (testComp.good()) afd = "testeComp.jff";
        else afd = "afd.jff";
    }

    Lexical lexical;
    if (!lexical.loadAfd(afd)) {
        cerr << "Nao foi possivel carregar o AFD do arquivo " << afd << endl;
        return 1;
    }

    if (!lexical.run("input.txt", reservedKeywords)) {
        cerr << "Nao foi possivel abrir input.txt" << endl;
        return 1;
    }

    return 0;
}
