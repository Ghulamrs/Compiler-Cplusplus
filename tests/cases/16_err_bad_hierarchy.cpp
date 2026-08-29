class A : public Missing { public: int a; };
class Selfish : public Selfish { public: int s; };
class X : public Y { public: int x; };
class Y : public X { public: int y; };
