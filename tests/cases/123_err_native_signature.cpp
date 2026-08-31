// A function with no body whose NAME is a built-in's is bound to that built-in.
// That declaration is the whole of the binding: there is no linker in this
// pipeline to check it against a symbol, and nothing checked it against
// anything else either. So a declaration that merely spelled the name right
// was accepted, and the call went through with whatever the machine's own
// signature was:
//
//     int sqrt(int x);        sqrt(4)    printed 0
//     double pow(double a);   pow(2.0)   printed 1
//
// Both compiled, both ran, and both printed a wrong number without a word.
// The values themselves were never the problem -- they come from the host's
// own libm, and agree with it to the last digit -- but only if the machine is
// handed what it expects.
//
// The table records how many arguments each built-in takes and whether it
// answers in floating point, so a declaration disagreeing with either is
// refused here. It does not record each parameter's type, with one exception
// that holds throughout: a built-in answering in floating point takes floating
// point all the way across.

void print_line();

// Right, and left alone.
double sqrt(double x);

// The return type disagrees.
int sin(int x);

// The count disagrees.
double pow(double a);

// The count agrees and the parameter does not.
double fabs(int n);

int main() {
    print_line();
    return 0;
}
