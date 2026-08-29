void print_double(double d);
void print_line();
int main() {
  double a = 2.0;
  print_double(a * a * a);           print_line();
  print_double(1.5 + 2.5 + 3.0);     print_line();
  print_double(10.0 / 2.0 / 2.5);    print_line();
  print_double(1.0 + 2.0 * 3.0);     print_line();
  float f = 1.5f;
  print_double(f * f * 2.0);         print_line();
  print_double(-(1.5 * 2.0));        print_line();
  return 0;
}
