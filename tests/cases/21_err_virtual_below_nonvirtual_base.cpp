class Base { public: int f() { return 1; } };
class Derived : public Base { public: virtual int g() { return 2; } };
