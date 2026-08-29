// ++ and -- have a type, and it has to reach the float side of lowering.
void print_double(double d);
void print_int(int n);
void print_line();
int main() {
  double x = 2.5;
  int i = 3;
  print_double(x * i++); print_line();
  print_int(i);          print_line();
  int j = 4;
  double z = j++;
  print_double(z);       print_line();
  print_int(j);          print_line();
  double y = 1.5;
  print_double(++y);     print_line();
  print_double(y--);     print_line();
  print_double(y);       print_line();
  return 0;
}
