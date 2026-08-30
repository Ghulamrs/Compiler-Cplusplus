// delete on a block that came from new[].
//
// The language leaves this undefined, because a real allocator has nowhere to
// record which form made the block -- the count would have to be a cookie in
// front of the payload, and free() cannot be asked about it.  This machine
// writes the form into the block's own header, so the mismatch is an error
// with a name rather than a corrupted heap.

void print_string(char *s);
void print_line();

class C {
public:
    int n;
    C()  { n = 1; }
    ~C() { }
};

int main() {
    C *a = new C[4];
    print_string("allocated");
    print_line();

    delete a;                       // should be delete[]

    print_string("unreachable");
    print_line();
    return 0;
}
