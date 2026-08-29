void print_int(int n);
void print_string(char* s);
void print_line();
bool isEven(int n) { return n % 2 == 0; }
class Flag {
public:
  bool on;
  Flag(bool v) : on(v) { }
  bool get() { return on; }
  bool flip() { on = !on; return on; }
};
int main() {
  bool t = true;
  bool f = false;
  print_int(t); print_int(f); print_line();
  print_int(isEven(4)); print_int(isEven(7)); print_line();
  bool c = 5 > 3;
  print_int(c); print_line();
  bool fromInt = 42;
  print_int(fromInt); print_line();
  bool fromZero = 0;
  print_int(fromZero); print_line();
  int back = t;
  print_int(back); print_line();
  Flag fl(false);
  print_int(fl.get()); print_int(fl.flip()); print_int(fl.get()); print_line();
  if (t && !f) { print_string("logic ok"); print_line(); }
  char* p = "x";
  bool hasP = p;
  print_int(hasP); print_line();
  double d = 0.0;
  bool nz = d;
  print_int(nz); print_line();
  return 0;
}
