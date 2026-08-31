// The two remaining kinds of const, both real now.
//
//   int get() const     -- the const applies to *this
//   char* const p       -- the POINTER is const, not what it points at
//                          (const char* p is the other way round)
void print_int(int n);
void print_line();
class A {
public:
  int x;
  A() { x = 1; }
  int  get() const { return x; }
  void bump()      { x = x + 1; }
};
int readIt(const A& a)  { return a.get(); }
int viaPtr(const A* p)  { return p->get(); }
int main() {
  A a;
  a.bump(); a.bump();
  print_int(a.get());
  const A c;
  print_int(c.get());
  print_int(readIt(c));
  print_int(viaPtr(&c));
  char buf[3];               /* writable storage; a literal is not, and
                                writing through one is undefined -- which
                                makes it the wrong thing to assert here */
  buf[0] = 'a'; buf[1] = 'b'; buf[2] = 0;
  char* const p = buf;
  print_int(p[0]);
  p[0] = 'z';                /* the chars are NOT const */
  print_int(p[0]);
  const char* q = "cd";
  print_int(q[0]);
  q = "ef";                  /* the pointer IS free to move */
  print_int(q[0]);
  print_line();
  return 0;
}
