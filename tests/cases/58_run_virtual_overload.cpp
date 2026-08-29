// Two virtuals may share a name.  The slot is chosen by signature.
void print_int(int n);
void print_line();
class Base {
public:
  virtual int f(int a)        { return 100 + a; }
  virtual int f(int a, int b) { return 200 + a + b; }
  virtual ~Base() { }
};
class Derived : public Base {
public:
  int f(int a)        { return 300 + a; }
  int f(int a, int b) { return 400 + a + b; }
};
int main() {
  Derived d;
  Base* p = &d;
  print_int(p->f(1));    print_line();
  print_int(p->f(1, 2)); print_line();
  return 0;
}
