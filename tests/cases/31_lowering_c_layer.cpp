int add(int a, int b) { return a + b; }
int main() {
  int x = 2;
  int y = add(x, 3);
  int z = 0;
  if (y > 4 && x != 0) { z = 1; }
  for (int i = 0; i < 3; i = i + 1) {
    if (i == 2) { break; }
    z = z + i;
  }
  while (z > 0) { z = z - 1; }
  return z;
}
