// A namespace qualification that is not `std::`.
//
// Namespaces are excluded, and the keyword is reported by name -- but a
// QUALIFICATION is not the keyword.  `foo::bar` was an identifier followed by
// a stray '::', and the three cascading errors that produced never said the
// word "namespace".
//
// The qualifier is named and dropped and the name kept, so one mistake costs
// one line and the rest of the expression still parses.  Dropping it is not
// the same as accepting it: there is no foo, and reading `foo::bar` as `bar`
// would be answering a question nobody asked -- so it is reported.
//
// `std::` is the one exception and is accepted in silence; that is
// 125_run_std_qualifier, which is a run_ case precisely because it produces no
// diagnostic at all.  A::f, which IS a qualified name this language has, is
// untouched here: A names a class.
#include <iostream>

class A {
public:
    int f(int n);
};

int A::f(int n) { return n * 2; }        // still fine: A is a class

int main() {
    A a;
    cout << a.f(21) << endl;             // the unqualified form is the one

    cout << foo::bar << endl;            // one line, naming the qualifier
    int v = other::thing;                // and one for this one

    return 0;
}
