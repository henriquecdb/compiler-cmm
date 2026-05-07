#ifndef SYMBOLTABLE_H
#define SYMBOLTABLE_H

#include "token.h" 
#include <bits/stdc++.h>

using namespace std;
class SymbolTable {
private:
    unordered_map<string, shared_ptr<Token>> table; 

public:
    void insert(shared_ptr<Token> novoToken) {
        if (table.find(novoToken->lexema) == table.end()) {
            table[novoToken->lexema] = novoToken;
        }
    }

    bool exists(const string& lexeme) {
        return table.find(lexeme) != table.end();
    }

    void print(const string& filename = "tabela_simbolos.txt") {
        ofstream outFile(filename);
        if (!outFile.is_open()) return;

        outFile << left << setw(20) << "LEXEMA" << " | " << setw(20) << "TIPO" << " | " << "POSIÇÃO" << endl;
        for (auto const& pair : table) {
            auto lexeme = pair.first;
            auto token = pair.second;
            outFile << left << setw(20) << lexeme << " | " << setw(20) << token->tipo << " | " << "[" << token->linha << "][" << token->coluna << "]" << endl;
        }
        outFile.close();
    }
};

#endif