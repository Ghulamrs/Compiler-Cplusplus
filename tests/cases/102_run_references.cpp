// A reference in every position the language allows it: parameter, local,
// bound to a field, to an array element, to another reference, and returned.
#include <iostream>
class Box { public: int v; Box() { v = 0; } };
int g = 1;
void byRef(int &r) { r = r * 2; }
void byConstRef(const int &r, int &out) { out = r + 1; }
int &ret() { return g; }
int main() {
    int a = 21;
    byRef(a);
    cout << "param:" << a << endl;
    int &la = a;
    la = la + 1;
    cout << "local:" << a << endl;
    Box b;
    int &fr = b.v;
    fr = 7;
    cout << "field:" << b.v << endl;
    int arr[3];
    arr[1] = 5;
    int &er = arr[1];
    er = 9;
    cout << "elem:" << arr[1] << endl;
    int out = 0;
    byConstRef(a, out);
    cout << "constref:" << out << endl;
    int &chain = la;
    chain = 100;
    cout << "chain:" << a << endl;
    ret() = 5;
    cout << "retref:" << g << endl;
    return 0;
}
