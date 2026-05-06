#include "ast.h"

struct SvgNodeInfo {
    ASTNode* node;
    double x;
    double y;
    double width;
    double height;
};

void printAST(ASTNode* node, int level) {
    if (!node) return;

    for (int i = 0; i < level; i++) cout << ' ';
    cout << "-" << node->label << endl;

    for (auto child : node->children) {
        printAST(child, level + 1);
    }
}

static void computePositions(ASTNode* node, int depth, map<ASTNode*, SvgNodeInfo>& info, double& nextX, double hSpacing, double vSpacing) {
    if (!node) return;

    if (node->children.empty()) {
        double w = max(60.0, (double)node->label.size() * 7.0 + 10.0);
        info[node] = {node, nextX, depth * vSpacing, w, 24.0};
        nextX += hSpacing;
        return;
    }

    for (auto child : node->children) {
        computePositions(child, depth + 1, info, nextX, hSpacing, vSpacing);
    }

    double minx = 1e18, maxx = -1e18;
    for (auto child : node->children) {
        SvgNodeInfo &ci = info[child];
        minx = min(minx, ci.x);
        maxx = max(maxx, ci.x);
    }
    double cx = (minx + maxx) / 2.0;
    double w = max(60.0, (double)node->label.size() * 7.0 + 10.0);
    info[node] = {node, cx, depth * vSpacing, w, 24.0};
}

void writeASTSvg(ASTNode* root, const string& filename) {
    if (!root) return;

    map<ASTNode*, SvgNodeInfo> info;
    double nextX = 50.0;
    double hSpacing = 140.0;
    double vSpacing = 100.0;

    computePositions(root, 0, info, nextX, hSpacing, vSpacing);

    double minx = 1e18, maxx = -1e18, maxy = 0;
    for (auto &p : info) {
        minx = min(minx, p.second.x - p.second.width/2.0);
        maxx = max(maxx, p.second.x + p.second.width/2.0);
        maxy = max(maxy, p.second.y + p.second.height/2.0);
    }
    double width = max(800.0, maxx - minx + 100.0);
    double height = max(600.0, maxy + 100.0);

    ofstream f(filename);
    if (!f.is_open()) return;

    f << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    f << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"" << (int)width << "\" height=\"" << (int)height << "\">\n";
    f << "<style>text{font-family:Arial;font-size:12px;} .node{fill:#e8f0ff;stroke:#3b6ea5;stroke-width:1.5;} .edge{stroke:#333;stroke-width:1.2;}</style>\n";

    for (auto &p : info) {
        ASTNode* node = p.first;
        SvgNodeInfo ni = p.second;
        for (auto child : node->children) {
            SvgNodeInfo ci = info[child];
            double x1 = ni.x;
            double y1 = ni.y + ni.height/2.0;
            double x2 = ci.x;
            double y2 = ci.y - ci.height/2.0;
            f << "  <line class=\"edge\" x1=\""<<x1<<"\" y1=\""<<y1<<"\" x2=\""<<x2<<"\" y2=\""<<y2<<"\" />\n";
        }
    }

    for (auto &p : info) {
        SvgNodeInfo ni = p.second;
        double x = ni.x - ni.width/2.0;
        double y = ni.y - ni.height/2.0;
        string label = ni.node->label;
        string esc;

        for (char c : label) {
            if (c == '&') esc += "&amp;";
            else if (c == '<') esc += "&lt;";
            else if (c == '>') esc += "&gt;";
            else if (c == '"') esc += "&quot;";
            else if (c == '\'') esc += "&apos;";
            else esc += c;
        }

        f << "  <g class=\"node\">\n";
        f << "    <rect x=\""<<x<<"\" y=\""<<y<<"\" width=\""<<ni.width<<"\" height=\""<<ni.height<<"\" rx=\"6\" ry=\"6\" fill=\"#e8f0ff\" stroke=\"#3b6ea5\"/>\n";
        f << "    <text x=\""<<(ni.x)<<"\" y=\""<<(ni.y+4)<<"\" text-anchor=\"middle\">"<<esc<<"</text>\n";
        f << "  </g>\n";
    }

    f << "</svg>\n";
    f.close();
}
