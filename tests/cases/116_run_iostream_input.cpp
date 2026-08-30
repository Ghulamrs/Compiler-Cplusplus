// cin and >>, the other half of <iostream>.
//
// istream is the same shape as ostream: no state of its own, every operator
// forwards to a native and returns *this so that  cin >> a >> b  chains.
// Whether the last read worked lives in the machine rather than in the object,
// which is why cin.good() is right after a whole chain and not just after its
// first read -- and why returning a copy by value costs nothing.
//
// A read that fails leaves its destination alone, as C++98 says.  There are no
// exceptions here to throw instead, so the flag is the whole mechanism.
//
// The input for this case is tests/input/116_run_iostream_input.txt.
#include <iostream>

int main() {
    int a;
    int b;
    cin >> a >> b;
    cout << "sum " << a + b << endl;
    cout << "good " << cin.good() << endl;

    double d;
    char c;
    cin >> d >> c;
    cout << d << " " << c << endl;

    bool flag;
    cin >> flag;
    cout << "flag " << flag << endl;

    // A word into a buffer that knows how long it is.
    char *w = new char[32];
    cin >> w;
    cout << "[" << w << "]" << endl;
    delete[] w;

    // The rest of the line, with the width given.
    char line[64];
    cin.getline(line, 64);
    cin.getline(line, 64);
    cout << "[" << line << "]" << endl;

    // Reading past the end fails, and leaves the destination alone.
    int keep = 99;
    cin >> keep;
    cout << "kept " << keep << endl;
    cout << "eof " << cin.eof() << endl;
    return 0;
}
