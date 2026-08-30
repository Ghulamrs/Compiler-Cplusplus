// cout << p, for a pointer that is not a char*.
//
// There was no overload a pointer could reach except the bool -- `if (p)` is
// why bool takes one at all -- so `cout << p` printed 1, silently, for every
// pointer in the language.  ostream now has void*, and any object pointer
// converts to it as in C++.
//
// That creates the collision this compiler has no conversion ranking to
// settle: a pointer can become either a void* or a bool, and an overload set
// holding both is ambiguous for every pointer.  void* is called the exact
// answer for a pointer, which settles it the way C++ settles it.
#include <iostream>

class Point {
public:
    int x;
    Point() { x = 0; }
};

int main() {
    int a = 1;
    int b = 2;
    int *p = &a;
    int *q = &a;
    int *r = &b;

    // Two names for one object are one address; a different object is not.
    cout << (p == q) << (p == r) << endl;

    // A null pointer prints as 0, the way it is written.
    int *nothing = 0;
    cout << nothing << endl;

    // char* is still a string, not an address: it is an exact match, and an
    // exact match wins before any of this is consulted.
    char *s = new char[3];
    s[0] = 72; s[1] = 105; s[2] = 0;
    cout << s << " " << "literal" << endl;
    delete[] s;

    // bool still prints as 0 and 1, and a pointer still tests as one.
    bool flag = true;
    cout << flag << (p != 0) << endl;
    bool hasP = p;                  // still legal: bool converts from anything
    cout << hasP << endl;

    // void* is the generic address, and a cast brings it back.
    Point pt;
    pt.x = 7;
    void *v = &pt;
    Point *back = (Point*)v;
    cout << back->x << endl;

    // Addresses themselves are this machine's, so they are not printed here:
    // the test would then depend on the layout of its own frame.
    cout << (v == (void*)&pt) << endl;
    return 0;
}
