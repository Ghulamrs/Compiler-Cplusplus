int add(int a, int b) { return a + b; }
int twice(int x) { return add(x, x); }
int main() { return twice(add(1, 2)); }
