class Res {
public:
  int h;
  Res() : h(0) { }
  ~Res() { }
};
class Plain { public: int v; };
int main() {
  Res a;
  Plain notDestroyed;
  { Res b; Res c; }
  Res d;
  if (a.h) { Res e; return 1; }
  return notDestroyed.v;
}
