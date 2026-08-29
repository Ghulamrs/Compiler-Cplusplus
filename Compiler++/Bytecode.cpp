// Bytecode.cpp
//
// C++98 only.

#include "Bytecode.h"

#include <cstddef>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>

const char *opCodeName(OpCode op) {
    switch (op) {
    case OP_PushConst:    return "push";
    case OP_PushFConst:   return "fpush";
    case OP_LoadReg:      return "ldr";
    case OP_StoreReg:     return "str";
    case OP_Pop:          return "pop";
    case OP_LocalAddr:    return "local";
    case OP_StaticAddr:   return "static";
    case OP_FieldAddr:    return "field";
    case OP_FuncAddr:     return "funcaddr";
    case OP_Load:         return "load";
    case OP_Store:        return "store";
    case OP_MemCopy:      return "memcpy";
    case OP_Add:          return "add";
    case OP_Sub:          return "sub";
    case OP_Mul:          return "mul";
    case OP_Div:          return "div";
    case OP_Mod:          return "mod";
    case OP_UDiv:         return "udiv";
    case OP_UMod:         return "umod";
    case OP_Neg:          return "neg";
    case OP_Not:          return "not";
    case OP_FAdd:         return "fadd";
    case OP_FSub:         return "fsub";
    case OP_FMul:         return "fmul";
    case OP_FDiv:         return "fdiv";
    case OP_FNeg:         return "fneg";
    case OP_CmpEQ:        return "cmp.eq";
    case OP_CmpNE:        return "cmp.ne";
    case OP_CmpLT:        return "cmp.lt";
    case OP_CmpGT:        return "cmp.gt";
    case OP_CmpLE:        return "cmp.le";
    case OP_CmpGE:        return "cmp.ge";
    case OP_UCmpLT:       return "ucmp.lt";
    case OP_UCmpGT:       return "ucmp.gt";
    case OP_UCmpLE:       return "ucmp.le";
    case OP_UCmpGE:       return "ucmp.ge";
    case OP_FCmpEQ:       return "fcmp.eq";
    case OP_FCmpNE:       return "fcmp.ne";
    case OP_FCmpLT:       return "fcmp.lt";
    case OP_FCmpGT:       return "fcmp.gt";
    case OP_FCmpLE:       return "fcmp.le";
    case OP_FCmpGE:       return "fcmp.ge";
    case OP_IntToFloat:   return "itof";
    case OP_FloatToInt:   return "ftoi";
    case OP_FloatResize:  return "fresize";
    case OP_IntResize:    return "iresize";
    case OP_Jump:         return "jump";
    case OP_BranchZero:   return "brz";
    case OP_BranchNZ:     return "brnz";
    case OP_Call:         return "call";
    case OP_CallIndirect: return "call.ind";
    case OP_VTableLoad:   return "vtable";
    case OP_Native:       return "native";
    case OP_Return:       return "ret";
    case OP_ReturnVoid:   return "ret.void";
    case OP_Alloc:        return "alloc";
    case OP_Free:         return "free";
    case OP_Halt:         return "halt";
    }
    return "?";
}

NativeId nativeByName(const std::string &name) {
    if (name == "print_int")    return NAT_PrintInt;
    if (name == "print_char")   return NAT_PrintChar;
    if (name == "print_double") return NAT_PrintDouble;
    if (name == "print_string") return NAT_PrintString;
    if (name == "print_line")   return NAT_PrintLine;
    return NAT_Count;
}

const char *nativeName(NativeId id) {
    switch (id) {
    case NAT_PrintInt:    return "print_int";
    case NAT_PrintChar:   return "print_char";
    case NAT_PrintDouble: return "print_double";
    case NAT_PrintString: return "print_string";
    case NAT_PrintLine:   return "print_line";
    case NAT_Count:       break;
    }
    return "?";
}

int nativeArgCount(NativeId id) {
    return (id == NAT_PrintLine) ? 0 : 1;
}

// --- the object file ---------------------------------------------------
//
// Everything is written little-endian and fixed-width.  A variable-length
// encoding would make the file smaller and the reader harder to follow, and
// the file is not the interesting part of this compiler.

namespace {

void putU(std::string &out, unsigned long v, int bytes) {
    for (int i = 0; i < bytes; ++i) out += static_cast<char>((v >> (i * 8)) & 0xFF);
}

void putI64(std::string &out, long v) {
    putU(out, static_cast<unsigned long>(v), 8);
}

void putF64(std::string &out, double v) {
    // The bit pattern, so the value survives the trip exactly.
    unsigned char buf[8];
    std::memcpy(buf, &v, 8);
    for (int i = 0; i < 8; ++i) out += static_cast<char>(buf[i]);
}

void putStr(std::string &out, const std::string &s) {
    putU(out, static_cast<unsigned long>(s.size()), 4);
    out += s;
}

struct Reader {
    const std::string &data;
    std::size_t at;
    bool ok;
    Reader(const std::string &d) : data(d), at(0), ok(true) {}

    bool need(std::size_t n) {
        if (!ok || at + n > data.size()) { ok = false; return false; }
        return true;
    }
    unsigned long getU(int bytes) {
        if (!need(static_cast<std::size_t>(bytes))) return 0;
        unsigned long v = 0;
        for (int i = bytes - 1; i >= 0; --i) {
            v = (v << 8) | static_cast<unsigned char>(data[at + i]);
        }
        at += bytes;
        return v;
    }
    long getI64() { return static_cast<long>(getU(8)); }
    double getF64() {
        if (!need(8)) return 0.0;
        double v = 0.0;
        std::memcpy(&v, &data[at], 8);
        at += 8;
        return v;
    }
    std::string getStr() {
        const unsigned long n = getU(4);
        if (!need(n)) return std::string();
        const std::string s = data.substr(at, n);
        at += n;
        return s;
    }
};

} // namespace

bool Image::write(const std::string &path, std::string &error) const {
    std::string out;
    putU(out, Magic, 4);
    putU(out, Version, 4);
    putI64(out, entry);

    putU(out, static_cast<unsigned long>(staticData.size()), 4);
    for (std::size_t i = 0; i < staticData.size(); ++i) {
        out += static_cast<char>(staticData[i]);
    }

    putU(out, static_cast<unsigned long>(functions.size()), 4);
    for (std::size_t f = 0; f < functions.size(); ++f) {
        const FuncImage &fi = functions[f];
        putStr(out, fi.name);
        putI64(out, fi.paramCount);
        putI64(out, fi.frameSize);
        putI64(out, fi.registerCount);

        putU(out, static_cast<unsigned long>(fi.localOffset.size()), 4);
        for (std::size_t i = 0; i < fi.localOffset.size(); ++i) putI64(out, fi.localOffset[i]);
        putU(out, static_cast<unsigned long>(fi.localSize.size()), 4);
        for (std::size_t i = 0; i < fi.localSize.size(); ++i) putI64(out, fi.localSize[i]);
        putU(out, static_cast<unsigned long>(fi.localFloat.size()), 4);
        for (std::size_t i = 0; i < fi.localFloat.size(); ++i) putU(out, fi.localFloat[i], 1);

        putU(out, static_cast<unsigned long>(fi.code.size()), 4);
        for (std::size_t i = 0; i < fi.code.size(); ++i) {
            const Instr &n = fi.code[i];
            putU(out, static_cast<unsigned long>(n.op), 1);
            putI64(out, n.imm);
            putI64(out, n.b);
            putF64(out, n.fimm);
            putI64(out, n.line);
        }
    }

    std::ofstream file(path.c_str(), std::ios::binary);
    if (!file) { error = "cannot open '" + path + "' for writing"; return false; }
    file.write(out.data(), static_cast<std::streamsize>(out.size()));
    if (!file) { error = "failed while writing '" + path + "'"; return false; }
    return true;
}

bool Image::read(const std::string &path, std::string &error) {
    std::ifstream file(path.c_str(), std::ios::binary);
    if (!file) { error = "cannot open '" + path + "'"; return false; }
    std::ostringstream ss;
    ss << file.rdbuf();
    const std::string data = ss.str();

    Reader r(data);
    if (r.getU(4) != Magic) { error = "'" + path + "' is not a Compiler++ image"; return false; }
    const unsigned long ver = r.getU(4);
    if (ver != Version) {
        std::ostringstream m;
        m << "'" << path << "' is version " << ver << ", this build reads version " << Version;
        error = m.str();
        return false;
    }
    entry = static_cast<int>(r.getI64());

    const unsigned long dataLen = r.getU(4);
    staticData.clear();
    if (!r.need(dataLen)) { error = "'" + path + "' is truncated"; return false; }
    staticData.reserve(dataLen);
    for (unsigned long i = 0; i < dataLen; ++i) {
        staticData.push_back(static_cast<unsigned char>(data[r.at + i]));
    }
    r.at += dataLen;

    const unsigned long funcCount = r.getU(4);
    functions.clear();
    for (unsigned long f = 0; f < funcCount && r.ok; ++f) {
        FuncImage fi;
        fi.name = r.getStr();
        fi.paramCount = static_cast<int>(r.getI64());
        fi.frameSize = static_cast<int>(r.getI64());
        fi.registerCount = static_cast<int>(r.getI64());

        unsigned long n = r.getU(4);
        for (unsigned long i = 0; i < n && r.ok; ++i) fi.localOffset.push_back(static_cast<int>(r.getI64()));
        n = r.getU(4);
        for (unsigned long i = 0; i < n && r.ok; ++i) fi.localSize.push_back(static_cast<int>(r.getI64()));
        n = r.getU(4);
        for (unsigned long i = 0; i < n && r.ok; ++i)
            fi.localFloat.push_back(static_cast<unsigned char>(r.getU(1)));

        n = r.getU(4);
        for (unsigned long i = 0; i < n && r.ok; ++i) {
            Instr in;
            in.op = static_cast<OpCode>(r.getU(1));
            in.imm = r.getI64();
            in.b = r.getI64();
            in.fimm = r.getF64();
            in.line = static_cast<int>(r.getI64());
            fi.code.push_back(in);
        }
        functions.push_back(fi);
    }

    if (!r.ok) { error = "'" + path + "' is truncated or malformed"; return false; }
    return true;
}

void Image::disassemble() const {
    std::cout << "static data: " << staticData.size() << " bytes" << std::endl << std::endl;
    for (std::size_t f = 0; f < functions.size(); ++f) {
        const FuncImage &fi = functions[f];
        std::cout << "function " << f << "  " << fi.name
                  << "  params=" << fi.paramCount
                  << " frame=" << fi.frameSize
                  << " regs=" << fi.registerCount << std::endl;
        for (std::size_t i = 0; i < fi.code.size(); ++i) {
            const Instr &n = fi.code[i];
            std::cout << "  " << i << "\t" << opCodeName(n.op);
            switch (n.op) {
            case OP_PushFConst:
                std::cout << " " << n.fimm;
                break;
            case OP_Load:
            case OP_Store:
                std::cout << " :" << n.imm;
                if (n.b & 2)      std::cout << " float";
                else if (n.b & 1) std::cout << " signed";
                break;
            case OP_IntResize:
                std::cout << " :" << n.imm << (n.b ? " signed" : " unsigned");
                break;
            case OP_Call:
                std::cout << " " << n.imm << " (" << n.b << " args)";
                break;
            case OP_CallIndirect:
                std::cout << " (" << n.b << " args)";
                break;
            case OP_Native:
                std::cout << " " << nativeName(static_cast<NativeId>(n.imm));
                break;
            case OP_ReturnVoid:
            case OP_Pop:
            case OP_Halt:
                break;
            default:
                std::cout << " " << n.imm;
                break;
            }
            std::cout << std::endl;
        }
        std::cout << std::endl;
    }
}
