// A constructor is an overload like any other.  P(int,int) and
// P(double,double) take the same NUMBER of arguments, so only the types can
// tell them apart -- which is why the symbol carries a signature.
void print_int(int n); void print_double(double d); void print_line();
class P {
public:
  int kind;
  double d;
  P()                  { kind = 0; }
  P(int a)             { kind = 1; }
  P(int a, int b)      { kind = 2; }
  P(double a, double b){ kind = 3; }     /* SAME arity as P(int,int) */
  P(char* s)           { kind = 4; }
};
int main() {
  P a;         print_int(a.kind);
  P b(1);      print_int(b.kind);
  P c(1, 2);   print_int(c.kind);
  P d(1.5, 2.5); print_int(d.kind);
  P e("hi");   print_int(e.kind);
  print_line();
  return 0;
}
