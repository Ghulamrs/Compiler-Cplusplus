// Assignment and copy-initialisation of a class both copy every member.
void print_int(int n);
void print_line();
class P {
public:
  int a;
  int b;
  int c;
  P() { a = 1; b = 2; c = 3; }
};
int main() {
  P x;
  x.a = 10; x.b = 20; x.c = 30;
  P y;
  y = x;
  print_int(y.a); print_int(y.b); print_int(y.c); print_line();
  P z = x;
  print_int(z.a); print_int(z.b); print_int(z.c); print_line();
  return 0;
}
