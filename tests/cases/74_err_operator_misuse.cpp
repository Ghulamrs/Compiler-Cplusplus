// An operator that does not exist is named, not silently approximated by
// arithmetic on the object's bytes.
void print_int(int n);
class V { public: int x; V() { x = 0; } V operator+(V o) { return o; } };
class W { public: int y; W() { y = 0; } };
int main() {
  V a; V b;
  a += b;                 /* no operator+= */
  W c; W d;
  W e = c + d;            /* no operator+ at all */
  print_int(a.x);
  return 0;
}
