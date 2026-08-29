int add(int a, int b) { return a + b; }
void nothing() { return; }
int main() {
  int a = add(1);
  int b = add(1, 2, 3);
  nothing();
  return nothing();
}
