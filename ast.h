#ifndef AST_H
#define AST_H

#include <bits/stdc++.h>

using namespace std;

class ASTNode {
public:
    string label;
    vector<ASTNode*> children;

    ASTNode(string label) {
        this->label = label;
    }

    void add(ASTNode* child) {
        if (child) children.push_back(child);
    }
};

void printAST(ASTNode* node, int level = 0);

#endif
