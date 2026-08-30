// bool& and const bool.
//
// bool lives in the C++ layer, so cxx::Parser::parseType claims it before the
// C layer's specifier soup ever sees it.  That branch returned as soon as it
// had the type, which skipped the reference suffix every other type reaches at
// the bottom of the same function -- so `bool*` parsed and `bool&` did not.
// And `const` was only peeked at, never consumed, so `const bool` fell through
// to the C layer, which ate the const and then failed on the bool.

void print_int(int n);
void print_line();

void flip(bool &b) { b = !b; }

bool bothTrue(const bool &a, const bool &b) { return a && b; }

class Box {
public:
    bool flag;
    Box() { flag = false; }
    void set(const bool &v) { flag = v; }
    bool get() { return flag; }
};

int main() {
    bool x = true;
    flip(x);
    print_int(x);               // 0

    bool &r = x;
    r = true;
    print_int(x);               // 1  -- written through the reference

    const bool t = true;
    const bool &cr = t;
    print_int(cr);              // 1

    print_int(bothTrue(x, t));  // 1
    print_line();

    Box b;
    print_int(b.get());         // 0
    b.set(t);
    print_int(b.get());         // 1

    bool *p = &x;               // the pointer form still works
    print_int(*p);              // 1
    print_line();
    return 0;
}
