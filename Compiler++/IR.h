// IR.h
//
// PASS 5a -- the intermediate representation.
//
// ============================ WHY THERE IS NO IR1 =========================
//
//   Every other part of this compiler comes in two layers, cc:: and cxx::,
//   because C++ adds things to C that C has no way to say.  The IR has ONE
//   layer, and that is the whole point of it.
//
//   Lowering's entire job is to ERASE the C++ layer.  A method becomes a
//   function whose first parameter is `this`.  A reference becomes a pointer.
//   A field access becomes an address plus a constant offset.  A virtual call
//   becomes: load the vptr, index it by a constant, call through the result.
//   `new` becomes allocate-then-call, `delete` becomes call-then-free.  By the
//   time anything reaches this file, there are no classes, no references, no
//   inheritance and no dispatch left -- only memory, arithmetic and calls.
//
//   So the two lowering PASSES are layered in the usual way (Lower.h holds
//   cc::Lowering, Lower1.h holds cxx::Lowering deriving from it), but they
//   both target this one C-level instruction set.  "C++ is C plus X" and
//   "code generation for C++ is code generation for C, after X is erased" are
//   the same sentence, and this file is where that sentence lands.
//
// ==========================================================================
//
//   Shape of the IR: a flat list of three-address instructions per function,
//   over an unlimited supply of virtual registers, with labels and jumps
//   rather than explicit basic blocks.  Flat and linear is easier to read than
//   a block graph, and it lowers directly to either of the back ends under
//   consideration -- a stack VM, or emitted C.
//
// C++98 only.

#ifndef IR_H
#define IR_H

#include <cstddef>
#include <string>
#include <vector>

#include "AST.h"

// A virtual register.  There is no register allocation here; that is the code
// generator's problem, and a stack VM does not even have it.
typedef int IRReg;
const IRReg IR_NoReg = -1;

enum IROp {
    // --- values -------------------------------------------------------
    IR_Const,        // dest = imm
    IR_Move,         // dest = a

    // --- arithmetic ---------------------------------------------------
    IR_Add, IR_Sub, IR_Mul, IR_Div, IR_Mod,
    IR_Neg,          // dest = -a
    IR_LogicalNot,   // dest = (a == 0)

    // --- comparison (all yield 0 or 1) --------------------------------
    IR_CmpEQ, IR_CmpNE, IR_CmpLT, IR_CmpGT, IR_CmpLE, IR_CmpGE,

    // --- addresses ----------------------------------------------------
    IR_LocalAddr,    // dest = address of local slot imm
    IR_GlobalAddr,   // dest = address of global `sym`
    IR_FieldAddr,    // dest = a + imm        -- a field at a constant offset
    IR_FuncAddr,     // dest = address of function `sym`

    // --- memory -------------------------------------------------------
    IR_Load,         // dest = *a             (imm = size in bytes)
    IR_Store,        // *a = b                (imm = size in bytes)

    // --- calls --------------------------------------------------------
    IR_Call,         // dest = sym(args...)
    IR_CallIndirect, // dest = (*a)(args...)  -- this is a virtual call

    // --- dispatch -----------------------------------------------------
    // a = the object's address; imm = the vtable slot.  Kept as one opcode
    // rather than three loads because naming the operation is what makes a
    // dump readable -- the code generator expands it.
    IR_VCallTarget,  // dest = (*(vptr of a))[imm]

    // --- free store ---------------------------------------------------
    IR_Alloc,        // dest = allocate imm bytes
    IR_Free,         // release a

    // --- control ------------------------------------------------------
    IR_Label,        // imm = label id
    IR_Jump,         // goto imm
    IR_BranchZero,   // if a == 0 goto imm
    IR_BranchNZ,     // if a != 0 goto imm
    IR_Return        // return a, or return nothing when a is IR_NoReg
};

const char *irOpName(IROp op);

struct IRInstr {
    IROp op;
    IRReg dest;
    IRReg a;
    IRReg b;
    long imm;
    std::string sym;            // callee or global name
    std::vector<IRReg> args;    // for IR_Call / IR_CallIndirect
    int line;                   // source line, so a dump can be traced back

    IRInstr(IROp o)
        : op(o), dest(IR_NoReg), a(IR_NoReg), b(IR_NoReg), imm(0), line(0) {}
};

// One named slot in a function's frame.  Class-typed locals occupy their whole
// object size here, which is what makes `Point p;` a real object rather than a
// pointer to one.
struct IRLocal {
    std::string name;
    int slot;
    int size;
    bool isParam;
    IRLocal(const std::string &n, int s, int sz, bool p)
        : name(n), slot(s), size(sz), isParam(p) {}
};

struct IRFunction {
    std::string name;           // mangled:  Point__getX
    std::string sourceName;     // as written:  Point::getX
    int paramCount;             // includes `this` where there is one
    bool returnsValue;
    std::vector<IRLocal> locals;
    std::vector<IRInstr> code;

    IRFunction(const std::string &mangled, const std::string &source)
        : name(mangled), sourceName(source), paramCount(0),
          returnsValue(false), nextReg(0), nextLabel(0) {}

    IRReg newReg() { return nextReg++; }
    int newLabel() { return nextLabel++; }
    int addLocal(const std::string &n, int size, bool isParam);

    // The builder.  Every emit returns the instruction's destination register
    // where it has one, so expressions compose without a temporary variable
    // at every step.
    IRReg emitConst(long value, int line);
    IRReg emitUnary(IROp op, IRReg a, int line);
    IRReg emitBinary(IROp op, IRReg a, IRReg b, int line);
    IRReg emitLocalAddr(int slot, int line);
    IRReg emitGlobalAddr(const std::string &sym, int line);
    IRReg emitFieldAddr(IRReg base, long offset, int line);
    IRReg emitFuncAddr(const std::string &sym, int line);
    IRReg emitLoad(IRReg addr, int size, int line);
    void  emitStore(IRReg addr, IRReg value, int size, int line);
    IRReg emitCall(const std::string &sym, const std::vector<IRReg> &args,
                   bool wantsResult, int line);
    IRReg emitCallIndirect(IRReg target, const std::vector<IRReg> &args,
                           bool wantsResult, int line);
    IRReg emitVCallTarget(IRReg object, long slot, int line);
    IRReg emitAlloc(long bytes, int line);
    void  emitFree(IRReg ptr, int line);
    void  emitLabel(int label);
    void  emitJump(int label, int line);
    void  emitBranchZero(IRReg cond, int label, int line);
    void  emitBranchNZ(IRReg cond, int label, int line);
    void  emitReturn(IRReg value, int line);

    int registerCount() const { return nextReg; }
    // Does control already leave here?  Anything emitted after a return or an
    // unconditional jump, up to the next label, can never run -- so lowering
    // asks before emitting a function's trailing code.
    bool endsWithTerminator() const;

private:
    int nextReg;
    int nextLabel;
    void push(const IRInstr &i) { code.push_back(i); }
};

struct IRGlobal {
    std::string name;
    int size;
    IRGlobal(const std::string &n, int s) : name(n), size(s) {}
};

// A vtable is data, not code: an array of function addresses, one per slot.
// Slot indices come straight from Layout, which is why they mean the same
// thing in a base and in everything derived from it.
struct IRVTable {
    std::string className;
    std::vector<std::string> slots;     // mangled function names
};

struct IRModule {
    std::vector<IRFunction*> functions;
    std::vector<IRGlobal> globals;
    std::vector<IRVTable> vtables;

    ~IRModule();
    void print() const;

private:
    static void printInstr(const IRInstr &i);
};

// Name mangling.  One flat namespace of symbols is all a linker or a VM wants,
// so a class member's owning class is folded into its name.
std::string mangleFunction(const std::string &className, const std::string &name);
std::string mangleConstructor(const std::string &className, std::size_t argCount);
std::string mangleDestructor(const std::string &className);
std::string mangleVTable(const std::string &className);

#endif
