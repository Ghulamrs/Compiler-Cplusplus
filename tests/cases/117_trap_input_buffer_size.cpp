// `cin >> s` gives no width, so the buffer has to say how long it is.
//
// A buffer from new[] does: the allocator wrote its length in the block's
// header, and the machine asks.  A declared array does not -- the pointer that
// arrives has decayed and nothing knows what it points into -- so the read is
// refused rather than run past the end.  That is the same answer this machine
// gives everywhere else it is handed a length it cannot check.
//
// Input: tests/input/117_trap_input_buffer_size.txt
#include <iostream>

int main() {
    char *sized = new char[8];
    cin >> sized;
    cout << "[" << sized << "]" << endl;    // truncated to fit, not overflowed
    delete[] sized;

    char plain[32];
    cin >> plain;                            // refused: nothing knows its size
    cout << "unreachable" << endl;
    return 0;
}
