void print_int(int n);
void print_line();
int add(int a, int b) { return a + b; }
int main() {
  int total = 0;
  for (int i = 1; i <= 5; i = i + 1) { total = add(total, i); }
  print_int(total);
  print_line();
  return total;
}
