// The corners: an overloaded operator chosen by its right operand, reference
// parameters, operands that are array elements, an operator used inside a
// member, an inherited operator, and compound assignment.
void print_int(int n);
void print_line();
class V {
public:
  int x;
  V() { x = 0; }
  V operator+(V o)        { V r; r.x = x + o.x; return r; }
  V operator+(int k)      { V r; r.x = x + k;   return r; }
  V operator/(V o)        { V r; r.x = x / o.x; return r; }
  V operator%(V o)        { V r; r.x = x % o.x; return r; }
  V operator+=(V o)       { x = x + o.x; return *this; }
  V operator*=(int k)     { x = x * k;   return *this; }
  bool operator<(const V& o) { return x < o.x; }
  V twice()               { return *this + *this; }
};
class Base {
public:
  int x;
  Base() { x = 0; }
  Base operator+(Base o) { Base r; r.x = x + o.x; return r; }
  virtual int who() { return 1; }
  virtual ~Base() { }
};
class Derived : public Base {
public:
  Derived() { x = 0; }
  virtual int who() { return 2; }
};
int main() {
  V a; a.x = 10;
  V b; b.x = 3;
  print_int((a + b).x);          /* operator+(V)   */
  print_int((a + 5).x);          /* operator+(int) */
  print_int((a / b).x);
  print_int((a % b).x);
  print_int(a.twice().x);
  print_line();
  print_int(a < b); print_int(b < a);
  V arr[3]; arr[0].x = 1; arr[1].x = 2;
  print_int((arr[0] + arr[1]).x);
  print_line();
  V c; c.x = 1; V d; d.x = 2;
  c += d;  print_int(c.x);
  c *= 10; print_int(c.x);
  print_line();
  Derived p; p.x = 3;
  Derived q; q.x = 4;
  Base r = p + q;                /* inherited operator; vptr survives */
  print_int(r.x); print_int(r.who());
  Base* bp = &p;
  print_int(bp->who());
  print_line();
  return 0;
}
