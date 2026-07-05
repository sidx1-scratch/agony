// ============================================================================
// AGONY — Assembly-Glyph-Opcode-Notation-Yielding language
//
// Every line of AGONY source requires you to simultaneously write:
//   - the opcode in raw 8-bit BINARY
//   - the mnemonic in ASSEMBLY (redundant with the binary — must match, or
//     the interpreter refuses to run)
//   - the operands tagged with EMOJI (register / number / string / label)
//   - a PARITY-CHECK emoji (✅ even bit-parity / ❌ odd bit-parity of the
//     opcode byte — also redundant, also enforced)
//   - a CHECKSUM emoji sequence that spells the opcode's decimal value out
//     again using keycap digit emoji (0️⃣-9️⃣) — triple-redundant, mandatory
//   - a mandatory closing flourish: 🏁
//
// Get any of the five redundant encodings of the same instruction out of
// sync with each other and the program refuses to compile. This is the
// entire point.
//
// Zero runtime dependencies. Single file. Compile with:
//   g++ -std=c++17 -O2 -o agony agony.cpp
// Run with:
//   ./agony program.agony
// ============================================================================

#include <bits/stdc++.h>
using namespace std;

// ---------------------------------------------------------------------------
// Emoji constants (raw UTF-8 byte sequences via normal string literals —
// this source file is UTF-8, so these just work).
// ---------------------------------------------------------------------------
static const string TAG_REG   = "\U0001F4CD"; // 📍 register
static const string TAG_NUM   = "\U0001F522"; // 🔢 number literal
static const string TAG_STR   = "\U0001F524"; // 🔤 string literal
static const string TAG_LABEL = "\U0001F3AF"; // 🎯 label
static const string PARITY_EVEN = "\u2705";   // ✅
static const string PARITY_ODD  = "\u274C";   // ❌
static const string TERMINATOR  = "\U0001F3C1"; // 🏁

static const string DIGIT_EMOJI[10] = {
    "0\uFE0F\u20E3","1\uFE0F\u20E3","2\uFE0F\u20E3","3\uFE0F\u20E3","4\uFE0F\u20E3",
    "5\uFE0F\u20E3","6\uFE0F\u20E3","7\uFE0F\u20E3","8\uFE0F\u20E3","9\uFE0F\u20E3"
};

// ---------------------------------------------------------------------------
// Opcode table — the ONE source of truth. Binary, mnemonic, parity, and
// checksum in the source are all independently validated against this.
// ---------------------------------------------------------------------------
enum Op {
    OP_NOP=1, OP_PUSH=2, OP_POP=3, OP_ADD=4, OP_SUB=5, OP_MUL=6, OP_DIV=7,
    OP_MOV=8, OP_PRINT=9, OP_PRINTS=10, OP_JMP=11, OP_JZ=12, OP_LBL=13,
    OP_JNZ=14, OP_HALT=15
};

static const map<int,string> MNEMONIC = {
    {OP_NOP,"NOP"}, {OP_PUSH,"PUSH"}, {OP_POP,"POP"}, {OP_ADD,"ADD"},
    {OP_SUB,"SUB"}, {OP_MUL,"MUL"}, {OP_DIV,"DIV"}, {OP_MOV,"MOV"},
    {OP_PRINT,"PRINT"}, {OP_PRINTS,"PRINTS"}, {OP_JMP,"JMP"}, {OP_JZ,"JZ"},
    {OP_LBL,"LBL"}, {OP_JNZ,"JNZ"}, {OP_HALT,"HALT"}
};

// ---------------------------------------------------------------------------
// Utility: split a line on the "::" field separator (exactly, not per-char).
// ---------------------------------------------------------------------------
vector<string> splitFields(const string& line) {
    vector<string> out;
    size_t pos = 0;
    while (true) {
        size_t next = line.find("::", pos);
        if (next == string::npos) { out.push_back(line.substr(pos)); break; }
        out.push_back(line.substr(pos, next - pos));
        pos = next + 2;
    }
    return out;
}

struct AgonyError : public runtime_error {
    AgonyError(const string& msg) : runtime_error(msg) {}
};

enum OperandType { REG, NUM, STR, LABEL };
struct Operand { OperandType type; string value; };

// ---------------------------------------------------------------------------
// Tokenize the operand field into a sequence of tagged operands by scanning
// raw UTF-8 bytes for the known multi-byte tag emoji sequences. ASCII value
// bytes (digits, letters, quotes) never collide with these multi-byte
// sequences, so a byte-wise scan is sufficient.
// ---------------------------------------------------------------------------
vector<Operand> parseOperands(const string& field, int lineNo) {
    vector<Operand> ops;
    static const vector<pair<string,OperandType>> tags = {
        {TAG_REG, REG}, {TAG_NUM, NUM}, {TAG_STR, STR}, {TAG_LABEL, LABEL}
    };
    int curType = -1;
    string cur;
    size_t i = 0;
    auto flush = [&]() {
        if (curType != -1) {
            string v = cur;
            if ((OperandType)curType == STR) {
                // strip surrounding quotes
                size_t a = v.find('"');
                size_t b = v.rfind('"');
                if (a != string::npos && b != string::npos && b > a) v = v.substr(a+1, b-a-1);
                // minimal escape processing
                string unescaped;
                for (size_t k = 0; k < v.size(); k++) {
                    if (v[k] == '\\' && k+1 < v.size()) {
                        char n = v[k+1];
                        if (n == 'n') { unescaped += '\n'; k++; continue; }
                        if (n == 't') { unescaped += '\t'; k++; continue; }
                        if (n == '"') { unescaped += '"'; k++; continue; }
                        if (n == '\\') { unescaped += '\\'; k++; continue; }
                    }
                    unescaped += v[k];
                }
                v = unescaped;
            }
            ops.push_back({(OperandType)curType, v});
        }
    };
    while (i < field.size()) {
        bool matched = false;
        for (auto& t : tags) {
            if (field.compare(i, t.first.size(), t.first) == 0) {
                flush();
                curType = t.second;
                cur.clear();
                i += t.first.size();
                matched = true;
                break;
            }
        }
        if (!matched) { cur += field[i]; i++; }
    }
    flush();
    if (field.empty()) return {};
    return ops;
}

int popcount8(int byte) {
    int c = 0;
    for (int i = 0; i < 8; i++) if (byte & (1 << i)) c++;
    return c;
}

string decimalToEmoji(int n) {
    string s = to_string(n);
    string out;
    for (char c : s) out += DIGIT_EMOJI[c - '0'];
    return out;
}

// ---------------------------------------------------------------------------
// A parsed instruction.
// ---------------------------------------------------------------------------
struct Instr {
    int opcode;
    vector<Operand> operands;
    int lineNo;
};

// ---------------------------------------------------------------------------
// Parse one line into an Instr. Throws AgonyError with a precise, gleefully
// pedantic message on any of the five redundant encodings disagreeing.
// ---------------------------------------------------------------------------
Instr parseLine(const string& rawLine, int lineNo) {
    vector<string> f = splitFields(rawLine);
    if (f.size() != 6) {
        throw AgonyError("line " + to_string(lineNo) + ": expected exactly 6 '::'-separated "
            "fields [BINARY::MNEMONIC::OPERANDS::PARITY::CHECKSUM::🏁], got " + to_string(f.size()));
    }
    string binary = f[0], mnemonic = f[1], operandsField = f[2], parity = f[3], checksum = f[4], term = f[5];

    if (binary.size() != 8 || binary.find_first_not_of("01") != string::npos) {
        throw AgonyError("line " + to_string(lineNo) + ": opcode field '" + binary +
            "' is not exactly 8 binary digits. AGONY requires the FULL byte, not your shortcuts.");
    }
    int opcode = 0;
    for (int i = 0; i < 8; i++) if (binary[i] == '1') opcode |= (1 << (7 - i));

    auto it = MNEMONIC.find(opcode);
    if (it == MNEMONIC.end()) {
        throw AgonyError("line " + to_string(lineNo) + ": binary " + binary +
            " (decimal " + to_string(opcode) + ") does not correspond to any known opcode.");
    }
    if (it->second != mnemonic) {
        throw AgonyError("line " + to_string(lineNo) + ": binary " + binary + " decodes to opcode " +
            to_string(opcode) + " (" + it->second + "), but you wrote the mnemonic '" + mnemonic +
            "'. Binary and assembly must agree. That is the whole point.");
    }

    string expectedParity = (popcount8(opcode) % 2 == 0) ? PARITY_EVEN : PARITY_ODD;
    if (parity != expectedParity) {
        throw AgonyError("line " + to_string(lineNo) + ": opcode byte " + binary + " has " +
            to_string(popcount8(opcode)) + " set bits (" +
            (popcount8(opcode) % 2 == 0 ? "even" : "odd") + " parity), which requires " +
            (expectedParity == PARITY_EVEN ? "✅" : "❌") + ", not whatever you wrote.");
    }

    string expectedChecksum = decimalToEmoji(opcode);
    if (checksum != expectedChecksum) {
        throw AgonyError("line " + to_string(lineNo) + ": checksum must spell out decimal " +
            to_string(opcode) + " in keycap emoji digits. Yours didn't match.");
    }

    if (term != TERMINATOR) {
        throw AgonyError("line " + to_string(lineNo) + ": every instruction must end with the "
            "mandatory flourish 🏁. Without it, how would anyone know you were truly done suffering?");
    }

    vector<Operand> ops = parseOperands(operandsField, lineNo);
    return {opcode, ops, lineNo};
}

// ---------------------------------------------------------------------------
// VM
// ---------------------------------------------------------------------------
struct VM {
    vector<long long> registers = vector<long long>(8, 0);
    vector<long long> stack;
    vector<Instr> program;
    map<string,int> labels;

    int regIndex(const string& s, int lineNo) {
        if (s.size() < 2 || s[0] != 'R') throw AgonyError("line " + to_string(lineNo) + ": '" + s + "' is not a register (expected R0-R7).");
        int idx = stoi(s.substr(1));
        if (idx < 0 || idx > 7) throw AgonyError("line " + to_string(lineNo) + ": register index out of range: " + s);
        return idx;
    }

    long long resolve(const Operand& o, int lineNo) {
        if (o.type == REG) return registers[regIndex(o.value, lineNo)];
        if (o.type == NUM) return stoll(o.value);
        throw AgonyError("line " + to_string(lineNo) + ": expected a register or number operand.");
    }

    void run() {
        for (int i = 0; i < (int)program.size(); i++) {
            if (program[i].opcode == OP_LBL) {
                if (program[i].operands.empty() || program[i].operands[0].type != LABEL)
                    throw AgonyError("line " + to_string(program[i].lineNo) + ": LBL requires a 🎯 label operand.");
                labels[program[i].operands[0].value] = i;
            }
        }
        int ip = 0;
        long long steps = 0;
        const long long MAX_STEPS = 50000000;
        while (ip < (int)program.size()) {
            if (++steps > MAX_STEPS) throw AgonyError("execution exceeded step limit (possible infinite loop).");
            Instr& in = program[ip];
            auto& ops = in.operands;
            int next = ip + 1;
            switch (in.opcode) {
                case OP_NOP: case OP_LBL: break;
                case OP_PUSH: stack.push_back(resolve(ops[0], in.lineNo)); break;
                case OP_POP: {
                    if (stack.empty()) throw AgonyError("line " + to_string(in.lineNo) + ": POP on empty stack.");
                    registers[regIndex(ops[0].value, in.lineNo)] = stack.back(); stack.pop_back();
                    break;
                }
                case OP_ADD: registers[regIndex(ops[0].value,in.lineNo)] += resolve(ops[1], in.lineNo); break;
                case OP_SUB: registers[regIndex(ops[0].value,in.lineNo)] -= resolve(ops[1], in.lineNo); break;
                case OP_MUL: registers[regIndex(ops[0].value,in.lineNo)] *= resolve(ops[1], in.lineNo); break;
                case OP_DIV: {
                    long long d = resolve(ops[1], in.lineNo);
                    if (d == 0) throw AgonyError("line " + to_string(in.lineNo) + ": division by zero.");
                    registers[regIndex(ops[0].value,in.lineNo)] /= d;
                    break;
                }
                case OP_MOV: registers[regIndex(ops[0].value,in.lineNo)] = resolve(ops[1], in.lineNo); break;
                case OP_PRINT: cout << resolve(ops[0], in.lineNo); break;
                case OP_PRINTS: cout << ops[0].value; break;
                case OP_JMP: {
                    auto it = labels.find(ops[0].value);
                    if (it == labels.end()) throw AgonyError("line " + to_string(in.lineNo) + ": unknown label '" + ops[0].value + "'.");
                    next = it->second;
                    break;
                }
                case OP_JZ: case OP_JNZ: {
                    long long v = resolve(ops[0], in.lineNo);
                    auto it = labels.find(ops[1].value);
                    if (it == labels.end()) throw AgonyError("line " + to_string(in.lineNo) + ": unknown label '" + ops[1].value + "'.");
                    bool take = (in.opcode == OP_JZ) ? (v == 0) : (v != 0);
                    if (take) next = it->second;
                    break;
                }
                case OP_HALT: return;
                default: throw AgonyError("line " + to_string(in.lineNo) + ": unimplemented opcode.");
            }
            ip = next;
        }
    }
};

int main(int argc, char** argv) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <program.agony>\n";
        return 1;
    }
    ifstream file(argv[1]);
    if (!file) { cerr << "Could not open " << argv[1] << "\n"; return 1; }

    VM vm;
    string line;
    int lineNo = 0;
    try {
        while (getline(file, line)) {
            lineNo++;
            // trim trailing \r (in case of CRLF)
            if (!line.empty() && line.back() == '\r') line.pop_back();
            // skip blank lines and // comments
            string trimmed = line;
            size_t start = trimmed.find_first_not_of(" \t");
            if (start == string::npos) continue;
            if (trimmed.compare(start, 2, "//") == 0) continue;
            vm.program.push_back(parseLine(line, lineNo));
        }
        vm.run();
    } catch (const AgonyError& e) {
        cerr << "AGONY compile/runtime error: " << e.what() << "\n";
        return 1;
    } catch (const exception& e) {
        cerr << "Unexpected error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
