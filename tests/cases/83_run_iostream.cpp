// <iostream> is written in this language and prepended when a program includes
// it: ostream forwards to the printing natives and returns *this, which is
// what makes a chain of << work.  endl is an object, so it picks an overload
// rather than needing a rule of its own.
#include <iostream>
int main() {
  cout << "Hello, world!" << endl;
  cout << 42 << " " << 3.5 << endl;
  int a = 7;
  cout << "a = " << a << ", a*2 = " << a * 2 << endl;
  char c = 'X';
  cout << c << endl;
  bool t = true;
  cout << t << endl;
  double r = 2.0;
  cout << "area " << 3.14159 * r * r << endl;
  cerr << "to cerr" << endl;
  return 0;
}
