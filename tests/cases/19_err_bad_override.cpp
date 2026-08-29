class Base {
public:
  virtual int f(int a) { return a; }
  virtual int g(int a) { return a; }
};
class Derived : public Base {
public:
  char f(int a) { return a; }
  int g(int a, int b) { return a + b; }
};
