class Base {
public:
  int b;
  Base() : b(0) { }
  Base(int v) : b(v) { }
  virtual int get() { return b; }
  virtual ~Base() { }
};
class Derived : public Base {
public:
  int d;
  int e;
  Derived(int v) : Base(v), d(v), e(v) { }
  ~Derived() { }
  int get() { return b + d + e; }
};
int main() {
  Derived obj(5);
  Base* p = new Derived(7);
  delete p;
  return obj.get();
}
