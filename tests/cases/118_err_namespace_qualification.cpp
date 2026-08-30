// std::cout.
//
// Namespaces are excluded, and the keyword is reported by name -- but a
// QUALIFICATION is not the keyword.  `std::cout` was an identifier followed by
// a stray '::', and the three cascading errors that produced never said the
// word "namespace", in a language whose own <iostream> is the reason anyone
// types it in the first place.
//
// The qualifier is named and dropped and the name kept, so one mistake costs
// one line and the rest of the expression still parses.  A::f, which IS a
// qualified name this language has, is untouched: A names a class.
#include <iostream>

int abs(int n);                          // a native, so the recovery is clean

class A {
public:
    int f(int n);
};

int A::f(int n) { return n * 2; }        // still fine: A is a class

int main() {
    A a;
    cout << a.f(21) << endl;             // the unqualified form is the one

    std::cout << "hello" << std::endl;   // two qualifiers, two lines
    int v = std::abs(-3);

    return 0;
}
