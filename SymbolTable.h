#ifndef SYMBOLTABLE_H
#define SYMBOLTABLE_H

#include <bits/stdc++.h>
#include <iostream>      
#include <string>        
#include <unordered_map>

using namespace std;

class SymbolTable {
    private:
        unordered_map<string, string> table; 

    public:
        // Tenta inserir um ID. Se já existir, ele ignora ou atualiza.
        void insert(const string& lexeme, const string& type = "ID") {
            if (table.find(lexeme) == table.end()) {
                table[lexeme] = type;
            }
        }

        bool exists(const string& lexeme) {
            return table.find(lexeme) != table.end();
        }

        void print() {
            cout << "\n--- TABELA DE SIMBOLOS ---\n";
            for (auto const& entry : table) {
                cout << entry.first << " -> " << entry.second << endl;
            }
        }
};
#endif