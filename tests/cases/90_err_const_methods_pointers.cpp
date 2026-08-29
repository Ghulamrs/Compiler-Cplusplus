// Each way of breaking the two promises.
class A {
public:
  int x;
  A() { x = 1; }
  int  get() const { return x; }
  int  bad() const { x = 5; return x; }   /* a const method writing a field */
  void bump()      { x = x + 1; }
};
int touchRef(const A& a) { a.bump(); return 0; }
int touchPtr(const A* p) { p->bump(); return 0; }
int freeFn() const { return 1; }          /* only a member may be const */
int main() {
  const A c;
  c.bump();
  char* const p = "ab";
  p = "cd";                               /* the pointer is const */
  const char* q = "cd";
  q[0] = 'z';                             /* the chars are const */
  return 0;
}
