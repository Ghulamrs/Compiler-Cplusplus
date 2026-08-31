// An array whose size cannot be represented, and one that merely does not fit.
//
// `int a[700000000];` used to overflow the int this multiply was done in --
// 700000000 * 4 does not fit -- and a wrapped size is not a diagnostic. The
// array was laid out at whatever the wrap produced, the program COMPILED, it
// RAN, and it reported success. An accepted program that means nothing is the
// worst of the available answers, and it was the one being given.
//
// The size is computed in the machine's own word now, and compared against the
// memory the machine actually has while there is still a declaration to point
// at. The VM checks the same thing when it loads an image, but by then CodeGen
// has already built the bytes in the host: a declaration like the second one
// below reached a gigabyte of host memory before anything objected. On a
// desktop that is a slow error. On a phone it is the process being killed.
//
// Only the first is reported. The others are the same mistake, and one mistake
// costs one line.

void print_int(int);

// Too large for a class to hold, which used to add a second diagnostic of its
// own -- "field 'a' has no size" -- on top of the first.
class Big {
public:
    int a[700000000];
};

// Overflows the multiply outright.
int overflows() {
    int a[700000000];
    return a[0];
}

// Fits in an int and still does not fit in the machine.
int tooBig() {
    int b[300000000];
    return b[0];
}

// An array of objects reaches the same check through a different route.
class Cell {
public:
    double x;
    double y;
};

int objects() {
    Cell c[50000000];
    return 0;
}

int main() {
    Big big;
    print_int(overflows() + tooBig() + objects() + big.a[0]);
    return 0;
}
