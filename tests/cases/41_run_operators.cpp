void print_int(int n);
void print_line();
int main() {
  int i = 5;
  print_int(i++); print_int(i); print_line();
  print_int(++i); print_int(i); print_line();
  print_int(i--); print_int(--i); print_line();
  int a = 10;
  a += 5;  print_int(a); print_line();
  a -= 3;  print_int(a); print_line();
  a *= 2;  print_int(a); print_line();
  a /= 4;  print_int(a); print_line();
  a %= 4;  print_int(a); print_line();
  int n = 0;
  do { n++; } while (n < 3);
  print_int(n); print_line();
  int r = 0;
  switch (n) {
    case 1: r = 100; break;
    case 3: r = 300;
    case 4: r = r + 7; break;
    default: r = -1;
  }
  print_int(r); print_line();
  double d = 3.7;
  print_int((int)d); print_line();
  char* s = "xy";
  int k = 0;
  while (s[k] != 0) { k++; }
  print_int(k); print_line();
  return 0;
}
