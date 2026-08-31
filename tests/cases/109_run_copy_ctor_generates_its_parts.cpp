// The generated copy constructor names every base and every class-typed member
// in its initialiser list, so each of them must have a constructor taking one
// argument.  A part that merely had a default constructor did not, and naming
// it made a class that used to compile stop compiling.
//
// The rule now runs downwards: a part that will be named gets a generated copy
// constructor of its own first.  An array member is named too, though no list
// can reach its elements: what its entry means is settled in lowering, where
// each element is copy-constructed from its opposite number.

void print_int(int);
void print_line();

class Counted {
public:
    int n;
    Counted() { n = 0; }
    Counted(const Counted &o) { n = o.n + 1; }
};

// Only a default constructor: named by Outer's generated list, so it gets a
// generated copy constructor too.
class Plain {
public:
    int x;
    Plain() { x = 5; }
};

class Outer {
public:
    Counted c;
    Plain p;
    Outer() {}
};

// A base with no copy constructor, named by Derived's generated list.
class Base {
public:
    int t;
    Base() { t = 7; }
};

class Derived : public Base {
public:
    Counted c;
    Derived() {}
};

// An array member no list can name.  It used to cost the WHOLE class its
// generated copy constructor, so Counted's never ran either -- and a Counted
// that owned memory would have been shared between the two objects and freed
// twice.  Both parts are copied properly now: c by its constructor, a by a
// whole-array move.
class HasArray {
public:
    Counted c;
    int a[3];
    HasArray() { a[0] = 1; a[1] = 2; a[2] = 3; }
};

// Holding one of those inherits the same answer.
class Holder {
public:
    Counted c;
    HasArray h;
    Holder() {}
};

int main() {
    Outer o;
    o.c.n = 1;
    Outer o2 = o;
    print_int(o2.p.x);          // 5   -- Plain was copied, not left default
    print_int(o2.c.n);          // 2   -- Counted's copy constructor ran
    print_line();

    Derived d;
    d.c.n = 4;
    Derived d2 = d;
    print_int(d2.t);            // 7   -- the base arrived
    print_int(d2.c.n);          // 5
    print_line();

    HasArray x;
    x.c.n = 5;
    HasArray y = x;
    print_int(y.a[2]);          // 3   -- the array arrives
    print_int(y.c.n);           // 6   -- and Counted's copy constructor ran
    print_line();

    Holder h;
    h.c.n = 1;
    Holder h2 = h;
    print_int(h2.h.a[0]);       // 1
    print_int(h2.c.n);          // 2   -- and it propagates through the holder
    print_line();
    return 0;
}
