class Point { public: int x; };
int main() {
  Point p;
  p.x = 1;
  int &r = p.x;
  r = 5;
  return r;
}
