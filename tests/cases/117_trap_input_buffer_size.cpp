// `cin >> s` gives no width, so something has to say how long the buffer is.
//
// Two things can.  A buffer from new[] carries its length in the block header
// the allocator wrote.  A declared LOCAL has decayed to a pointer by the time
// it reaches the native and its type is gone -- but the slot it points into is
// still described by the frame table of the function that declared it, and the
// machine has that table for every frame it has pushed.  So both are answered
// without a cookie and without a width at the call.
//
// A global is the one that is not: static data is a flat run of bytes with no
// table describing what lives where.  That read is refused rather than run
// past the end of whatever it landed in.
//
// Input: tests/input/117_trap_input_buffer_size.txt
#include <iostream>

char shared[8];                          // a global: no table describes it

int main() {
    char *sized = new char[8];
    cin >> sized;
    cout << "[" << sized << "]" << endl;     // truncated to fit, not overflowed
    delete[] sized;

    char local[8];
    cin >> local;
    cout << "[" << local << "]" << endl;     // the frame table knows this one

    cin >> shared;                           // refused: nothing knows its size
    cout << "unreachable" << endl;
    return 0;
}
