int narrow(char c) { return c; }
int main() {
  double d = 3.9;
  int    i = d;
  char   c = 300;
  short  s = 70000;
  unsigned int u = -1;
  float  ok = 1.0;
  short  fits = 1;
  int    r = narrow(i);
  d = i;
  return i;
}
