#ifndef TOKEN_H
#define TOKEN_H

#include <bits/stdc++.h>

using namespace std;

class Token {
public:
    string lexema;
    string tipo;
    int linha;
    int coluna;

    Token(string lex, string t, int l, int c) : lexema(lex), tipo(t), linha(l), coluna(c) {}
    virtual ~Token() = default;
};

class TokenId : public Token {
public:
    TokenId(string lex, int l, int c) : Token(lex, "ID", l, c) {}
};

class TokenKeyword : public Token {
public:
    TokenKeyword(string lex, string t, int l, int c) : Token(lex, t, l, c) {}
};

class TokenNum : public Token {
public:
    TokenNum(string lex, int l, int c) : Token(lex, "NUM", l, c) {}
};

class TokenPontuacoes : public Token {
public:
    TokenPontuacoes(string lex, string t, int l, int c) : Token(lex, t, l, c) {}
};

class TokenLiteral : public Token {
public:
    TokenLiteral(string lex, string t, int l, int c) : Token(lex, t, l, c) {}
};

#endif