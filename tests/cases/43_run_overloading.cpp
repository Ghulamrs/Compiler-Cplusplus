void print_int(int n);
void print_double(double d);
void print_line();
int    twice(int a)          { return a * 2; }
double twice(double a)       { return a * 2.0; }
int    add(int a, int b)     { return a + b; }
int    add(int a, int b, int c) { return a + b + c; }
class Box {
public:
  int v;
  Box(int x) : v(x) { }
  int get()        { return v; }
  int get(int off) { return v + off; }
};
int main() {
  print_int(twice(3));        print_line();
  print_double(twice(2.5));   print_line();
  print_int(add(1, 2));       print_line();
  print_int(add(1, 2, 3));    print_line();
  Box b(10);
  print_int(b.get());         print_line();
  print_int(b.get(5));        print_line();
  return 0;
}
