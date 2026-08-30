// A class that declares NO constructor is default-constructible -- that is the
// C case, and it stays true when the compiler gives the class an implicit copy
// constructor.  Writing nothing cannot be what makes `Derived a;` illegal.
//
// Also checks the two things that go with it: the copy itself still runs the
// base's copy constructor, and a polymorphic class with no declared
// constructor still gets its vptr, so a virtual call through a base pointer
// reaches the override.

void print_int(int);
void print_string(const char *s);
void print_line();

class Base {
public:
    int tag;
    Base() { tag = 1; }
    Base(const Base &o) { tag = o.tag + 100; }
};

// Declares nothing at all: gets an implicit copy constructor, and must keep
// its implicit default constructor.
class Derived : public Base {
public:
    int extra;
};

class Shape {
public:
    virtual void name() { print_string("Shape"); print_line(); }
};

// Also declares nothing: still needs its vptr set when it is constructed.
class Circle : public Shape {
public:
    virtual void name() { print_string("Circle"); print_line(); }
};

int main() {
    Derived a;
    a.extra = 2;
    print_int(a.extra);
    print_line();

    // Set the base field here rather than reading what Base() left, so this
    // case measures the copy and nothing else.
    a.tag = 5;
    Derived b = a;          // the base's copy constructor runs: 5 + 100
    print_int(b.tag);
    print_line();

    Circle c;
    Shape *p = &c;
    p->name();
    return 0;
}
