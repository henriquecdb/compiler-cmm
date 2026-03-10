#include <bits/stdc++.h>

using namespace std;

// Classes: 0 = letra, 1 = digito, 2 = outro/terminador
int char_class(int ch) {
    if (ch == EOF) return 2;
    unsigned char uch = static_cast<unsigned char>(ch);
    if (isalpha(uch) || ch == '_') return 0;
    if (isdigit(uch)) return 1;
    return 2;
}

void run_scanner(ifstream &arquivo, const vector<string> &reserved);

int main()
{
    ios::sync_with_stdio(0);
    cin.tie(0);

    vector<string> reservedKeywords = {
        "if", "else", "while", "for", "return", "int", "float", "char",
        "double", "void", "break", "continue", "switch", "case", "default",
        "do", "struct", "typedef", "const", "true", "false"
    };

    sort(reservedKeywords.begin(), reservedKeywords.end());

    ifstream arquivo("input.txt");
    if (!arquivo.is_open()) return 1;

    run_scanner(arquivo, reservedKeywords);

    arquivo.close();
    return 0;
}

void run_scanner(ifstream &arquivo, const vector<string> &reserved) {
    int next_state[4][3] = {
        {1, 3, 3},
        {1, 1, 2},
        {2, 2, 2},
        {3, 3, 3}
    };

    while (true) {
        while (isspace(arquivo.peek())) {
            int tmp = arquivo.get();
            if (tmp == EOF) break;
        }

        int p = arquivo.peek();
        if (p == EOF) break;

        int state = 0;
        string lexeme;
        bool done = false;

        while (!done) {
            int nextChar = arquivo.peek();
            int charCls = char_class(nextChar);
            int nextState = next_state[state][charCls];

            {
                char ch;
                switch (nextState) {
                    case 1:
                        arquivo.get(ch);
                        lexeme.push_back(ch);
                        state = nextState;
                        break;
                    case 2:
                        state = nextState;
                        done = true;
                        break;
                    case 3:
                        arquivo.get(ch);
                        lexeme.push_back(ch);
                        state = nextState;
                        done = true;
                        break;
                    default:
                        done = true;
                        break;
                }
            }

            if (arquivo.eof() && state == 1) {
                state = 2;
                done = true;
            }
        }

        if (state == 2) {
            if (binary_search(reserved.begin(), reserved.end(), lexeme)) {
                cout << "Palavra reservada encontrada: " << lexeme << endl;
            } else {
                cout << "Token ID: " << lexeme << endl;
            }
        } else if (state == 3) {
            cout << "Erro léxico: " << lexeme << endl;
        } else {
            char ch;
            arquivo.get(ch);
            (void)ch;
        }
    }
}
