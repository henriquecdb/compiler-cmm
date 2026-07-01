#pragma once

#include <bits/stdc++.h>

struct Value
{
    enum class Kind
    {
        Number,
        Bool,
        String,
        Char,
        Void
    } kind = Kind::Void;

    double number = 0.0;
    std::string text;

    static Value Number(double v);
    static Value Bool(bool v);
    static Value String(const std::string &v);
    static Value Char(char c);
    static Value Void();

    bool isNumericLike() const;
    double asNumber() const;
    bool asBool() const;
    std::string toString() const;
};

struct FunctionInfo
{
    std::string name;
    std::string returnType;
    int start = -1;
    int end = -1;
    std::unordered_map<std::string, int> labels;
    std::vector<std::string> parameters;
};

struct Frame
{
    std::string functionName;
    std::unordered_map<std::string, Value> vars;
    std::unordered_map<std::string, std::vector<Value>> arrays;
    std::vector<Value> pendingParams;
};

struct ArrayRef
{
    bool isArray = false;
    std::string base;
    std::string indexExpr;
};

class MipsLikeSimulator
{
public:
    explicit MipsLikeSimulator(bool traceMode = false);

    bool loadProgram(const std::string &filename);

    int run();

private:
    std::vector<std::string> lines;
    std::unordered_map<std::string, FunctionInfo> functions;
    std::unordered_map<std::string, Value> globalVars;
    std::unordered_map<std::string, std::vector<Value>> globalArrays;
    bool trace = false;
    std::vector<std::string> warnings;

    void indexProgram();
    void prepareGlobals();

    bool isLabel(const std::string &line) const;
    std::string parseDeclaredName(const std::string &line) const;
    std::string parseDeclaredType(const std::string &line) const;
    int parseArraySize(const std::string &type) const;
    Value defaultValueForType(const std::string &type) const;

    void declareVariable(const std::string &line,
                         std::unordered_map<std::string, Value> &vars,
                         std::unordered_map<std::string, std::vector<Value>> &arrays);

    Value executeFunction(const std::string &functionName, const std::vector<Value> &args);
    int jumpToLabel(const FunctionInfo &f, const std::string &label, int currentPc);

    Value parseInputValue(const std::string &input) const;
    ArrayRef parseArrayRef(const std::string &expr) const;

    Value getValue(Frame &frame, const std::string &name);
    void setValue(Frame &frame, const std::string &name, const Value &value);

    Value getArrayValue(Frame &frame, const ArrayRef &ref);
    void setArrayValue(Frame &frame, const ArrayRef &ref, const Value &value);

    Value readArray(const std::vector<Value> &arr, const std::string &name, int idx);
    void writeArray(std::vector<Value> &arr, const std::string &name, int idx, const Value &value);

    bool isNumberLiteral(const std::string &s) const;
    Value evalOperand(Frame &frame, const std::string &expr);
    Value evalRhs(Frame &frame, const std::string &rhs);
    Value evalCall(Frame &frame, const std::string &callExpr);

    size_t findBinaryOperator(const std::string &s, std::string &foundOp) const;
    Value applyBinary(const Value &a, const std::string &op, const Value &b);
};

int runSim(int argc, char *argv[]);
