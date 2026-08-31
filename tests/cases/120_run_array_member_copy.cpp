// A class holding an array used to be copied byte for byte, whole.  No list
// can name an array's elements, so no copy constructor was generated at all --
// and the members BESIDE the array lost theirs with it.  For a member owning
// memory that was not a missing side effect but a double free: both objects
// held one pointer, and the second destructor released it again.
//
// The array is still moved whole, which is what the byte copy did for it and
// what a copy of scalars is.  Everything else in the class is copied the way
// it would have been if the array were not there.

void print_int(int);
void print_string(char*);
void print_line();

int live = 0;

// Owns a block, so a shared pointer shows up as a double delete rather than as
// a number that is merely wrong.
class Owner {
public:
    int *p;
    Owner()                { p = new int; *p = 1; live = live + 1; }
    Owner(const Owner &o)  { p = new int; *p = *o.p + 1; live = live + 1; }
    ~Owner()               { delete p; live = live - 1; }
};

class WithScalarArray {
public:
    Owner o;
    int a[3];
    WithScalarArray() { a[0] = 7; a[1] = 8; a[2] = 9; }
};

// The elements are objects: each is copy-constructed from its opposite number,
// because calling the constructor per element is something lowering can do
// even where the list cannot say it.
class WithObjectArray {
public:
    Owner many[2];
    int tag;
    WithObjectArray() { tag = 4; }
};

// One holding the other: the fix has to reach through, or the outer class is
// back on the byte copy for everything the inner one owns.  The constructor is
// written out because a class that declares none does not construct its
// members at all -- a separate gap, and not the one under test here.
class Holder {
public:
    Owner o;
    WithScalarArray inner;
    Holder() {}
};

int main() {
    print_string("start ");    print_int(live); print_line();

    {
        WithScalarArray a;
        WithScalarArray b = a;
        print_string("scalar array ");
        print_int(*b.o.p);              // 2  -- Owner's copy constructor ran
        print_int(b.a[2]);              // 9  -- and the array arrived
        print_int(live);                // 2  -- two live objects, not one
        print_line();
    }
    print_string("after ");    print_int(live); print_line();      // 0

    {
        WithObjectArray a;
        WithObjectArray b = a;
        print_string("object array ");
        print_int(*b.many[0].p);        // 2  -- element 0 copy-constructed
        print_int(*b.many[1].p);        // 2  -- and so was element 1
        print_int(b.tag);               // 4
        print_int(live);                // 4  -- every element is its own object
        print_line();
    }
    print_string("after ");    print_int(live); print_line();      // 0

    {
        Holder a;
        Holder b = a;
        print_string("through a holder ");
        print_int(*b.o.p);              // 2
        print_int(*b.inner.o.p);        // 2  -- the nested owner too
        print_int(b.inner.a[0]);        // 7
        print_int(live);                // 4
        print_line();
    }
    print_string("after ");    print_int(live); print_line();      // 0

    return 0;
}
