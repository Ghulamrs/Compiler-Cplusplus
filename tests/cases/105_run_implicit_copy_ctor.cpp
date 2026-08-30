// A copy is memberwise: the base is copied by ITS copy constructor and so is
// every member that has one.  A class that declares none of its own still
// needs one, or the copy is a byte copy and those constructors never run --
// which is a silent wrong answer for any class whose copy does more than move
// bytes, exactly what a declared copy constructor exists to prevent.
//
// Counted counts its own copies, so a number greater than zero is proof the
// constructor ran rather than the bytes being moved past it.
//
// Not covered here, and deliberately: a class with an ARRAY member keeps the
// byte copy.  An initialiser list cannot name an array, so a synthesised
// constructor would leave it default-constructed and lose the source's
// elements -- worse than the copy such a class already gets.
#include <iostream>
class Counted {
public:
    int n;
    Counted() { n = 0; }
    Counted(const Counted &o) { n = o.n + 1; }
};
class Mid  : public Counted { public: int m; Mid()  { m = 5; } };
class Deep : public Mid     { public: int d; Deep() { d = 9; } };
class Holds { public: Counted held; int k; Holds() { k = 3; } };
class Both  : public Counted { public: Counted held; int k; Both() { k = 4; } };

void byValue(Deep x) { cout << "callee " << x.n << " " << x.m << " " << x.d << endl; }

int main() {
    // through two levels of inheritance, with each level's own members intact
    Deep a;
    Deep b = a;
    cout << "derived " << b.n << " " << b.m << " " << b.d << endl;
    Deep c = b;
    cout << "twice " << c.n << endl;

    // a member that has one
    Holds h;
    Holds h2 = h;
    cout << "member " << h2.held.n << " " << h2.k << endl;

    // both at once, and they are independent
    Both x;
    Both y = x;
    cout << "both " << y.n << " " << y.held.n << " " << y.k << endl;

    // a by-value parameter is a copy like any other
    byValue(a);

    // a declared copy constructor still wins over the generated one
    Counted p;
    Counted q = p;
    cout << "declared " << q.n << endl;
    return 0;
}
