// && and || must yield 0 or 1, not the operand that decided the answer.
void print_int(int n);
void print_line();
int main() {
  print_int(2 && 4);  print_line();
  print_int(0 || 7);  print_line();
  print_int(0 && 5);  print_line();
  print_int(0 || 0);  print_line();
  int a = 3;
  int b = (a > 1) && (a < 10);
  print_int(b);       print_line();
  print_int(!5);      print_line();
  print_int(!0);      print_line();
  return 0;
}
