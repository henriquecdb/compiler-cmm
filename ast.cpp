#include "ast.h"

void printAST(ASTNode* node, int level) {
    if (!node) return;

    for (int i = 0; i < level; i++) cout << ' ';
    cout << "-" << node->label << endl;

    for (auto child : node->children) {
        printAST(child, level + 1);
    }
}