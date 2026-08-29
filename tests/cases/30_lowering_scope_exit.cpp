class Res {
public:
  int h;
  Res() : h(0) { }
  ~Res() { }
};
int main() {
  Res a;
  { Res b; Res c; }
  Res d;
  if (a.h) { Res e; return 1; }
  return 0;
}
