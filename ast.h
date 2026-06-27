#ifndef AST_H
#define AST_H

#include <bits/stdc++.h>

using namespace std;

class ASTNode {
public:
    string label;
    int linha = -1;
    int coluna = -1;
    vector<ASTNode*> children;

    ASTNode(const string& label, int linha = -1, int coluna = -1)
        : label(label), linha(linha), coluna(coluna) {}

    void add(ASTNode* child) {
        if (child) children.push_back(child);
    }
};

void printAST(ASTNode* node, int level = 0);
void writeASTSvg(ASTNode* node, const string& filename);

#endif
