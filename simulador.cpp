#include "simulador.h"
#include <bits/stdc++.h>

using namespace std;

static string trim(const string& s) {
    size_t a = 0, b = s.size();
    while (a < b && isspace(static_cast<unsigned char>(s[a]))) a++;
    while (b > a && isspace(static_cast<unsigned char>(s[b - 1]))) b--;
    return s.substr(a, b - a);
}

static bool startsWith(const string& s, const string& prefix) {
    return s.rfind(prefix, 0) == 0;
}

static bool endsWith(const string& s, const string& suffix) {
    if (suffix.size() > s.size()) return false;
    return equal(suffix.rbegin(), suffix.rend(), s.rbegin());
}

static vector<string> splitWords(const string& s) {
    vector<string> out;
    string w;
    stringstream ss(s);
    while (ss >> w) out.push_back(w);
    return out;
}

static string unquoteString(const string& token) {
    if (token.size() < 2) return token;
    char q = token.front();
    if (!((q == '"' && token.back() == '"') || (q == '\'' && token.back() == '\''))) return token;

    string body = token.substr(1, token.size() - 2);
    string out;
    for (size_t i = 0; i < body.size(); ++i) {
        if (body[i] == '\\' && i + 1 < body.size()) {
            char n = body[++i];
            switch (n) {
                case 'n':  out.push_back('\n'); break;
                case 't':  out.push_back('\t'); break;
                case 'r':  out.push_back('\r'); break;
                case '\\': out.push_back('\\'); break;
                case '\'': out.push_back('\''); break;
                case '"':  out.push_back('"');  break;
                default:   out.push_back(n);    break;
            }
        } else {
            out.push_back(body[i]);
        }
    }
    return out;
}

Value Value::Number(double v) {
    Value x;
    x.kind   = Kind::Number;
    x.number = v;
    return x;
}

Value Value::Bool(bool v) {
    Value x;
    x.kind   = Kind::Bool;
    x.number = v ? 1.0 : 0.0;
    return x;
}

Value Value::String(const string& v) {
    Value x;
    x.kind = Kind::String;
    x.text = v;
    return x;
}

Value Value::Char(char c) {
    Value x;
    x.kind   = Kind::Char;
    x.number = static_cast<unsigned char>(c);
    x.text   = string(1, c);
    return x;
}

Value Value::Void() {
    return Value{};
}

bool Value::isNumericLike() const {
    return kind == Kind::Number || kind == Kind::Bool || kind == Kind::Char;
}

double Value::asNumber() const {
    if (kind == Kind::String) {
        try { return stod(text); } catch (...) { return 0.0; }
    }
    return number;
}

bool Value::asBool() const {
    if (kind == Kind::String) return !text.empty();
    if (kind == Kind::Void)   return false;
    return fabs(asNumber()) > 1e-12;
}

string Value::toString() const {
    if (kind == Kind::Void)   return "void";
    if (kind == Kind::String) return text;
    if (kind == Kind::Char)   return text;
    if (kind == Kind::Bool)   return asBool() ? "true" : "false";

    double v = number;
    if (fabs(v - llround(v)) < 1e-9)
        return to_string(static_cast<long long>(llround(v)));

    ostringstream oss;
    oss << fixed << setprecision(6) << v;
    string s = oss.str();
    while (!s.empty() && s.back() == '0') s.pop_back();
    if (!s.empty() && s.back() == '.')    s.pop_back();
    return s;
}

MipsLikeSimulator::MipsLikeSimulator(bool traceMode) : trace(traceMode) {}

bool MipsLikeSimulator::loadProgram(const string& filename) {
    ifstream in(filename);
    if (!in.is_open()) {
        cerr << "Erro: nao foi possivel abrir o arquivo '" << filename << "'.\n";
        return false;
    }

    string line;
    while (getline(in, line)) {
        line = trim(line);
        if (!line.empty()) lines.push_back(line);
    }

    if (lines.empty()) {
        cerr << "Erro: arquivo de codigo de tres enderecos esta vazio.\n";
        return false;
    }

    indexProgram();
    prepareGlobals();
    return true;
}

int MipsLikeSimulator::run() {
    string entry = "main";
    if (functions.find(entry) == functions.end()) {
        if (functions.empty()) {
            cerr << "Erro: nenhuma funcao encontrada no codigo de tres enderecos.\n";
            return 1;
        }
        entry = functions.begin()->first;
        cerr << "Aviso: funcao main nao encontrada. Executando '" << entry << "'.\n";
    }

    Value ret = executeFunction(entry, {});
    cerr << "\n[SIMULADOR] Programa finalizado. Retorno de " << entry
         << " = " << ret.toString() << "\n";

    if (!warnings.empty()) {
        cerr << "\n[SIMULADOR] Avisos encontrados:\n";
        for (const auto& w : warnings) cerr << "- " << w << "\n";
    }

    return 0;
}

void MipsLikeSimulator::indexProgram() {
    functions.clear();

    string currentFunction;
    for (int i = 0; i < static_cast<int>(lines.size()); ++i) {
        const string& line = lines[i];

        if (startsWith(line, "function ")) {
            vector<string> words = splitWords(line);
            if (words.size() >= 4 && words[2] == "returns") {
                FunctionInfo f;
                f.name       = words[1];
                f.returnType = words[3];
                f.start      = i;
                functions[f.name] = f;
                currentFunction   = f.name;
            }
            continue;
        }

        if (startsWith(line, "end function ")) {
            if (!currentFunction.empty()) {
                functions[currentFunction].end = i;
                currentFunction.clear();
            }
            continue;
        }

        if (!currentFunction.empty()) {
            if (startsWith(line, "parameter ")) {
                string name = parseDeclaredName(line);
                if (!name.empty()) functions[currentFunction].parameters.push_back(name);
            }
            if (isLabel(line)) {
                string label = line.substr(0, line.size() - 1);
                functions[currentFunction].labels[label] = i;
            }
        }
    }

    for (auto& entry : functions) {
        FunctionInfo& f = entry.second;
        if (f.end < 0) f.end = static_cast<int>(lines.size());
    }
}

void MipsLikeSimulator::prepareGlobals() {
    bool insideFunction = false;
    for (const string& line : lines) {
        if (startsWith(line, "function "))     { insideFunction = true;  continue; }
        if (startsWith(line, "end function ")) { insideFunction = false; continue; }
        if (!insideFunction && startsWith(line, "variable "))
            declareVariable(line, globalVars, globalArrays);
    }
}

bool MipsLikeSimulator::isLabel(const string& line) const {
    if (!endsWith(line, ":"))             return false;
    if (line.find(' ') != string::npos)   return false;
    return true;
}

string MipsLikeSimulator::parseDeclaredName(const string& line) const {
    vector<string> words = splitWords(line);
    if (words.size() >= 2) return words[1];
    return "";
}

string MipsLikeSimulator::parseDeclaredType(const string& line) const {
    size_t pos = line.find(':');
    if (pos == string::npos) return "int";
    return trim(line.substr(pos + 1));
}

int MipsLikeSimulator::parseArraySize(const string& type) const {
    size_t lb = type.find('[');
    size_t rb = type.find(']', lb == string::npos ? 0 : lb + 1);
    if (lb == string::npos || rb == string::npos || rb <= lb + 1) return -1;
    string inside = trim(type.substr(lb + 1, rb - lb - 1));
    try { return stoi(inside); } catch (...) { return -1; }
}

Value MipsLikeSimulator::defaultValueForType(const string& type) const {
    string t = type;
    transform(t.begin(), t.end(), t.begin(),
              [](unsigned char c){ return static_cast<char>(tolower(c)); });
    if (startsWith(t, "string")) return Value::String("");
    if (startsWith(t, "char"))   return Value::Char('\0');
    if (startsWith(t, "bool"))   return Value::Bool(false);
    return Value::Number(0);
}

void MipsLikeSimulator::declareVariable(const string& line,
                                        unordered_map<string, Value>& vars,
                                        unordered_map<string, vector<Value>>& arrays) {
    string name = parseDeclaredName(line);
    string type = parseDeclaredType(line);
    if (name.empty()) return;

    int arraySize = parseArraySize(type);
    if (arraySize >= 0)
        arrays[name] = vector<Value>(static_cast<size_t>(arraySize), defaultValueForType(type));
    else
        vars[name] = defaultValueForType(type);
}

Value MipsLikeSimulator::executeFunction(const string& functionName, const vector<Value>& args) {
    auto it = functions.find(functionName);
    if (it == functions.end()) {
        warnings.push_back("chamada para funcao inexistente '" + functionName + "'. Retornando 0.");
        return Value::Number(0);
    }

    const FunctionInfo& f = it->second;
    Frame frame;
    frame.functionName = functionName;

    for (size_t i = 0; i < f.parameters.size(); ++i)
        frame.vars[f.parameters[i]] = i < args.size() ? args[i] : Value::Number(0);

    int pc = f.start + 1;
    while (pc < f.end) {
        string line = lines[pc];

        if (trace) cerr << "[TRACE] " << functionName << " pc=" << pc << " :: " << line << "\n";

        if (line.empty() || isLabel(line) || startsWith(line, "parameter ")) {
            pc++; continue;
        }

        if (startsWith(line, "variable ")) {
            declareVariable(line, frame.vars, frame.arrays);
            pc++; continue;
        }

        if (startsWith(line, "goto ")) {
            string label = trim(line.substr(5));
            pc = jumpToLabel(f, label, pc);
            continue;
        }

        if (startsWith(line, "ifFalse ")) {
            string rest = trim(line.substr(8));
            size_t pos  = rest.find(" goto ");
            if (pos == string::npos) {
                warnings.push_back("instrucao ifFalse mal formada: " + line);
                pc++; continue;
            }
            string condExpr = trim(rest.substr(0, pos));
            string label    = trim(rest.substr(pos + 6));
            Value  cond     = evalOperand(frame, condExpr);
            pc = cond.asBool() ? pc + 1 : jumpToLabel(f, label, pc);
            continue;
        }

        if (startsWith(line, "print ")) {
            cout << evalOperand(frame, trim(line.substr(6))).toString() << "\n";
            pc++; continue;
        }

        if (startsWith(line, "read ")) {
            string target = trim(line.substr(5));
            string input;
            cout << "[read " << target << "] ";
            cin >> input;
            setValue(frame, target, parseInputValue(input));
            pc++; continue;
        }

        if (startsWith(line, "return")) {
            string expr = trim(line.substr(6));
            if (expr.empty()) return Value::Void();
            return evalOperand(frame, expr);
        }

        if (startsWith(line, "param ")) {
            frame.pendingParams.push_back(evalOperand(frame, trim(line.substr(6))));
            pc++; continue;
        }

        size_t eq = line.find(" = ");
        if (eq != string::npos) {
            string lhs = trim(line.substr(0, eq));
            string rhs = trim(line.substr(eq + 3));
            setValue(frame, lhs, evalRhs(frame, rhs));
            pc++; continue;
        }

        warnings.push_back("instrucao desconhecida ignorada: " + line);
        pc++;
    }

    return Value::Void();
}

int MipsLikeSimulator::jumpToLabel(const FunctionInfo& f, const string& label, int currentPc) {
    auto it = f.labels.find(label);
    if (it == f.labels.end()) {
        warnings.push_back("label '" + label + "' nao encontrado na funcao '" + f.name + "'.");
        return currentPc + 1;
    }
    return it->second;
}

Value MipsLikeSimulator::parseInputValue(const string& input) const {
    if (input == "true")  return Value::Bool(true);
    if (input == "false") return Value::Bool(false);
    char* end = nullptr;
    double v  = strtod(input.c_str(), &end);
    if (end != input.c_str() && *end == '\0') return Value::Number(v);
    return Value::String(input);
}

ArrayRef MipsLikeSimulator::parseArrayRef(const string& expr) const {
    string s   = trim(expr);
    size_t lb  = s.find('[');
    size_t rb  = s.rfind(']');
    if (lb == string::npos || rb == string::npos || rb <= lb) return {};
    ArrayRef ref;
    ref.isArray   = true;
    ref.base      = trim(s.substr(0, lb));
    ref.indexExpr = trim(s.substr(lb + 1, rb - lb - 1));
    return ref;
}

Value MipsLikeSimulator::getValue(Frame& frame, const string& name) {
    ArrayRef ref = parseArrayRef(name);
    if (ref.isArray) return getArrayValue(frame, ref);

    auto local = frame.vars.find(name);
    if (local != frame.vars.end()) return local->second;

    auto global = globalVars.find(name);
    if (global != globalVars.end()) return global->second;

    warnings.push_back("uso de variavel nao inicializada/declarada '" + name + "'. Usando 0.");
    frame.vars[name] = Value::Number(0);
    return frame.vars[name];
}

void MipsLikeSimulator::setValue(Frame& frame, const string& name, const Value& value) {
    ArrayRef ref = parseArrayRef(name);
    if (ref.isArray) { setArrayValue(frame, ref, value); return; }

    if (frame.vars.find(name)   != frame.vars.end())   { frame.vars[name]   = value; return; }
    if (globalVars.find(name)   != globalVars.end())   { globalVars[name]   = value; return; }

    frame.vars[name] = value; // temporários / variáveis não declaradas
}

Value MipsLikeSimulator::getArrayValue(Frame& frame, const ArrayRef& ref) {
    int idx    = static_cast<int>(evalOperand(frame, ref.indexExpr).asNumber());
    auto local = frame.arrays.find(ref.base);
    if (local != frame.arrays.end()) return readArray(local->second, ref.base, idx);
    auto global = globalArrays.find(ref.base);
    if (global != globalArrays.end()) return readArray(global->second, ref.base, idx);

    warnings.push_back("uso de vetor nao declarado '" + ref.base + "'. Retornando 0.");
    return Value::Number(0);
}

void MipsLikeSimulator::setArrayValue(Frame& frame, const ArrayRef& ref, const Value& value) {
    int idx    = static_cast<int>(evalOperand(frame, ref.indexExpr).asNumber());
    auto local = frame.arrays.find(ref.base);
    if (local != frame.arrays.end()) { writeArray(local->second, ref.base, idx, value); return; }
    auto global = globalArrays.find(ref.base);
    if (global != globalArrays.end()) { writeArray(global->second, ref.base, idx, value); return; }

    warnings.push_back("atribuicao em vetor nao declarado '" + ref.base + "'. Criando vetor dinamico.");
    frame.arrays[ref.base] = vector<Value>(static_cast<size_t>(max(10, idx + 1)), Value::Number(0));
    writeArray(frame.arrays[ref.base], ref.base, idx, value);
}

Value MipsLikeSimulator::readArray(const vector<Value>& arr, const string& name, int idx) {
    if (idx < 0 || idx >= static_cast<int>(arr.size())) {
        warnings.push_back("indice fora do limite em '" + name + "[" + to_string(idx) + "]'. Retornando 0.");
        return Value::Number(0);
    }
    return arr[static_cast<size_t>(idx)];
}

void MipsLikeSimulator::writeArray(vector<Value>& arr, const string& name, int idx, const Value& value) {
    if (idx < 0) {
        warnings.push_back("indice negativo em '" + name + "[" + to_string(idx) + "]'. Escrita ignorada.");
        return;
    }
    if (idx >= static_cast<int>(arr.size())) {
        warnings.push_back("indice fora do limite em '" + name + "[" + to_string(idx) + "]'. Expandindo vetor dinamicamente.");
        arr.resize(static_cast<size_t>(idx + 1), Value::Number(0));
    }
    arr[static_cast<size_t>(idx)] = value;
}

bool MipsLikeSimulator::isNumberLiteral(const string& s) const {
    if (s.empty()) return false;
    char* end = nullptr;
    strtod(s.c_str(), &end);
    return end != s.c_str() && *end == '\0';
}

Value MipsLikeSimulator::evalOperand(Frame& frame, const string& expr) {
    string s = trim(expr);
    if (s.empty()) return Value::Void();

    if (s == "true")  return Value::Bool(true);
    if (s == "false") return Value::Bool(false);

    if (s.size() >= 2 && s.front() == '"' && s.back() == '"')
        return Value::String(unquoteString(s));

    if (s.size() >= 3 && s.front() == '\'' && s.back() == '\'') {
        string ch = unquoteString(s);
        return Value::Char(ch.empty() ? '\0' : ch[0]);
    }

    if (isNumberLiteral(s))
        return Value::Number(strtod(s.c_str(), nullptr));

    return getValue(frame, s);
}

Value MipsLikeSimulator::evalRhs(Frame& frame, const string& rhs) {
    string s = trim(rhs);

    if (startsWith(s, "call ")) return evalCall(frame, s);

    if (startsWith(s, "!") && s.size() > 1)
        return Value::Bool(!evalOperand(frame, trim(s.substr(1))).asBool());

    string op;
    size_t pos = findBinaryOperator(s, op);
    if (pos != string::npos) {
        string left  = trim(s.substr(0, pos));
        string right = trim(s.substr(pos + op.size()));
        return applyBinary(evalOperand(frame, left), op, evalOperand(frame, right));
    }

    return evalOperand(frame, s);
}

Value MipsLikeSimulator::evalCall(Frame& frame, const string& callExpr) {
    string rest   = trim(callExpr.substr(5));
    size_t comma  = rest.find(',');
    string callee = comma == string::npos ? trim(rest) : trim(rest.substr(0, comma));
    int n = 0;
    if (comma != string::npos) {
        try { n = stoi(trim(rest.substr(comma + 1))); }
        catch (...) {
            warnings.push_back("quantidade de argumentos invalida em chamada: " + callExpr);
        }
    }

    vector<Value> args;
    if (n > static_cast<int>(frame.pendingParams.size())) {
        warnings.push_back("chamada para '" + callee + "' pediu " + to_string(n) +
                           " parametro(s), mas havia apenas " +
                           to_string(frame.pendingParams.size()) + ".");
        args = frame.pendingParams;
        frame.pendingParams.clear();
    } else {
        size_t start = frame.pendingParams.size() - static_cast<size_t>(n);
        args.assign(frame.pendingParams.begin() + static_cast<long>(start),
                    frame.pendingParams.end());
        frame.pendingParams.erase(frame.pendingParams.begin() + static_cast<long>(start),
                                  frame.pendingParams.end());
    }

    return executeFunction(callee, args);
}

size_t MipsLikeSimulator::findBinaryOperator(const string& s, string& foundOp) const {
    vector<string> ops = {"||", "&&", "==", "!=", "<=", ">=", "+", "-", "*", "/", "%", "<", ">"};

    bool inDouble = false, inSingle = false;
    int  bracketDepth = 0;

    for (size_t i = 0; i < s.size(); ++i) {
        char c = s[i];
        if      (c == '"'  && !inSingle) inDouble = !inDouble;
        else if (c == '\'' && !inDouble) inSingle = !inSingle;
        else if (!inDouble && !inSingle) {
            if      (c == '[') bracketDepth++;
            else if (c == ']') bracketDepth = max(0, bracketDepth - 1);
        }

        if (inDouble || inSingle || bracketDepth > 0) continue;

        for (const string& op : ops) {
            if (i + op.size() <= s.size() && s.compare(i, op.size(), op) == 0) {
                bool leftSpace  = (i > 0 && isspace(static_cast<unsigned char>(s[i - 1])));
                bool rightSpace = (i + op.size() < s.size() &&
                                   isspace(static_cast<unsigned char>(s[i + op.size()])));
                if (leftSpace && rightSpace) { foundOp = op; return i; }
            }
        }
    }
    return string::npos;
}

Value MipsLikeSimulator::applyBinary(const Value& a, const string& op, const Value& b) {
    if (op == "+") {
        if (a.kind == Value::Kind::String || b.kind == Value::Kind::String)
            return Value::String(a.toString() + b.toString());
        return Value::Number(a.asNumber() + b.asNumber());
    }
    if (op == "-")  return Value::Number(a.asNumber() - b.asNumber());
    if (op == "*")  return Value::Number(a.asNumber() * b.asNumber());
    if (op == "/") {
        if (fabs(b.asNumber()) < 1e-12) {
            warnings.push_back("divisao por zero. Retornando 0.");
            return Value::Number(0);
        }
        return Value::Number(a.asNumber() / b.asNumber());
    }
    if (op == "%") {
        long long x = static_cast<long long>(a.asNumber());
        long long y = static_cast<long long>(b.asNumber());
        if (y == 0) { warnings.push_back("modulo por zero. Retornando 0."); return Value::Number(0); }
        return Value::Number(x % y);
    }

    if (op == "<")  return Value::Bool(a.asNumber() <  b.asNumber());
    if (op == "<=") return Value::Bool(a.asNumber() <= b.asNumber());
    if (op == ">")  return Value::Bool(a.asNumber() >  b.asNumber());
    if (op == ">=") return Value::Bool(a.asNumber() >= b.asNumber());

    if (op == "==") {
        if (a.isNumericLike() && b.isNumericLike())
            return Value::Bool(fabs(a.asNumber() - b.asNumber()) < 1e-12);
        return Value::Bool(a.toString() == b.toString());
    }
    if (op == "!=") {
        if (a.isNumericLike() && b.isNumericLike())
            return Value::Bool(fabs(a.asNumber() - b.asNumber()) >= 1e-12);
        return Value::Bool(a.toString() != b.toString());
    }

    if (op == "&&") return Value::Bool(a.asBool() && b.asBool());
    if (op == "||") return Value::Bool(a.asBool() || b.asBool());

    warnings.push_back("operador desconhecido '" + op + "'. Retornando 0.");
    return Value::Number(0);
}

int runSim(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Uso:\n";
        cerr << "  " << argv[0] << " 3_code.txt\n";
        cerr << "  " << argv[0] << " 3_code.txt --trace\n";
        return 1;
    }

    string filename = argv[1];
    bool   trace    = false;
    for (int i = 2; i < argc; ++i)
        if (string(argv[i]) == "--trace") trace = true;

    MipsLikeSimulator sim(trace);
    if (!sim.loadProgram(filename)) return 1;
    return sim.run();
}
