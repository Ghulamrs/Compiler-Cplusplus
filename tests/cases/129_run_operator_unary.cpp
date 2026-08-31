// Unary minus, overloaded.
//
// `operator-` is TWO functions with one name.  The member taking a parameter
// is subtraction; the member taking NOTHING is negation, and the only thing
// that tells them apart is that count.  The parser accepted both spellings
// from the start -- `operator-` is in the table of names it will take after
// `operator` -- and the analyser then refused every USE of the second one:
//
//     error: unary '-' needs an arithmetic type, got V
//
// which is the worst of the three possible answers.  Refusing the declaration
// would have been honest.  Accepting both would have been right.  Accepting
// the declaration and refusing the call let a person write a negation, see it
// compile, and be told at the point of use that their own class was not
// arithmetic -- a message about the operand that says nothing about the
// function sitting right there.
//
// It matters more than the count of operators suggests.  Unary minus is the
// one unary operator this compiler overloads, and it is the one worth having:
// a vector, a matrix and a complex number all negate, and none of them have a
// meaningful `!` or `~`.  A class that could add but not negate could not
// express `a - b` as `a + -b`, or a step in the opposite direction, without
// naming a method.
//
// Every route is here because they lower differently:
//   - the member with no parameters, beside the binary member of the same name
//   - a non-member taking the operand, for a class that cannot be changed
//   - the result used as a value, as an address, and as an operand again

void print_int(int);
void print_line();

class V {
public:
    int a;
    int b;
    V(int x, int y) : a(x), b(y) {}
    V() : a(0), b(0) {}

    // Negation and subtraction, same name, told apart by the parameter count.
    V operator-() { return V(-a, -b); }
    V operator-(const V &o) { return V(a - o.a, b - o.b); }
    V operator+(const V &o) { return V(a + o.a, b + o.b); }
};

// A non-member on the left of an object, which is how scaling gets written.
V operator*(int k, const V &v) { return V(k * v.a, k * v.b); }

// A class whose definition is not being changed gets its negation from
// outside, and the one parameter is what makes THIS one unary.
class W {
public:
    int v;
    W(int x) : v(x) {}
};
W operator-(const W &w) { return W(-w.v); }

int main() {
    V p(3, 4);

    V q = -p;
    print_int(q.a); print_int(q.b);              // -3 -4

    // The binary member of the same name still resolves as itself.
    V s = p - q;
    print_int(s.a); print_int(s.b);              // 6 8

    // a - b written as a + -b, which is the point of having the operator.
    V t = p + -q;
    print_int(t.a); print_int(t.b);              // 6 8

    // A free binary operator whose right operand is a unary call.
    V u = 2 * -p;
    print_int(u.a); print_int(u.b);              // -6 -8

    // The result as an ADDRESS: (-p).a has to find the slot it was built in.
    print_int((-p).a);                           // -3

    // And as an operand of itself.
    V d = -(-p);
    print_int(d.a); print_int(d.b);              // 3 4

    // The non-member route.
    W w(5);
    W n = -w;
    print_int(n.v);                              // -5

    // A negation inside a condition, where the value is consumed and dropped.
    int i = 0;
    V acc;
    while (i < 3) {
        acc = acc + -V(i, i * 2);
        i = i + 1;
    }
    print_int(acc.a); print_int(acc.b);          // -3 -6

    print_line();
    return 0;
}
