// An excluded construct on the line after `using namespace std;`.
//
// The directive is stepped over in silence, and a silent skip that answers
// with no declaration reads to parseTranslationUnit exactly like a failed
// parse -- so `if (!d) synchronize()` ran and skipped the token after it. The
// next line's `template` was swallowed whole, and what was left of it,
// `<class T>`, parsed as a class:
//
//     expected '{' to open a class body, found '>'
//     expected a declaration, found '>'
//     expected a declaration, found return
//     expected a declaration, found '}'
//
// Four lines, none of which contained the word template -- for a program whose
// single fault is one this compiler has a good message for and had already
// written. The skip says it succeeded now, which is what it did.
//
// Every one of these is a construct with a named diagnostic, placed where it
// can only be reached through the directive above it. The point is not the
// diagnostics themselves, which have their own cases; it is that they still
// arrive from here.
#include <iostream>
using namespace std;

template <class T> T largest(T a, T b) { return a; }

enum Colour { Red, Green };

typedef int Number;

union U { int i; };

int main() {
    cout << "unreachable" << endl;
    return 0;
}
