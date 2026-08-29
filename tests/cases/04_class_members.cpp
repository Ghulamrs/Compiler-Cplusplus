class Point {
public:
  int x;
  int y;
  int getX() { return x; }
  int sum() { return x + y; }
};
int main() {
  Point p;
  p.x = 3;
  p.y = 4;
  return p.getX() + p.sum();
}
