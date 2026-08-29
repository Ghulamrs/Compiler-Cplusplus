// class B; names a type without defining it, which is what lets two classes
// hold pointers to each other.  A pointer needs only the name; anything by
// value still needs the definition.
void print_int(int n); void print_line();
class B;
class A { public: int x; B* partner; A(){x=1;partner=0;} };
class B { public: int y; A* partner; B(){y=2;partner=0;} };
int main() {
  A a; B b;
  a.partner = &b; b.partner = &a;
  print_int(a.partner->y); print_int(b.partner->x); print_line();
  return 0;
}
