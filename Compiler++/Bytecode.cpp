// Bytecode.cpp
//
// C++98 only.

#include "Bytecode.h"

#include <cstddef>
#include <iostream>

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
                std::cout << " :" << n.imm << (n.b ? " signed" : "");
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
