// Warnings and errors are two channels, and one used to switch the other off.
//
// The error cap is twenty. Past that, `capped` was set - and the warning
// routine tested it too, so the twenty-first ERROR silenced every warning for
// the rest of the compilation. Not deferred, not counted and summarised:
// gone, including warnings about the very code the errors were in, and with
// nothing said about their disappearance.
//
// A program with a typo near the top is exactly the program whose warnings
// matter, so this was worst where it was least affordable.
//
// Below: twenty-two undeclared names, which is past the cap, and then a
// narrowing call whose warning has to arrive anyway. The counts in the summary
// are the point - errors capped, warnings not.

void print_int(int);
void print_line();

int a0 = missing0;
int a1 = missing1;
int a2 = missing2;
int a3 = missing3;
int a4 = missing4;
int a5 = missing5;
int a6 = missing6;
int a7 = missing7;
int a8 = missing8;
int a9 = missing9;
int b0 = absent0;
int b1 = absent1;
int b2 = absent2;
int b3 = absent3;
int b4 = absent4;
int b5 = absent5;
int b6 = absent6;
int b7 = absent7;
int b8 = absent8;
int b9 = absent9;
int c0 = gone0;
int c1 = gone1;
int c2 = gone2;

int main() {
    double d = 3.9;
    print_int(d);            // the warning that has to survive all of the above
    print_line();
    return 0;
}
