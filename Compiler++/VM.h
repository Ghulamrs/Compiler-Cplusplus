// VM.h -- PASS 6c, running the bytecode.
//
// The last step, and the one that makes the rest checkable: the compiler runs
// what it produced, on the machine that built it.
//
// One flat byte memory holds static data, the frame stack and the heap, so an
// address is an address wherever it points.  Values on the operand stack are 8
// bytes and hold a vmword or a double; memory keeps each type at its declared
// width, which is why loads and stores carry a size and a signedness.
//
// C++98 only.

#ifndef VM_H
#define VM_H

#include <string>
#include <vector>

#include "Bytecode.h"

class VM {
public:
    VM();

    // Runs main and returns its result.  `ok` reports whether execution
    // finished rather than trapping.
    vmword run(const Image &image, bool &ok);

    const std::string &errorMessage() const { return error; }
    vmword stepCount() const { return steps; }

private:
    // A value is 8 bytes either way; which half is live depends on the
    // instruction that produced it, exactly as in a real register file.
    union Value {
        vmword i;
        double d;
    };

    std::vector<unsigned char> mem;
    std::vector<Value> stack;       // the operand stack
    std::string error;
    vmword steps;

    vmword stackBase;               // frames grow from here
    vmword stackTop;
    vmword heapBase;
    vmword heapTop;
    vmword freeList;                // singly linked, through each block's header

    struct Frame {
        int func;
        int pc;
        vmword base;                // byte offset of this frame in mem
        int regBase;                // where registers start within the frame
        bool wantsResult;
    };
    std::vector<Frame> frames;

    void trap(const std::string &msg);
    bool failed() const { return !error.empty(); }

    void push(vmword v);
    void pushD(double v);
    Value pop();

    vmword readInt(vmword addr, int size, bool isSigned);
    void writeInt(vmword addr, int size, vmword value);
    double readFloat(vmword addr, int size);
    void writeFloat(vmword addr, int size, double value);

    // `arrayCount` is -1 for plain new and the element count for new[].  It is
    // recorded in the block, so delete[] knows how many destructors to run and
    // the matching form can be required rather than assumed.
    vmword allocate(vmword bytes, vmword arrayCount);
    void release(vmword addr, bool isArray);
    // Is this the START of a block, as opposed to somewhere inside one?
    bool isBlockStart(vmword block);
    // Is it on the free list already?  Both forms of delete need to say so.
    bool isOnFreeList(vmword block);
    // How many elements the new[] block at `addr` holds, or 0 with a trap set.
    vmword arrayCount(vmword addr);
    // The longest the free list could legitimately be; a walk past it is
    // going round a cycle, so both walks stop instead of spinning.
    vmword freeListLimit() const;

    void callNative(NativeId id, int argc);

    VM(const VM &);
    VM &operator=(const VM &);
};

#endif
