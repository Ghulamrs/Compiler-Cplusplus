class Base {
public:
  int b;
  virtual int get() { return b; }
  ~Base() { }
};
class Derived : public Base {
public:
  int d;
  int get() { return d; }
};
int main() {
  Base* p = new Derived();
  delete p;
  return 0;
}
