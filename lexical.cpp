#include "lexical.h"
#include "SymbolTable.h"
#include <bits/stdc++.h>

using namespace std;

string Lexical::stringToLower(const string &text) {
    string out = text;
    transform(out.begin(), out.end(), out.begin(), [](unsigned char c) { return static_cast<char>(tolower(c)); });
    return out;
}

string Lexical::trim(const string &text) {
    size_t start = 0;
    while (start < text.size() && isspace(static_cast<unsigned char>(text[start]))) {
        start++;
    }

    size_t end = text.size();
    while (end > start && isspace(static_cast<unsigned char>(text[end - 1]))) {
        end--;
    }

    return text.substr(start, end - start);
}

string Lexical::extractTagValue(const string &block, const string &tag) {
    string openTag = "<" + tag + ">";
    string closeTag = "</" + tag + ">";

    size_t openPos = block.find(openTag);
    if (openPos == string::npos) return "";

    openPos += openTag.size();
    size_t closePos = block.find(closeTag, openPos);
    if (closePos == string::npos) return "";

    return trim(block.substr(openPos, closePos - openPos));
}

vector<unsigned char> Lexical::decodeReadSymbol(const string &rawSymbol) {
    string normalized = stringToLower(trim(rawSymbol));

    if (normalized == "&gt;") normalized = ">";
    if (normalized == "&lt;") normalized = "<";
    if (normalized == "&amp;") normalized = "&";

    vector<unsigned char> decoded;

    if (normalized.empty() || normalized == "outro") return decoded;

    if (normalized == "l" || normalized == "letter" || normalized == "letra") {
        for (int ch = 'a'; ch <= 'z'; ++ch)
            decoded.push_back(static_cast<unsigned char>(ch));
        for (int ch = 'A'; ch <= 'Z'; ++ch)
            decoded.push_back(static_cast<unsigned char>(ch));
        decoded.push_back(static_cast<unsigned char>('_'));
        return decoded;
    }

    if (normalized == "d" || normalized == "digit" || normalized == "digito") {
        for (int ch = '0'; ch <= '9'; ++ch)
            decoded.push_back(static_cast<unsigned char>(ch));
        return decoded;
    }

    if (normalized == "space") {
        decoded.push_back(static_cast<unsigned char>(' '));
        return decoded;
    }

    if (normalized.size() == 2 && normalized[0] == '\\') {
        switch (normalized[1]) {
        case 'n':
            decoded.push_back(static_cast<unsigned char>('\n'));
            return decoded;
        case 't':
            decoded.push_back(static_cast<unsigned char>('\t'));
            return decoded;
        case 'r':
            decoded.push_back(static_cast<unsigned char>('\r'));
            return decoded;
        case 's':
            decoded.push_back(static_cast<unsigned char>(' '));
            return decoded;
        default:
            break;
        }
    }

    if (normalized.size() == 3 && normalized[1] == '-') {
        unsigned char from = static_cast<unsigned char>(normalized[0]);
        unsigned char to = static_cast<unsigned char>(normalized[2]);
        if (from <= to) {
            for (int ch = from; ch <= to; ++ch) {
                decoded.push_back(static_cast<unsigned char>(ch));
            }
            return decoded;
        }
    }

    if (normalized.size() == 1) {
        decoded.push_back(static_cast<unsigned char>(normalized[0]));
        return decoded;
    }

    for (char ch : normalized) {
        decoded.push_back(static_cast<unsigned char>(ch));
    }

    return decoded;
}

bool Lexical::loadAfd(const string &jflapPath) {
    ifstream jflapFileStream(jflapPath);
    if (!jflapFileStream.is_open()) return false;

    string xml((istreambuf_iterator<char>(jflapFileStream)), istreambuf_iterator<char>());

    unordered_map<int, int> idToIndex;
    vector<int> stateIds;
    vector<bool> isFinal;
    vector<string> stateNames;

    int initialIndex = -1;

    size_t pos = 0;
    while (true) {
        size_t start = xml.find("<state", pos);
        if (start == string::npos) break;

        size_t end = xml.find("</state>", start);
        if (end == string::npos) break;

        string stateBlock = xml.substr(start, end + 8 - start);

        size_t idPos = stateBlock.find("id=\"");
        if (idPos != string::npos) {
            idPos += 4;
            size_t idEnd = stateBlock.find('"', idPos);
            if (idEnd != string::npos) {
                int stateId = stoi(stateBlock.substr(idPos, idEnd - idPos));
                int index = static_cast<int>(stateIds.size());
                stateIds.push_back(stateId);
                idToIndex[stateId] = index;

                size_t namePos = stateBlock.find("name=\"");
                string stateName;
                if (namePos != string::npos) {
                    namePos += 6;
                    size_t nameEnd = stateBlock.find('"', namePos);
                    if (nameEnd != string::npos) {
                        stateName = stateBlock.substr(namePos, nameEnd - namePos);
                    }
                }
                stateNames.push_back(stateName);

                bool finalState = (stateBlock.find("<final/>") != string::npos) || (stateBlock.find("<final />") != string::npos);
                isFinal.push_back(finalState);

                bool initialState = (stateBlock.find("<initial/>") != string::npos) || (stateBlock.find("<initial />") != string::npos);
                
                if (initialState) initialIndex = index;
            }
        }

        pos = end + 8;
    }

    if (stateIds.empty() || initialIndex < 0) return false;

    vector<vector<int>> table(stateIds.size(), vector<int>(256, -1));
    vector<int> outroTarget(stateIds.size(), -1);

    pos = 0;
    while (true) {
        size_t start = xml.find("<transition>", pos);
        if (start == string::npos) break;

        size_t end = xml.find("</transition>", start);
        if (end == string::npos) break;

        string transitionBlock = xml.substr(start, end + 13 - start);
        string fromText = extractTagValue(transitionBlock, "from");
        string toText = extractTagValue(transitionBlock, "to");
        string readText = extractTagValue(transitionBlock, "read");

        if (!fromText.empty() && !toText.empty()) {
            int fromId = stoi(fromText);
            int toId = stoi(toText);

            auto fromIt = idToIndex.find(fromId);
            auto toIt = idToIndex.find(toId);
            if (fromIt != idToIndex.end() && toIt != idToIndex.end()) {
                int fromIndex = fromIt->second;
                int toIndex = toIt->second;
                if (stringToLower(trim(readText)) == "outro") {
                    outroTarget[fromIndex] = toIndex;
                } else {
                    vector<unsigned char> symbols = decodeReadSymbol(readText);
                    for (unsigned char sym : symbols) {
                        table[fromIndex][sym] = toIndex;
                    }
                }
            }
        }

        pos = end + 13;
    }

    for (int state = 0; state < static_cast<int>(table.size()); ++state) {
        if (outroTarget[state] >= 0) {
            for (int ch = 0; ch < 256; ++ch) {
                if (table[state][ch] < 0) {
                    table[state][ch] = outroTarget[state];
                }
            }
        }
    }

    unordered_set<int> finals;
    unordered_set<int> validFinals;
    for (int i = 0; i < static_cast<int>(isFinal.size()); ++i) {
        if (isFinal[i]) {
            finals.insert(i);
            if (stringToLower(stateNames[i]).find("erro") == string::npos) {
                validFinals.insert(i);
            }
        }
    }

    initialState_ = initialIndex;
    transitionTable_ = move(table);
    finalStates_ = move(finals);
    validFinalStates_ = move(validFinals);
    stateNames_ = move(stateNames);
    return true;
}

bool Lexical::accepts(const string &lexeme) const {
    if (initialState_ < 0 || transitionTable_.empty()) {
        return false;
    }

    int state = initialState_;
    for (unsigned char ch : lexeme) {
        int next = transitionTable_[state][ch];
        if (next < 0) {
            return false;
        }
        state = next;
    }

    return finalStates_.find(state) != finalStates_.end();
}

bool Lexical::run(const string &inputPath, const vector<string> &reservedKeywords) const {
    SymbolTable sb;
    ifstream inputFile(inputPath);
    if (!inputFile.is_open()) return false;

    string input((istreambuf_iterator<char>(inputFile)),
                 istreambuf_iterator<char>());

    unordered_set<string> reservedSet;
    for (const string &word : reservedKeywords) {
        reservedSet.insert(stringToLower(word));
    }

    size_t i = 0;

    while (i < input.size()) {
        unsigned char current = input[i];

        if (current == '/' && i + 1 < input.size() && input[i + 1] == '*') {
            i += 2;
            while (i + 1 < input.size() && !(input[i] == '*' && input[i + 1] == '/')) {
                i++;
            }
            if (i + 1 < input.size()) i += 2;
            else cout << "Erro lexico: comentario nao fechado\n";
            continue;
        }

        if (isspace(current)) {
            i++;
            continue;
        }

        if (isalpha(current) || current == '_') {
            size_t j = i;
            bool erro = false;

            while (j < input.size()) {
                unsigned char c = input[j];

                if (isalnum(c) || c == '_') {
                    j++;
                } else{
                    break;
                } 
            }

            string lexeme = input.substr(i, j - i);
            string lowerLexeme = stringToLower(lexeme);

            if (erro) {
                cout << "Erro lexico:" << lexeme << '\n';
            } 
            else if (reservedSet.count(lowerLexeme)) {
                transform(lowerLexeme.begin(), lowerLexeme.end(), lowerLexeme.begin(), ::toupper);
                cout << lowerLexeme << '\n';
            } 
            else {
                sb.insert(lexeme, "ID");
                cout << "ID." << lexeme << '\n';
            }

            i = j;
            continue;
        }

        if (isdigit(current)) {
            size_t j = i;
            bool hasDot = false;
            bool erro = false;

            while (j < input.size()) {
                unsigned char c = input[j];

                if (isdigit(c)) j++;
                else if (c == '.') {
                    if (hasDot) erro = true;
                    hasDot = true;
                    j++;
                } else break;
            }

            string lexeme = input.substr(i, j - i);

            if (erro || lexeme.back() == '.') {
                cout << "Erro lexico:" << lexeme << '\n';
            } else {
                cout << "NUM." << lexeme << '\n';
            }

            i = j;
            continue;
        }

        int state = initialState_;
        int lastFinalState = -1;
        size_t lastFinalPos = i;
        size_t j = i;

        while (j < input.size()) {
            unsigned char ch = input[j];
            int next = transitionTable_[state][ch];
            if (next < 0) break;

            state = next;
            j++;

            if (validFinalStates_.count(state)) {
                lastFinalState = state;
                lastFinalPos = j;
            }
        }
        //cout << lastFinalState <<endl;
        if (lastFinalState >= 0 && lastFinalPos > i) {
            string lexeme = input.substr(i, lastFinalPos - i);
            if (stateNames_[lastFinalState] == "EHD"){ //funcao do * (pega -1 caractere para formar o token)
                lexeme = lexeme.substr(0, lexeme.size() - 1);
                lastFinalPos--; 
            }
            if (lexeme == "(") cout << "LPARENT\n";
            else if (lexeme == ")") cout << "RPARENT\n";
            else if (lexeme == "{") cout << "LBRACE\n";
            else if (lexeme == "}") cout << "RBRACE\n";
            else if (lexeme == "[") cout << "LBRACKET\n";
            else if (lexeme == "]") cout << "RBRACKET\n";
            else if (lexeme == ",") cout << "COMMA\n";
            else if (lexeme == ";") cout << "SEMICOLON\n";
            else if (lexeme == "+") cout << "PLUS\n"; 
            else if (lexeme == "++") cout << "INC\n"; 
            else if (lexeme == "-") cout << "MINUS\n";
            else if (lexeme == "--") cout << "DEC\n";
            else if (lexeme == "*") cout << "MULT\n"; 
            else if (lexeme == "/") cout << "DIV\n";
            else if (lexeme == "%") cout << "MOD\n";
            else if (lexeme == "=") cout << "ASSING\n";
            else if (lexeme == "<") cout << "LT\n";
            else if (lexeme == ">") cout << "GT\n"; 
            else if (lexeme == "<=") cout << "LEQ\n";
            else if (lexeme == ">=") cout << "GEQ\n";
            else if (lexeme == "==") cout << "EQ\n";
            else if (lexeme == "!=") cout << "NEQ\n";
            else if (lexeme == "!") cout << "NEG\n";
            else if (lexeme == "&&") cout << "AND\n";
            else if (lexeme == "||") cout << "OR\n";
            else cout << "Erro lexico: " << lexeme << '\n';

            i = lastFinalPos;
            continue;
        }
        cout << "Erro lexico: " << current << '\n';
        i++;
    }

    cout << "EOF\n";

    sb.print();
    return true;
}