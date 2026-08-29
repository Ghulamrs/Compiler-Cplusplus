// friend grants access; a non-member operator is the only form that can put
// the class on the RIGHT of the operator.  Together they are what makes
// symmetric arithmetic  --  3 * m  as well as  m * 3  --  possible.
void print_int(int n);
void print_string(char* s);
void print_line();
class Money {
private:
  int cents;
public:
  Money() { cents = 0; }
  void set(int c) { cents = c; }
  int get() { return cents; }
  Money operator+(Money o) { Money r; r.cents = cents + o.cents; return r; }
  friend Money operator*(int k, Money m);
  friend Money operator*(Money m, int k) { Money r; r.cents = m.cents * k; return r; }
  friend bool operator<(Money a, Money b) { return a.cents < b.cents; }
};
Money operator*(int k, Money m) { Money r; r.set(m.get() * k); return r; }
int main() {
  Money a; a.set(150);
  Money b; b.set(250);
  Money c = a + b;              /* member operator            */
  print_int(c.get()); print_line();
  Money d = 3 * a;              /* NON-member, class on right */
  print_int(d.get()); print_line();
  Money e = a * 4;              /* non-member, class on left, reads private */
  print_int(e.get()); print_line();
  print_int(a < b);             /* non-member friend comparison */
  print_int(b < a); print_line();
  Money f = 2 * a + b;          /* mixed, chained             */
  print_int(f.get()); print_line();
  return 0;
}
