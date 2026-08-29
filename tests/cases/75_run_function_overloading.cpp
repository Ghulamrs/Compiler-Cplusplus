// Overloads are chosen by argument TYPE, not just count -- including class
// and pointer parameters.
void print_int(int n); void print_double(double d); void print_string(char* s); void print_line();
int f(int a)            { return 1; }
int f(int a, int b)     { return 2; }
int f(double a)         { return 3; }
int f(char* s)          { return 4; }
int f(int a, double b)  { return 5; }
class K { public: int v; K(){v=0;} };
int g(K k)              { return 6; }
int g(int n)            { return 7; }
int main() {
  print_int(f(1));
  print_int(f(1,2));
  print_int(f(1.5));
  print_int(f("x"));
  print_int(f(1, 2.5));
  K k;
  print_int(g(k));
  print_int(g(9));
  print_line();
  return 0;
}
