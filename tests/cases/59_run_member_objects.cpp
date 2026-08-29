// Class-typed members are constructed in declaration order and destroyed in
// reverse.  Zed sorts after Wrap, which is how layout order got caught.
void print_int(int n);
void print_string(char* s);
void print_line();
class Zed {
public:
  int v;
  Zed()  { v = 42; }
  ~Zed() { print_string("~Zed "); }
};
class Inner {
public:
  int w;
  Inner()  { w = 7; }
  ~Inner() { print_string("~Inner "); }
};
class Wrap {
public:
  Inner in;
  Zed   z;
  Wrap()  { }
  ~Wrap() { print_string("~Wrap "); }
};
int main() {
  {
    Wrap w;
    print_int(w.in.w); print_int(w.z.v); print_line();
  }
  print_line();
  return 0;
}
