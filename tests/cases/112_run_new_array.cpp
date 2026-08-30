// new T[n] and delete[].
//
// The count is an expression, not a constant: the bound of a heap array is a
// value the program computes, which is the whole reason to want one over a
// declared array.  Elements are default-constructed, because there is nowhere
// to write arguments for each of them, and destroyed in reverse.
//
// delete[] finds the count in the block's header, where new[] wrote it.  The
// block's SIZE cannot stand in for the count: blocks are rounded up to a
// multiple of eight, so five four-byte elements would come back as six.

void print_int(int n);
void print_string(char *s);
void print_line();

int live = 0;
int nextId = 0;

class Counted {
public:
    int id;
    Counted()  { id = nextId; nextId = nextId + 1; live = live + 1; }
    ~Counted() { live = live - 1; }
};

// Twelve bytes, so the rounding the allocator does is visible: five of these
// occupy sixty bytes, which rounds to sixty-four.
class Wide {
public:
    int a;
    int b;
    int c;
    Wide()  { a = 1; b = 2; c = 3; live = live + 1; }
    ~Wide() { live = live - 1; }
};

class Noisy {
public:
    int id;
    Noisy()  { id = nextId; nextId = nextId + 1; }
    ~Noisy() { print_int(id); }
};

int main() {
    // Scalars: no constructor to run, just memory.
    char *s = new char[16];
    s[0] = 65;
    s[15] = 90;
    print_int(s[0]);
    print_int(s[15]);
    print_line();

    int *v = new int[4];
    for (int i = 0; i < 4; i = i + 1) v[i] = i * i;
    print_int(v[3]);
    print_line();
    delete[] v;
    delete[] s;

    // A literal bound.
    Counted *a = new Counted[8];
    print_int(live);
    print_line();
    delete[] a;
    print_int(live);
    print_line();

    // A bound the compiler does not know, and a size that does not divide the
    // rounded block.
    nextId = 0;
    int k = 5;
    Wide *w = new Wide[k];
    print_int(live);
    print_int(w[4].c);
    print_line();
    delete[] w;
    print_int(live);
    print_line();

    // Destroyed in reverse: the last built is the first torn down.
    nextId = 0;
    Noisy *n = new Noisy[4];
    delete[] n;
    print_line();

    // Zero elements is a real array: nothing is constructed, and delete[]
    // still has a block to release.
    Counted *none = new Counted[0];
    print_int(live);
    delete[] none;
    print_int(live);
    print_line();

    // delete[] of a null pointer, like delete, is harmless.
    Counted *nil = 0;
    delete[] nil;

    // The blocks come back: five hundred rounds in a fixed footprint.
    int total = 0;
    for (int i = 0; i < 500; i = i + 1) {
        int *t = new int[4];
        t[3] = i;
        total = total + t[3];
        delete[] t;
    }
    print_int(total);
    print_line();

    print_string("done");
    print_line();
    return 0;
}
