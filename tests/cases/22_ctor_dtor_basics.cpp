class Point {
public:
  int x;
  int y;
  Point() : x(0), y(0) { }
  Point(int a, int b) : x(a), y(b) { }
  ~Point() { }
  int sum() { return x + y; }
};
int main() {
  Point p;
  Point q(1, 2);
  return p.sum() + q.sum();
}
