class Base { public: int b; };
class Derived : public Base { public: int d; };
int takesBase(Base* p) { return p->b; }
int takesRef(Base& r) { return r.b; }
Base* makeBase() { Derived* d = 0; return d; }
int main() {
  Derived obj;
  obj.b = 1;
  obj.d = 2;
  Base* p = &obj;
  Base& r = obj;
  return takesBase(&obj) + takesRef(obj) + p->b + r.b;
}
