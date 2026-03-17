#include "lexical.h"
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
    ifstream inputFile(inputPath);
    if (!inputFile.is_open()) {
        return false;
    }

    string input((istreambuf_iterator<char>(inputFile)),
                 istreambuf_iterator<char>());

    unordered_set<string> reservedSet;
    for (const string &word : reservedKeywords) {
        reservedSet.insert(stringToLower(word));
    }

    string lastTokenType;
    size_t i = 0;
    while (i < input.size()) {
        unsigned char current = static_cast<unsigned char>(input[i]);
        if (isspace(current)) {
            ++i;
            continue;
        }

        int state = initialState_;
        int lastFinalState = -1;
        size_t lastFinalPos = i;
        size_t j = i;

        while (j < input.size()) {
            unsigned char ch = static_cast<unsigned char>(input[j]);
            int next = transitionTable_[state][ch];
            if (next < 0) {
                break;
            }

            state = next;
            ++j;
            if (validFinalStates_.find(state) != validFinalStates_.end()) {
                lastFinalState = state;
                lastFinalPos = j;
            }
        }

        if (lastFinalState < 0 || lastFinalPos == i) {
            unsigned char first = static_cast<unsigned char>(input[i]);
            if (isalpha(first) || first == '_') {
                size_t j2 = i;
                while (j2 < input.size()) {
                    unsigned char c2 = static_cast<unsigned char>(input[j2]);
                    if (!(isalnum(c2) || c2 == '_')) {
                        break;
                    }
                    ++j2;
                }

                string lexeme = input.substr(i, j2 - i);
                string lowerLexeme = stringToLower(lexeme);
                if (reservedSet.find(lowerLexeme) != reservedSet.end()) {
                    string keyword = lowerLexeme;
                    transform(keyword.begin(), keyword.end(), keyword.begin(),
                              [](unsigned char c) { return static_cast<char>(toupper(c)); });
                    cout << keyword << '\n';
                    lastTokenType = keyword;
                } else {
                    cout << "ID." << lexeme << '\n';
                    lastTokenType = "ID";
                }
                i = j2;
                continue;
            }

            if (isdigit(first)) {
                size_t j2 = i;
                bool hasDot = false;
                while (j2 < input.size()) {
                    unsigned char c2 = static_cast<unsigned char>(input[j2]);
                    if (isdigit(c2)) {
                        ++j2;
                        continue;
                    }
                    if (c2 == '.' && !hasDot) {
                        hasDot = true;
                        ++j2;
                        continue;
                    }
                    break;
                }

                string lexeme = input.substr(i, j2 - i);
                if (hasDot) {
                    cout << "NUM." << lexeme << '\n';
                    lastTokenType = "NUM";
                } else {
                    if (lastTokenType == "ASSING") {
                        cout << "NUMINT." << lexeme << '\n';
                        lastTokenType = "NUMINT";
                    } else {
                        cout << "NUM." << lexeme << '\n';
                        lastTokenType = "NUM";
                    }
                }
                i = j2;
                continue;
            }

            string two;
            if (i + 1 < input.size()) {
                two = input.substr(i, 2);
            }

            if (two == "<=") {
                cout << "LEQ" << '\n';
                lastTokenType = "LEQ";
                i += 2;
                continue;
            }
            if (two == ">=") {
                cout << "GEQ" << '\n';
                lastTokenType = "GEQ";
                i += 2;
                continue;
            }
            if (two == "==") {
                cout << "EQ" << '\n';
                lastTokenType = "EQ";
                i += 2;
                continue;
            }
            if (two == "!=") {
                cout << "NEQ" << '\n';
                lastTokenType = "NEQ";
                i += 2;
                continue;
            }
            if (two == "&&") {
                cout << "AND" << '\n';
                lastTokenType = "AND";
                i += 2;
                continue;
            }
            if (two == "||") {
                cout << "OR" << '\n';
                lastTokenType = "OR";
                i += 2;
                continue;
            }

            char single = input[i];
            if (single == '(') {
                cout << "LPARENT" << '\n';
                lastTokenType = "LPARENT";
                ++i;
                continue;
            }
            if (single == ')') {
                cout << "RPARENT" << '\n';
                lastTokenType = "RPARENT";
                ++i;
                continue;
            }
            if (single == ';') {
                cout << "SEMICOLON" << '\n';
                lastTokenType = "SEMICOLON";
                ++i;
                continue;
            }
            if (single == '=') {
                cout << "ASSING" << '\n';
                lastTokenType = "ASSING";
                ++i;
                continue;
            }

            cout << "Erro léxico: " << input[i] << '\n';
            ++i;
            continue;
        }

        string lexeme = trim(input.substr(i, lastFinalPos - i));
        if (lexeme.empty()) {
            i = lastFinalPos;
            continue;
        }
        string lowerLexeme = stringToLower(lexeme);

        if (reservedSet.find(lowerLexeme) != reservedSet.end()) {
            string keyword = lowerLexeme;
            transform(
                keyword.begin(), keyword.end(), keyword.begin(),
                [](unsigned char c) { return static_cast<char>(toupper(c)); });
            cout << keyword << '\n';
            lastTokenType = keyword;
            i = lastFinalPos;
            continue;
        }

        string tokenType;
        if (lexeme == "(")
            tokenType = "LPARENT";
        else if (lexeme == ")")
            tokenType = "RPARENT";
        else if (lexeme == "{")
            tokenType = "LBRACE";
        else if (lexeme == "}")
            tokenType = "RBRACE";
        else if (lexeme == "[")
            tokenType = "LBRACKET";
        else if (lexeme == "]")
            tokenType = "RBRACKET";
        else if (lexeme == ";")
            tokenType = "SEMICOLON";
        else if (lexeme == "+")
            tokenType = "PLUS";
        else if (lexeme == "++")
            tokenType = "INC";
        else if (lexeme == "-")
            tokenType = "MINUS";
        else if (lexeme == "--")
            tokenType = "DEC";
        else if (lexeme == "*")
            tokenType = "MULT";
        else if (lexeme == "/")
            tokenType = "DIV";
        else if (lexeme == "%")
            tokenType = "MOD";
        else if (lexeme == "=")
            tokenType = "ASSING";
        else if (lexeme == "==")
            tokenType = "EQ";
        else if (lexeme == "!=")
            tokenType = "NEQ";
        else if (lexeme == "<")
            tokenType = "LT";
        else if (lexeme == "<=")
            tokenType = "LEQ";
        else if (lexeme == ">")
            tokenType = "GT";
        else if (lexeme == ">=")
            tokenType = "GEQ";
        else if (lexeme == "&&")
            tokenType = "AND";
        else if (lexeme == "||")
            tokenType = "OR";
        else if (!lexeme.empty() &&
                 (isalpha(static_cast<unsigned char>(lexeme[0])) || lexeme[0] == '_')) {
            bool isIdentifier = true;
            for (char ch : lexeme) {
                unsigned char uch = static_cast<unsigned char>(ch);
                if (!(isalnum(uch) || ch == '_')) {
                    isIdentifier = false;
                    break;
                }
            }

            if (isIdentifier) {
                tokenType = "ID." + lexeme;
            }
        } else if (!lexeme.empty() && isdigit(static_cast<unsigned char>(lexeme[0]))) {
            bool isNumeric = true;
            bool hasDot = false;
            for (char ch : lexeme) {
                if (ch == '.' && !hasDot) {
                    hasDot = true;
                    continue;
                }
                if (!isdigit(static_cast<unsigned char>(ch))) {
                    isNumeric = false;
                    break;
                }
            }

            if (isNumeric) {
                if (hasDot) {
                    tokenType = "NUM." + lexeme;
                } else if (lastTokenType == "ASSING") {
                    tokenType = "NUMINT." + lexeme;
                } else {
                    tokenType = "NUM." + lexeme;
                }
            }
        }

        if (tokenType.empty()) {
            string stateName = stringToLower(stateNames_[lastFinalState]);
            if (stateName == "real" || lexeme.find('.') != string::npos) {
                tokenType = "NUM." + lexeme;
            } else if (stateName == "int") {
                if (lastTokenType == "ASSING")
                    tokenType = "NUMINT." + lexeme;
                else
                    tokenType = "NUM." + lexeme;
            } else {
                tokenType = "ID." + lexeme;
            }
        }

        cout << tokenType << '\n';
        lastTokenType = tokenType;
        i = lastFinalPos;
    }

    cout << "EOF" << '\n';
    return true;
}
