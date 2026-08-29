// Overloaded operators are ordinary members: the expression becomes a call.
// Returning an object needs the caller to supply the space, which is what
// makes  V c = a + b + a;  work.
void print_int(int n);
void print_string(char* s);
void print_line();
class V {
public:
  int x;
  int y;
  V() { x = 0; y = 0; }
  V operator+(V o)  { V r; r.x = x + o.x; r.y = y + o.y; return r; }
  V operator-(V o)  { V r; r.x = x - o.x; r.y = y - o.y; return r; }
  V operator*(int k) { V r; r.x = x * k;  r.y = y * k;  return r; }
  bool operator==(V o) { return x == o.x && y == o.y; }
  bool operator!=(V o) { return !(x == o.x && y == o.y); }
  bool operator<(V o)  { return x < o.x; }
};
int main() {
  V a; a.x = 1; a.y = 2;
  V b; b.x = 10; b.y = 20;
  V c = a + b;
  print_int(c.x); print_int(c.y); print_line();
  V d = b - a;
  print_int(d.x); print_int(d.y); print_line();
  V e = a * 3;
  print_int(e.x); print_int(e.y); print_line();
  print_int(a == b); print_int(a != b); print_int(a < b); print_line();
  V f = a + b + a;
  print_int(f.x); print_int(f.y); print_line();
  if (a < b) { print_string("a<b"); print_line(); }
  return 0;
}
