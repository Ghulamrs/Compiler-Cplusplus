// Operators overload by SIGNATURE.  Three friend operator* share a name and
// are told apart only by their parameter types.
void print_int(int n); void print_line();
class M {
private:
  int v;
public:
  M() { v = 0; }
  void set(int n) { v = n; }
  int  get() { return v; }
  /* three friend operators sharing a name, distinguished only by signature */
  friend M operator*(int k, M m)   { M r; r.v = k * m.v;   return r; }
  friend M operator*(M m, int k)   { M r; r.v = m.v * k;   return r; }
  friend M operator*(M a, M b)     { M r; r.v = a.v * b.v; return r; }
  friend bool operator<(M a, M b)  { return a.v < b.v; }
};
int main() {
  M a; a.set(3);
  M b; b.set(4);
  print_int((2 * a).get());
  print_int((a * 5).get());
  print_int((a * b).get());
  print_int(a < b); print_int(b < a);
  print_line();
  return 0;
}
