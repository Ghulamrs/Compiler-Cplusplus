class Base { public: int b; };
class D : public Base {
public:
  int p;
  int q;
  D() : b(1), p(2), zzz(3), p(4) { }
  D(int v) : q(v), p(v) { }
};
