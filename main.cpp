
#include <bits/stdc++.h>
using namespace std;

// ===================== IR STRUCT =====================
struct IR {
    string op, arg1, arg2, result;
};

// ===================== GLOBAL =====================
int tempCount = 0;

string newTemp() {
    return "t" + to_string(tempCount++);
}

// ===================== IR GENERATION =====================
vector<IR> generateIR(string expr) {
    vector<IR> ir;

    expr.erase(remove(expr.begin(), expr.end(), ' '), expr.end());

    string lhs = expr.substr(0, expr.find('='));
    string rhs = expr.substr(expr.find('=') + 1);

    stack<string> values;
    stack<char> ops;

    auto applyOp = [&](char op) {
        string b = values.top(); values.pop();
        string a = values.top(); values.pop();

        string temp = newTemp();
        string opStr;

        if (op == '+') opStr = "ADD";
        else if (op == '-') opStr = "SUB";
        else if (op == '*') opStr = "MUL";
        else if (op == '/') opStr = "DIV";

        ir.push_back({opStr, a, b, temp});
        values.push(temp);
    };

    auto precedence = [&](char op) {
        if (op == '+' || op == '-') return 1;
        if (op == '*' || op == '/') return 2;
        return 0;
    };

    for (int i = 0; i < rhs.size(); i++) {
        if (isalnum(rhs[i])) {
            values.push(string(1, rhs[i]));
        } else {
            while (!ops.empty() && precedence(ops.top()) >= precedence(rhs[i])) {
                applyOp(ops.top());
                ops.pop();
            }
            ops.push(rhs[i]);
        }
    }

    while (!ops.empty()) {
        applyOp(ops.top());
        ops.pop();
    }

    ir.push_back({"MOV", values.top(), "", lhs});
    return ir;
}

// ===================== OPTIMIZATION =====================
vector<IR> optimizeIR(vector<IR>& ir) {
    vector<IR> optimized;
    unordered_map<string, string> constMap;

    for (auto &inst : ir) {
        string a = inst.arg1;
        string b = inst.arg2;

        if (constMap.count(a)) a = constMap[a];
        if (constMap.count(b)) b = constMap[b];

        // Constant folding
        if ((inst.op == "ADD" || inst.op == "MUL") &&
            !a.empty() && !b.empty() &&
            isdigit(a[0]) && isdigit(b[0])) {

            int x = stoi(a);
            int y = stoi(b);
            int res = (inst.op == "ADD") ? x + y : x * y;

            constMap[inst.result] = to_string(res);
            optimized.push_back({"MOV", to_string(res), "", inst.result});
        }
        else if (inst.op == "MOV" && !a.empty() && isdigit(a[0])) {
            constMap[inst.result] = a;
            optimized.push_back({"MOV", a, "", inst.result});
        }
        else {
            optimized.push_back({inst.op, a, b, inst.result});
        }
    }

    return optimized;
}

// ===================== CODE GENERATION =====================
void generateAssembly(vector<IR>& ir) {
    cout << "\n--- PTX-like Assembly ---\n";

    // Get final instruction
    IR finalInst = ir.back();
    string val = finalInst.arg1;

    // Remove spaces (safety)
    val.erase(remove(val.begin(), val.end(), ' '), val.end());

    // If final result is constant → directly emit
    if (!val.empty() && isdigit(val[0])) {
        cout << "mov.s32 " << finalInst.result << ", " << val << ";\n";
        return;
    }

    // Otherwise fallback (rare case)
    unordered_map<string, string> regMap;
    int regCount = 1;

    auto getReg = [&](string var) {
        if (var.empty()) return var;
        if (isdigit(var[0])) return var;

        if (!regMap.count(var))
            regMap[var] = "r" + to_string(regCount++);

        return regMap[var];
    };

    for (auto &inst : ir) {
        string r1 = getReg(inst.arg1);
        string r2 = getReg(inst.arg2);
        string r3 = getReg(inst.result);

        if (inst.op == "ADD")
            cout << "add.s32 " << r3 << ", " << r1 << ", " << r2 << ";\n";
        else if (inst.op == "MUL")
            cout << "mul.lo.s32 " << r3 << ", " << r1 << ", " << r2 << ";\n";
        else if (inst.op == "MOV")
            cout << "mov.s32 " << r3 << ", " << r1 << ";\n";
    }
}

// ===================== PRINT IR =====================
void printIR(vector<IR>& ir, string title) {
    cout << "\n--- " << title << " ---\n";
    for (auto &i : ir) {
        cout << i.result << " = " << i.arg1;
        if (!i.arg2.empty())
            cout << " " << i.op << " " << i.arg2;
        cout << "\n";
    }
}

// ===================== SUMMARY =====================
void printSummary(vector<IR>& before, vector<IR>& after) {
    cout << "\n--- Optimization Summary ---\n";
    cout << "Instructions Before: " << before.size() << "\n";
    cout << "Instructions After : " << after.size() << "\n";
}

// ===================== MAIN =====================
int main() {
    string input;

    cout << "Enter expression (e.g., a = b + c * d): ";
    getline(cin, input);

    vector<IR> ir = generateIR(input);
    printIR(ir, "Intermediate Representation");

    vector<IR> optimized = optimizeIR(ir);
    printIR(optimized, "Optimized IR");

    generateAssembly(optimized);

    printSummary(ir, optimized);

    return 0;
}