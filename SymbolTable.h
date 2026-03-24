#ifndef SYMBOLTABLE_H
#define SYMBOLTABLE_H

#include <bits/stdc++.h>
#include <fstream>    
#include <string>        
#include <unordered_map>
#include <iomanip>

using namespace std;

struct Token {
    string tipo;
    int linha;
    int coluna;
};

class SymbolTable {
    private:
        unordered_map<string, Token> table; 

    public:
        void insert(const string& lexeme, const string& tipo, int linha, int col) {
            if (table.find(lexeme) == table.end()) {
                table[lexeme] = {tipo, linha, col};
            }
        }

        bool exists(const string& lexeme) {
            return table.find(lexeme) != table.end();
        }

        void print(const string& filename = "tabela_simbolos.txt") {
            ofstream outFile(filename);

            if (!outFile.is_open()) {
                cerr << "Erro ao abrir o arquivo para salvar a tabela!" << endl;
                return;
            }

            outFile << "\n" << string(70, '-') << endl;
            outFile << left << setw(20) << "LEXEMA" 
                << " | " << setw(20) << "TIPO" 
                << " | " << "POSICAO ([Linha][Coluna])" << endl;
            outFile << string(70, '-') << endl;

            unordered_map<string, Token>::iterator it;
            for (it = table.begin(); it != table.end(); ++it) {
                outFile << left << setw(20) << it->first 
                    << " | " << setw(20) << it->second.tipo 
                    << " | " << "[" << it->second.linha << "]" << "[" << it->second.coluna << "]"  << endl;
            }
            outFile << string(70, '-') << endl;

            outFile.close(); 
            cout << "\nTabela de simbolos salva com sucesso em: " << filename << endl;
        }
}; 
#endif