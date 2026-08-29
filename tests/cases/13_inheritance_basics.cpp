class Shape {
public:
  int x;
  int y;
protected:
  int tag;
};
class Box : public Shape {
public:
  int w;
  int h;
  int area() { return w * h; }
  int useTag() { return tag; }
};
int main() {
  Box b;
  b.x = 1;
  b.w = 2;
  b.h = 3;
  return b.area() + b.useTag();
}
