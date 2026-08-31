// `std::cout` and `using namespace std;`, both accepted, both silent.
//
// This language's <iostream> is written in the language itself and puts cout,
// cin, cerr and endl at global scope.  So `std::cout` and `cout` name the same
// object, and `using namespace std;` asks for something that is already true.
// Refusing either bought nothing: the name resolved the same way regardless,
// and the diagnostic was a spelling complaint about the most common spelling
// there is -- which, for a program someone pasted in, was the whole difference
// between compiling and not.
//
// Every other qualifier is still refused, and refused rather than dropped:
// see 118_err_namespace_qualification.
#include <iostream>
using namespace std;

double sqrt(double x);

class Point {
public:
    int x;
    Point(int v) { x = v; }
    int get() const { return x; }
};

int main() {
    // Qualified.
    std::cout << "qualified" << std::endl;

    // Unqualified, in the same program, meaning the same thing.
    cout << "plain" << endl;

    // Mixed inside one statement.
    std::cout << "mixed " << 1 << " " << 2 << endl;

    // On a native, and on a name of the program's own making.
    std::cout << std::sqrt(16.0) << std::endl;

    Point p(7);
    std::cout << p.get() << std::endl;

    return 0;
}
