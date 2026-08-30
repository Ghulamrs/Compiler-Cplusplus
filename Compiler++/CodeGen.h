// CodeGen.h -- PASS 6b, IR to bytecode.
//
// The translation is nearly mechanical, because the IR has already done the
// hard part.  A three-address instruction becomes push-push-operate-pop:
//
//     %3 = add %1, %2      ->      ldr 1 / ldr 2 / add / str 3
//
// Every virtual register gets a frame slot rather than a real register.  A
// stack machine has no registers to allocate, and spilling everything keeps
// the bytecode readable next to the IR it came from -- which is the point of
// building a VM rather than emitting assembly.
//
// This pass also lays out the static data: globals, string literals, and the
// vtables, which become arrays of function indices.
//
// C++98 only.

#ifndef CODEGEN_H
#define CODEGEN_H

#include <map>
#include <string>
#include <vector>

#include "Bytecode.h"
#include "Diagnostics.h"
#include "IR.h"
#include "Layout.h"

class CodeGen {
public:
    CodeGen(Diagnostics &diag);
    void generate(const IRModule &module, Image &out);

private:
    Diagnostics &diag;

    // Symbol -> index, resolved in a first pass so a call may precede its
    // definition.
    std::map<std::string, int> functionIndex;
    std::map<std::string, NativeId> natives;
    std::map<std::string, long> staticAddress;   // globals, strings, vtables

    void collectSymbols(const IRModule &module, Image &out);
    void layoutStaticData(const IRModule &module, Image &out);
    void generateFunction(const IRFunction &fn, FuncImage &out);

    // IR labels are ids; bytecode branches are instruction offsets.
    void resolveLabels(const IRFunction &fn, FuncImage &out,
                       const std::map<vmword, int> &labelAt);

    CodeGen(const CodeGen &);
    CodeGen &operator=(const CodeGen &);
};

#endif
