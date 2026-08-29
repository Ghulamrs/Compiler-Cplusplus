// Arrays nest to any depth, as locals and as fields alike.
void print_int(int n);
void print_line();
class Grid { public: int cell[2][3]; int n; Grid() { n = 6; } };
int sum(int* p, int n) { int t = 0; for (int i = 0; i < n; i++) { t += p[i]; } return t; }
int main() {
  int a[4];             a[2] = 7;
  int b[2][3];          b[1][2] = 42;
  int c[2][3][4];       c[1][2][3] = 99;
  int d[2][2][2][2];    d[1][1][1][1] = 5;
  print_int(a[2]); print_int(b[1][2]); print_int(c[1][2][3]); print_int(d[1][1][1][1]);
  print_line();
  Grid g;
  g.cell[1][2] = 8;
  print_int(g.cell[1][2]); print_int(g.n);
  print_line();
  int m[2][3];
  for (int i = 0; i < 2; i++) { for (int j = 0; j < 3; j++) { m[i][j] = i * 3 + j; } }
  print_int(sum(m[0], 3)); print_int(sum(m[1], 3));
  char* names[2];
  names[0] = "ab"; names[1] = "cd";
  print_int(names[1][0]);
  print_line();
  return 0;
}
