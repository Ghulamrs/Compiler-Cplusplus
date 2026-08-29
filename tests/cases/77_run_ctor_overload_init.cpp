// Constructor overloads reached through every route: a member initialiser, a
// base initialiser, and new.  A class-typed member is CONSTRUCTED by its
// initialiser, not assigned from it.
void print_int(int n); void print_line();
class Inner { public: int k; Inner(){k=0;} Inner(int a){k=a;} Inner(double a){k=99;} };
class Outer {
public:
  Inner in;
  int t;
  Outer() : in(7), t(1) { }              /* member init picks Inner(int) */
  Outer(double d) : in(d), t(2) { }      /* member init picks Inner(double) */
};
class Base { public: int b; Base(){b=0;} Base(int n){b=n;} Base(char* s){b=42;} };
class Kid : public Base {
public:
  Kid() : Base(5) { }                    /* base init picks Base(int)  */
  Kid(char* s) : Base(s) { }             /* base init picks Base(char*) */
};
int main() {
  Outer a;      print_int(a.in.k); print_int(a.t);
  Outer b(1.5); print_int(b.in.k); print_int(b.t);
  Kid c;        print_int(c.b);
  Kid d("x");   print_int(d.b);
  Inner* p = new Inner(3);  print_int(p->k);   /* new with an overload */
  Inner* q = new Inner(1.0); print_int(q->k);
  delete p; delete q;
  print_line();
  return 0;
}
