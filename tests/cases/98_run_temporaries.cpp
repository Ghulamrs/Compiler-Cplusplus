// T(args) builds an unnamed object where it stands.  `return V(x + o.x);` is
// the ordinary body of an overloaded operator, so this is not a luxury.
void print_int(int n); void print_line();
class V {
public:
  int x;
  V() { x = 0; }
  V(int n) { x = n; }
  V operator+(V o) { return V(x + o.x); }
  V operator*(int k) { return V(x * k); }
};
int twice(V v) { return v.x * 2; }
int main() {
  V a(3); V b(4);
  print_int((a + b).x);
  print_int((a + b + a).x);
  print_int(((a + b) * 2).x);
  print_int(twice(V(9)));
  V c = V(11);
  print_int(c.x);
  print_line(); return 0;
}
