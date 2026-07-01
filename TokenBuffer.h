#ifndef TOKENBUFFER_H
#define TOKENBUFFER_H

#include "lexical.h"

class TokenBuffer {
private:
    vector<LexToken> tokens;
    size_t index = 0;

public:
    TokenBuffer(const vector<LexToken>& t) : tokens(t) {}

    LexToken peek() const{
        if (index < tokens.size()) return tokens[index];
        return {"EOF", "", -1, -1};
    }

    LexToken advance() {
        if (index < tokens.size()) return tokens[index++];
        return {"EOF", "", -1, -1};
    }
};

#endif