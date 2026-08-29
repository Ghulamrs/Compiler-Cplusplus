// Globals have initialisers, and global objects have constructors.  Both have
// to run before main does.
void print_int(int n);
void print_line();
int g = 5;
int h = 7;
class C {
public:
  int v;
  C() { v = 42; }
  virtual int get() { return v; }
  virtual ~C() { }
};
C gc;
int main() {
  print_int(g); print_int(h); print_line();
  g = g + h;
  print_int(g);       print_line();
  print_int(gc.v);    print_line();
  C* p = &gc;
  print_int(p->get());print_line();
  return 0;
}
