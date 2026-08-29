// VM.cpp
//
// C++98 only.

#include "VM.h"

#include <cstddef>
#include <cstring>
#include <iostream>
#include <sstream>

namespace {
const long MemorySize   = 4L * 1024 * 1024;
const long StackSize    = 1L * 1024 * 1024;
const long MaxSteps     = 50L * 1000 * 1000;   // a runaway program stops itself
const int  HeaderSize   = 16;                  // [size][next] before each block
}

VM::VM()
    : steps(0), stackBase(0), stackTop(0), heapBase(0), heapTop(0), freeList(0) {}

void VM::trap(const std::string &msg) {
    if (error.empty()) error = msg;
}

void VM::push(long v)    { Value x; x.i = v; stack.push_back(x); }
void VM::pushD(double v) { Value x; x.d = v; stack.push_back(x); }

VM::Value VM::pop() {
    if (stack.empty()) {
        trap("operand stack underflow");
        Value z; z.i = 0; return z;
    }
    Value v = stack.back();
    stack.pop_back();
    return v;
}

// --- memory ---

long VM::readInt(long addr, int size, bool isSigned) {
    if (addr <= 0 || addr + size > static_cast<long>(mem.size())) {
        std::ostringstream ss;
        ss << "read of " << size << " bytes at invalid address " << addr;
        trap(ss.str());
        return 0;
    }
    unsigned long raw = 0;
    for (int i = size - 1; i >= 0; --i) {
        raw = (raw << 8) | mem[addr + i];
    }
    if (isSigned && size < 8) {
        const unsigned long signBit = 1UL << (size * 8 - 1);
        if (raw & signBit) raw |= ~((1UL << (size * 8)) - 1);
    }
    return static_cast<long>(raw);
}

void VM::writeInt(long addr, int size, long value) {
    if (addr <= 0 || addr + size > static_cast<long>(mem.size())) {
        std::ostringstream ss;
        ss << "write of " << size << " bytes at invalid address " << addr;
        trap(ss.str());
        return;
    }
    unsigned long raw = static_cast<unsigned long>(value);
    for (int i = 0; i < size; ++i) {
        mem[addr + i] = static_cast<unsigned char>((raw >> (i * 8)) & 0xFF);
    }
}

double VM::readFloat(long addr, int size) {
    if (addr <= 0 || addr + size > static_cast<long>(mem.size())) {
        trap("floating read at an invalid address");
        return 0.0;
    }
    if (size == 4) {
        float f;
        std::memcpy(&f, &mem[addr], 4);
        return f;
    }
    double d;
    std::memcpy(&d, &mem[addr], 8);
    return d;
}

void VM::writeFloat(long addr, int size, double value) {
    if (addr <= 0 || addr + size > static_cast<long>(mem.size())) {
        trap("floating write at an invalid address");
        return;
    }
    if (size == 4) {
        float f = static_cast<float>(value);
        std::memcpy(&mem[addr], &f, 4);
        return;
    }
    std::memcpy(&mem[addr], &value, 8);
}

// A block carries its size, so `delete` knows how much it is releasing, and a
// next pointer, so a released block can be reused.  First fit, because the
// simplest allocator that reuses memory is enough to run a loop.
long VM::allocate(long bytes) {
    if (bytes <= 0) bytes = 1;
    if (bytes % 8) bytes += 8 - (bytes % 8);

    long prev = 0;
    for (long b = freeList; b != 0; ) {
        const long size = readInt(b, 8, true);
        const long next = readInt(b + 8, 8, true);
        if (size >= bytes) {
            if (prev) writeInt(prev + 8, 8, next);
            else      freeList = next;
            return b + HeaderSize;
        }
        prev = b;
        b = next;
    }

    if (heapTop + HeaderSize + bytes > static_cast<long>(mem.size())) {
        trap("out of heap memory");
        return 0;
    }
    const long block = heapTop;
    heapTop += HeaderSize + bytes;
    writeInt(block, 8, bytes);
    writeInt(block + 8, 8, 0);
    return block + HeaderSize;
}

void VM::release(long addr) {
    if (addr == 0) return;                      // deleting null is harmless
    const long block = addr - HeaderSize;
    if (block < heapBase || block >= heapTop) {
        trap("delete of a pointer that did not come from new");
        return;
    }
    writeInt(block + 8, 8, freeList);
    freeList = block;
}

// --- natives ---

void VM::callNative(NativeId id, int argc) {
    // Every argument is popped, not just the one that gets used: leaving the
    // rest behind would desync the operand stack for everything after.  The
    // first is the one the natives take.
    Value a;
    a.i = 0;
    for (int k = argc; k > 0; --k) {
        const Value v = pop();
        if (k == 1) a = v;
    }
    switch (id) {
    case NAT_PrintInt:    std::cout << a.i; break;
    case NAT_PrintChar:   std::cout << static_cast<char>(a.i); break;
    case NAT_PrintDouble: std::cout << a.d; break;
    case NAT_PrintLine:   std::cout << std::endl; break;
    case NAT_PrintString: {
        long p = a.i;
        while (p > 0 && p < static_cast<long>(mem.size()) && mem[p]) {
            std::cout << static_cast<char>(mem[p]);
            ++p;
        }
        break;
    }
    case NAT_Count: break;
    }
    push(0);                                    // every call yields a value
}

// --- the loop ---

long VM::run(const Image &image, bool &ok) {
    ok = false;
    error.clear();
    steps = 0;

    if (image.entry < 0) { trap("no entry point"); return 0; }
    // A .cxb may have come from anywhere, so nothing in it is trusted: an
    // out-of-range entry, or a function with no register base, would index
    // straight past the tables it arrived with.
    if (image.entry >= static_cast<int>(image.functions.size())) {
        trap("entry point is not a function in this image");
        return 0;
    }
    for (std::size_t i = 0; i < image.functions.size(); ++i) {
        if (image.functions[i].localOffset.empty()) {
            trap("function '" + image.functions[i].name + "' has no frame layout");
            return 0;
        }
    }

    mem.assign(MemorySize, 0);
    std::copy(image.staticData.begin(), image.staticData.end(), mem.begin());

    stackBase = static_cast<long>(image.staticData.size());
    if (stackBase % 8) stackBase += 8 - (stackBase % 8);
    stackTop = stackBase;
    heapBase = stackBase + StackSize;
    heapTop = heapBase;
    freeList = 0;

    // The entry frame.
    Frame top;
    top.func = image.entry;
    top.pc = 0;
    top.base = stackTop;
    top.regBase = image.functions[image.entry].localOffset.back();
    top.wantsResult = true;
    stackTop += image.functions[image.entry].frameSize;
    frames.push_back(top);

    long result = 0;

    while (!frames.empty() && !failed()) {
        if (++steps > MaxSteps) { trap("execution did not terminate"); break; }

        Frame &fr = frames.back();
        const FuncImage &fi = image.functions[fr.func];
        if (fr.pc < 0 || fr.pc >= static_cast<int>(fi.code.size())) {
            trap("program counter left the function");
            break;
        }
        const Instr &in = fi.code[fr.pc++];

        switch (in.op) {
        case OP_PushConst:  push(in.imm); break;
        case OP_PushFConst: pushD(in.fimm); break;
        case OP_Pop:        pop(); break;

        case OP_LoadReg:
            push(readInt(fr.base + fr.regBase + in.imm * 8, 8, true));
            break;
        case OP_StoreReg:
            writeInt(fr.base + fr.regBase + in.imm * 8, 8, pop().i);
            break;

        case OP_LocalAddr:
            if (in.imm < 0 || in.imm >= static_cast<long>(fi.localOffset.size()) - 1) {
                trap("local slot out of range");
                break;
            }
            push(fr.base + fi.localOffset[in.imm]);
            break;
        case OP_StaticAddr: push(in.imm); break;
        case OP_FuncAddr:   push(in.imm); break;
        case OP_FieldAddr:  push(pop().i + in.imm); break;

        case OP_Load: {
            const long addr = pop().i;
            if (in.b & 2) pushD(readFloat(addr, static_cast<int>(in.imm)));
            else          push(readInt(addr, static_cast<int>(in.imm), (in.b & 1) != 0));
            break;
        }
        case OP_Store: {
            const Value v = pop();
            const long addr = pop().i;
            if (in.b & 2) writeFloat(addr, static_cast<int>(in.imm), v.d);
            else          writeInt(addr, static_cast<int>(in.imm), v.i);
            break;
        }

        case OP_MemCopy: {
            const long src = pop().i;
            const long dst = pop().i;
            const long n   = in.imm;
            if (src <= 0 || dst <= 0 || n < 0 ||
                src + n > static_cast<long>(mem.size()) ||
                dst + n > static_cast<long>(mem.size())) {
                trap("object copy at an invalid address");
                break;
            }
            if (src != dst) std::memmove(&mem[dst], &mem[src], static_cast<std::size_t>(n));
            break;
        }

        case OP_Add: { long b = pop().i, a = pop().i; push(a + b); break; }
        case OP_Sub: { long b = pop().i, a = pop().i; push(a - b); break; }
        case OP_Mul: { long b = pop().i, a = pop().i; push(a * b); break; }
        case OP_Div: {
            long b = pop().i, a = pop().i;
            if (b == 0) { trap("division by zero"); break; }
            push(a / b);
            break;
        }
        case OP_Mod: {
            long b = pop().i, a = pop().i;
            if (b == 0) { trap("remainder by zero"); break; }
            push(a % b);
            break;
        }
        case OP_UDiv: {
            unsigned long b = static_cast<unsigned long>(pop().i);
            unsigned long a = static_cast<unsigned long>(pop().i);
            if (b == 0) { trap("division by zero"); break; }
            push(static_cast<long>(a / b));
            break;
        }
        case OP_UMod: {
            unsigned long b = static_cast<unsigned long>(pop().i);
            unsigned long a = static_cast<unsigned long>(pop().i);
            if (b == 0) { trap("remainder by zero"); break; }
            push(static_cast<long>(a % b));
            break;
        }
        case OP_Neg: push(-pop().i); break;
        case OP_Not: push(pop().i == 0 ? 1 : 0); break;

        case OP_FAdd: { double b = pop().d, a = pop().d; pushD(a + b); break; }
        case OP_FSub: { double b = pop().d, a = pop().d; pushD(a - b); break; }
        case OP_FMul: { double b = pop().d, a = pop().d; pushD(a * b); break; }
        case OP_FDiv: { double b = pop().d, a = pop().d; pushD(a / b); break; }
        case OP_FNeg: pushD(-pop().d); break;

        case OP_CmpEQ: { long b = pop().i, a = pop().i; push(a == b); break; }
        case OP_CmpNE: { long b = pop().i, a = pop().i; push(a != b); break; }
        case OP_CmpLT: { long b = pop().i, a = pop().i; push(a <  b); break; }
        case OP_CmpGT: { long b = pop().i, a = pop().i; push(a >  b); break; }
        case OP_CmpLE: { long b = pop().i, a = pop().i; push(a <= b); break; }
        case OP_CmpGE: { long b = pop().i, a = pop().i; push(a >= b); break; }
        case OP_UCmpLT: { unsigned long b = pop().i, a = pop().i; push(a <  b); break; }
        case OP_UCmpGT: { unsigned long b = pop().i, a = pop().i; push(a >  b); break; }
        case OP_UCmpLE: { unsigned long b = pop().i, a = pop().i; push(a <= b); break; }
        case OP_UCmpGE: { unsigned long b = pop().i, a = pop().i; push(a >= b); break; }
        case OP_FCmpEQ: { double b = pop().d, a = pop().d; push(a == b); break; }
        case OP_FCmpNE: { double b = pop().d, a = pop().d; push(a != b); break; }
        case OP_FCmpLT: { double b = pop().d, a = pop().d; push(a <  b); break; }
        case OP_FCmpGT: { double b = pop().d, a = pop().d; push(a >  b); break; }
        case OP_FCmpLE: { double b = pop().d, a = pop().d; push(a <= b); break; }
        case OP_FCmpGE: { double b = pop().d, a = pop().d; push(a >= b); break; }

        case OP_IntToFloat: {
            const long v = pop().i;
            pushD(in.imm ? static_cast<double>(static_cast<unsigned long>(v))
                         : static_cast<double>(v));
            break;
        }
        case OP_FloatToInt: push(static_cast<long>(pop().d)); break;
        case OP_FloatResize: {
            const double v = pop().d;
            pushD(in.imm == 4 ? static_cast<double>(static_cast<float>(v)) : v);
            break;
        }
        case OP_IntResize: {
            const long v = pop().i;
            const int size = static_cast<int>(in.imm);
            if (size >= 8) { push(v); break; }
            unsigned long masked = static_cast<unsigned long>(v) & ((1UL << (size * 8)) - 1);
            if (in.b) {
                const unsigned long signBit = 1UL << (size * 8 - 1);
                if (masked & signBit) masked |= ~((1UL << (size * 8)) - 1);
            }
            push(static_cast<long>(masked));
            break;
        }

        case OP_Jump:       fr.pc = static_cast<int>(in.imm); break;
        case OP_BranchZero: if (pop().i == 0) fr.pc = static_cast<int>(in.imm); break;
        case OP_BranchNZ:   if (pop().i != 0) fr.pc = static_cast<int>(in.imm); break;

        case OP_VTableLoad: {
            const long obj = pop().i;
            const long vtable = readInt(obj, 8, true);
            push(readInt(vtable + in.imm * 8, 8, true));
            break;
        }

        case OP_Native: callNative(static_cast<NativeId>(in.imm),
                                   static_cast<int>(in.b)); break;

        case OP_Call:
        case OP_CallIndirect: {
            const int argc = static_cast<int>(in.b);
            long target = in.imm;
            std::vector<Value> args(argc);
            for (int k = argc - 1; k >= 0; --k) args[k] = pop();
            if (in.op == OP_CallIndirect) target = pop().i;

            if (target < 0 || target >= static_cast<long>(image.functions.size())) {
                trap("call to an undefined function");
                break;
            }
            const FuncImage &callee = image.functions[target];
            if (stackTop + callee.frameSize > heapBase) {
                trap("call stack overflow (runaway recursion?)");
                break;
            }
            Frame nf;
            nf.func = static_cast<int>(target);
            nf.pc = 0;
            nf.base = stackTop;
            nf.regBase = callee.localOffset.back();
            nf.wantsResult = true;
            stackTop += callee.frameSize;
            // Arguments land in the first local slots, which is exactly where
            // the parameters were declared.
            for (int k = 0; k < argc && k < static_cast<int>(callee.localSize.size()); ++k) {
                // A float parameter is narrowed here; the operand stack always
                // carries a double, and the slot may be four bytes wide.
                const bool flt = k < static_cast<int>(callee.localFloat.size())
                                 && callee.localFloat[k] != 0;
                if (flt) writeFloat(nf.base + callee.localOffset[k], callee.localSize[k], args[k].d);
                else     writeInt(nf.base + callee.localOffset[k], callee.localSize[k], args[k].i);
            }
            frames.push_back(nf);
            break;
        }

        case OP_Return:
        case OP_ReturnVoid: {
            Value rv;
            rv.i = 0;
            if (in.op == OP_Return) rv = pop();
            stackTop = fr.base;
            frames.pop_back();
            if (frames.empty()) { result = rv.i; ok = !failed(); return result; }
            stack.push_back(rv);
            break;
        }

        case OP_Alloc: push(allocate(in.imm)); break;
        case OP_Free:  release(pop().i); break;
        case OP_Halt:  frames.clear(); break;
        }
    }

    ok = !failed();
    return result;
}
