class Shape {
public:
  int side;
  Shape(int s) : side(s) { }
  virtual int area() { return 0; }
  virtual ~Shape() { }
};
class Square : public Shape {
public:
  int extra;
  Square(int s) : Shape(s), extra(1) { }
  int area() { return side * side; }
  ~Square() { }
};
int main() {
  Square sq(4);
  Shape* s = &sq;
  int direct = sq.area();
  int virt = s->area();
  Shape* heap = new Square(5);
  delete heap;
  return direct + virt;
}
