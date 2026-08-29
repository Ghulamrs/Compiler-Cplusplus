int main() {
  int total = 0;
  int i = 0;
  while (i < 10) {
    if (i % 2 == 0) { total = total + i; }
    else { total = total - i; }
    i = i + 1;
  }
  for (int j = 0; j < 3; j = j + 1) {
    if (j == 2) { break; }
    total = total + j;
  }
  return total;
}
