class P {
public:
  int x;
  P(int a) : x(a) { }
};
int main() {
  P p;
  P q(1, 2);
  P* r = new P();
  P* s = new P(1);
  return s->x;
}
