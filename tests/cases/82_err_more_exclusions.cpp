// The remaining exclusions, each naming itself rather than failing as a
// parse error about a token.
class A {
public:
  int x : 3;
  mutable int m;
};
inline int f() { return 1; }
int main() {
  int a[3] = {1, 2, 3};
  double d = 1.5;
  int n = static_cast<int>(d);
  wchar_t w = 0;
  return n;
}
