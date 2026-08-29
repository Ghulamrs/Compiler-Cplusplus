// `this` in every place it can appear: a constructor, a method, an operator,
// returned as a pointer, dereferenced by value, and reaching a base method
// from a derived class.
void print_int(int n); void print_string(char* s); void print_line();
class C {
private:
  int v;
public:
  C() { v = 0; }
  C(int n) { this->v = n; }                  /* this-> in a ctor        */
  void set(int n) { this->v = n; }           /* this-> in a method      */
  int get() { return this->v; }
  C* self() { return this; }                 /* returning this          */
  C twin() { return *this; }                 /* *this by value          */
  C add(C o) { C r; r.v = this->v + o.v; return r; }
  C operator+(C o) { return this->add(o); }  /* this-> inside operator  */
  C operator+=(C o) { this->v += o.v; return *this; }   /* return *this */
  bool isBig() { return this->v > 100; }
  ~C() { }
};
int useIt(C* p) { return p->get(); }
class D : public C {
public:
  D() { }
  int viaBase() { return this->get(); }      /* this-> reaching a base method */
};
int main() {
  C a(5);
  print_int(a.get());
  print_int(a.self()->get());
  print_int(a.twin().get());
  print_int((a + a).get());
  a += a;
  print_int(a.get());
  print_int(a.isBig());
  print_int(useIt(a.self()));
  D d;
  d.set(9);
  print_int(d.viaBase());
  print_line();
  return 0;
}
