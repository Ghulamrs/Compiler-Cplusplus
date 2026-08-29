// The commonest program anyone will write against <iostream>: streaming a
// class of your own through a non-member operator<<.  ostream has member
// <<s, and those used to suppress every free one.
#include <iostream>
class P { public: int v; P(){v=7;} };
ostream operator<<(ostream o, P p) { o << p.v; return o; }
int main(){ P p; cout << p << endl; return 0; }
