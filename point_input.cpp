class Point {
public:
  int x;
};

int main() {
  Point p;
  p.x = 1;
  int &r = p.x;   // OK: p.x is an lvalue of type int
  int &s = 1;     // Error: initializer is not an lvalue
  return r;
}
