// const on a variable is enforced now, not merely parsed: it may be read
// freely, and never written after it is initialised.  It reaches through a
// reference and through member access, so a const object's fields are const.
void print_int(int n);
void print_line();
class A {
public:
  int x;
  A() { x = 1; }
};
int readOnly(const A& a) { return a.x; }
int useConstParam(const int n) { return n * 2; }
int main() {
  const int k = 10;
  print_int(k);
  print_int(k + 5);
  print_int(useConstParam(k));
  const A ca;                      /* a constructor initialises it */
  print_int(ca.x);
  print_int(readOnly(ca));
  const char* s = "ok";
  print_int(s[0]);
  s = "no";                        /* the POINTER may move; the chars may not */
  print_int(s[0]);
  const A* cp = &ca;
  print_int(cp->x);                /* reading through a const P* is fine */
  int m = 3;
  m = k;                           /* a const may be copied FROM */
  print_int(m);
  print_line();
  return 0;
}
