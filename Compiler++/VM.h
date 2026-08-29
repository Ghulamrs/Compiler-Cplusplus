// VM.h -- PASS 6c, running the bytecode.
//
// The last step, and the one that makes the rest checkable: the compiler runs
// what it produced, on the machine that built it.
//
// One flat byte memory holds static data, the frame stack and the heap, so an
// address is an address wherever it points.  Values on the operand stack are 8
// bytes and hold a long or a double; memory keeps each type at its declared
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
    long run(const Image &image, bool &ok);

    const std::string &errorMessage() const { return error; }
    long stepCount() const { return steps; }

private:
    // A value is 8 bytes either way; which half is live depends on the
    // instruction that produced it, exactly as in a real register file.
    union Value {
        long i;
        double d;
    };

    std::vector<unsigned char> mem;
    std::vector<Value> stack;       // the operand stack
    std::string error;
    long steps;

    long stackBase;                 // frames grow from here
    long stackTop;
    long heapBase;
    long heapTop;
    long freeList;                  // singly linked, through each block's header

    struct Frame {
        int func;
        int pc;
        long base;                  // byte offset of this frame in mem
        int regBase;                // where registers start within the frame
        bool wantsResult;
    };
    std::vector<Frame> frames;

    void trap(const std::string &msg);
    bool failed() const { return !error.empty(); }

    void push(long v);
    void pushD(double v);
    Value pop();

    long readInt(long addr, int size, bool isSigned);
    void writeInt(long addr, int size, long value);
    double readFloat(long addr, int size);
    void writeFloat(long addr, int size, double value);

    long allocate(long bytes);
    void release(long addr);

    void callNative(NativeId id, int argc);

    VM(const VM &);
    VM &operator=(const VM &);
};

#endif
