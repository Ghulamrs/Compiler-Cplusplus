// Bytecode.h -- PASS 6a, the instruction set the compiler targets.
//
// A stack machine, chosen so the compiler can RUN what it produces on the
// machine that built it.  Emitting native code for two ABIs would be more
// authentic and less useful: nothing about a calling convention teaches
// anything about C++, and cross-compiled output cannot be watched.
//
// One flat byte memory holds everything, so an address is an address whatever
// it points at -- a global, a string, a local, or the heap:
//
//     [0 .. 8)          reserved, so the null pointer is address 0
//     [static data]     globals, vtables, string literals
//     [frame stack]     one frame per active call
//     [heap]            new and delete
//
// Values on the operand stack are 8 bytes and hold either a long or a double;
// memory keeps each type at its declared width, so LOAD and STORE carry a size.
//
// C++98 only.

#ifndef BYTECODE_H
#define BYTECODE_H

#include <string>
#include <vector>

enum OpCode {
    // --- operand stack ---
    OP_PushConst,       // push imm
    OP_PushFConst,      // push fimm
    OP_LoadReg,         // push the value of frame register imm
    OP_StoreReg,        // pop into frame register imm
    OP_Pop,

    // --- addresses ---
    OP_LocalAddr,       // push the address of local slot imm
    OP_StaticAddr,      // push imm, an absolute address in static data
    OP_FieldAddr,       // pop a, push a + imm
    OP_FuncAddr,        // push imm, a function index

    // --- memory (imm = width in bytes; `b` = 1 when sign-extending) ---
    OP_Load,
    OP_Store,
    OP_MemCopy,     // imm = byte count; pops src then dst

    // --- integer arithmetic ---
    OP_Add, OP_Sub, OP_Mul, OP_Div, OP_Mod, OP_UDiv, OP_UMod,
    OP_Shl, OP_Shr, OP_UShr,
    OP_Neg, OP_Not,

    // --- floating arithmetic ---
    OP_FAdd, OP_FSub, OP_FMul, OP_FDiv, OP_FNeg,

    // --- comparison ---
    OP_CmpEQ, OP_CmpNE, OP_CmpLT, OP_CmpGT, OP_CmpLE, OP_CmpGE,
    OP_UCmpLT, OP_UCmpGT, OP_UCmpLE, OP_UCmpGE,
    OP_FCmpEQ, OP_FCmpNE, OP_FCmpLT, OP_FCmpGT, OP_FCmpLE, OP_FCmpGE,

    // --- conversions ---
    OP_IntToFloat,      // imm = 1 when the source is unsigned
    OP_FloatToInt,
    OP_FloatResize,     // imm = target width
    OP_IntResize,       // imm = target width, b = 1 when sign-extending

    // --- control ---
    OP_Jump,            // goto imm
    OP_BranchZero,      // pop; goto imm if zero
    OP_BranchNZ,
    OP_Call,            // imm = function index, b = argument count
    OP_CallIndirect,    // pop a function index, b = argument count
    OP_VTableLoad,      // pop an object address, push its vtable's slot imm
    OP_Native,          // imm = native index, b = argument count
    OP_Return,          // pop a value and return it
    OP_ReturnVoid,

    // --- free store ---
    OP_Alloc,           // push the address of imm fresh bytes
    OP_Free,            // pop an address and release it

    OP_Halt
};

const char *opCodeName(OpCode op);

struct Instr {
    OpCode op;
    long imm;
    long b;
    double fimm;
    int line;
    Instr(OpCode o = OP_Halt) : op(o), imm(0), b(0), fimm(0.0), line(0) {}
};

// One frame's shape.  Locals keep their declared widths; every virtual
// register is 8 bytes, because a register holds a long or a double and nothing
// smaller is worth the arithmetic.
struct FuncImage {
    std::string name;
    int paramCount;
    int frameSize;                  // bytes of locals
    int registerCount;
    std::vector<int> localOffset;   // slot -> byte offset within the frame
    std::vector<int> localSize;
    // Parallel to localSize: 1 when the slot holds a float or double, so the
    // VM writes an incoming argument with the right representation.
    std::vector<unsigned char> localFloat;
    // 1 when the slot is a by-value object: the argument is an address and the
    // VM copies localSize bytes from it.
    std::vector<unsigned char> localObject;
    std::vector<Instr> code;
    FuncImage() : paramCount(0), frameSize(0), registerCount(0) {}
};

// Everything the VM needs to run: the code, and the bytes that exist before it
// starts.
struct Image {
    std::vector<FuncImage> functions;
    std::vector<unsigned char> staticData;
    int entry;                      // index of main, or -1
    Image() : entry(-1) {}

    void disassemble() const;

    // --- the object file -------------------------------------------------
    // A compiled program has to outlive the process that made it, so the
    // image is written whole to a .cxb file: a magic word, a version, the
    // static data, then every function.  Little-endian and fixed-width
    // throughout, so a file written on one machine loads on another.
    bool write(const std::string &path, std::string &error) const;
    bool read(const std::string &path, std::string &error);

    static const unsigned long Magic   = 0x31425843UL;  // "CXB1"
    static const unsigned long Version = 3;   // v2 localFloat, v3 localObject
};

// Functions the VM supplies.  A program gets them by DECLARING one without a
// body -- there is no header file to include, so the declaration is the
// binding.  Without these a compiled program could compute but never show
// anything, which would make running it pointless.
enum NativeId {
    NAT_PrintInt,
    NAT_PrintChar,
    NAT_PrintDouble,
    NAT_PrintString,
    NAT_PrintLine,
    // Maths.  There is no <cmath> to include, so these are declared the same
    // way everything else is: `double sqrt(double);` with no body.
    NAT_Sqrt, NAT_Sin, NAT_Cos, NAT_Tan,
    NAT_Asin, NAT_Acos, NAT_Atan, NAT_Atan2,
    NAT_Pow, NAT_Fabs, NAT_Floor, NAT_Ceil,
    NAT_Log, NAT_Log10, NAT_Exp,
    NAT_Abs,                    // the one that is integer in and integer out
    NAT_Count
};

// Returns NAT_Count when the name is not a native.
NativeId nativeByName(const std::string &name);
const char *nativeName(NativeId id);
int nativeArgCount(NativeId id);
// True when the native hands back a double rather than an integer.
bool nativeReturnsFloat(NativeId id);
// The most any native takes; the VM keeps that many argument slots.
const int NativeMaxArgs = 2;

#endif
