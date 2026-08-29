void print_int(int n);
void print_line();
int sum(int* p, int n) { int t = 0; for (int i = 0; i < n; i++) { t += p[i]; } return t; }
int main() {
  int a[5];
  for (int i = 0; i < 5; i++) { a[i] = i * i; }
  for (int i = 0; i < 5; i++) { print_int(a[i]); }
  print_line();
  print_int(sum(a, 5)); print_line();
  char buf[4];
  buf[0] = 'h'; buf[1] = 'i'; buf[2] = 0;
  print_int(buf[0]); print_int(buf[1]); print_line();
  int g[2][3];
  g[1][2] = 42;
  print_int(g[1][2]); print_line();
  return 0;
}
