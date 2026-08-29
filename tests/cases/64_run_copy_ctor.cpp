// A declared copy constructor IS the copy, and its parameter is a reference,
// so it receives an address rather than the bytes it is meant to copy.
void print_int(int n);
void print_string(char* s);
void print_line();
class A {
public:
  int x;
  A() { x = 1; }
  A(const A& o) { x = o.x + 100; print_string("copyctor "); }
};
int main() {
  A a; a.x = 5;
  A b(a);
  A c = a;
  print_int(b.x); print_int(c.x); print_line();
  return 0;
}
