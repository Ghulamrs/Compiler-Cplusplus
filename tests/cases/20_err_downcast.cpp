class Base { public: int b; };
class Derived : public Base { public: int d; };
int main() {
  Base base;
  Base* bp = &base;
  Derived* dp = bp;
  return dp->d;
}
