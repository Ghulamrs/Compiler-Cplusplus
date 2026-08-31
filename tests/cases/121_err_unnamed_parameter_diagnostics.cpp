// A parameter need not have a name.  `void print_int(int);` is a complete
// declaration, and it heads a good part of this corpus -- so three messages
// that named the parameter by interpolating fn->params[i]->name printed
// nothing at all where there was nothing to print:
//
//     warning: narrowing unsigned int to int in the argument to ''
//     error: argument to reference parameter '' must be an lvalue
//     error: argument int does not match parameter '' of type A*
//
// An unnamed parameter is identified by the two things it does have, its
// position and its function.  Both spellings are here, side by side, because
// what makes the empty one hard to notice is that the named one reads fine.

void print_line();

class A {
public:
    int x;
};

// Unnamed, then named, for each of the three.
void narrows(int);
void narrowsNamed(int howMany);
int  takesRef(int &);
int  takesRefNamed(int &slot);
int  takesPtr(A*);
int  takesPtrNamed(A* which);

int main() {
    // Small on purpose.  Narrowing is judged from the TYPES, so any unsigned
    // does it -- and a literal above 2^31-1 makes this case answer differently
    // on LLP64, where `long` is 32 bits and Token::numberValue clips it before
    // it is ever a value.  That is a real defect, recorded in KNOWN-GAPS.md,
    // and it is not the one under test here.
    unsigned int u = 5;

    // Warnings: the program still compiles, and both name their parameter.
    narrows(u);                 // parameter 1 of 'narrows'
    narrowsNamed(u);            // parameter 'howMany'

    // An rvalue where a reference is wanted.
    takesRef(3);                // parameter 1 of 'takesRef'
    takesRefNamed(4);           // parameter 'slot'

    // A type that does not convert.
    A a;
    a.x = 1;
    takesPtr(a.x);              // parameter 1 of 'takesPtr'
    takesPtrNamed(a.x);         // parameter 'which'

    print_line();
    return 0;
}
