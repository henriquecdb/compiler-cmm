#ifndef LEXICAL_H
#define LEXICAL_H

#include <bits/stdc++.h>

using namespace std;

struct LexToken {
    string tipo;
    string lexema;
    int linha;
    int coluna;
};

class Lexical {
  public:
    bool loadAfd(const string &jflapPath);
    vector<LexToken> getTokens(const string &inputPath, const vector<string> &reservedKeywords);
    bool tokenize(const string &inputPath, const vector<string> &reservedKeywords, vector<LexToken> &tokens) const;
    bool run(const string &inputPath, const vector<string> &reservedKeywords) const;
    

  private:
    int initialState_ = -1;
    vector<vector<int>> transitionTable_;
    unordered_set<int> finalStates_;
    unordered_set<int> validFinalStates_;
    vector<string> stateNames_;
    

    static string trim(const string &text);
    static string extractTagValue(const string &block, const string &tag);
    static vector<unsigned char> decodeReadSymbol(const string &rawSymbol);
    static string stringToLower(const string &text);
};

#endif