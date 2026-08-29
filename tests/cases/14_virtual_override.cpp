class Shape {
public:
  int side;
  virtual int area() { return 0; }
  virtual int perimeter() { return 0; }
  int name() { return 1; }
};
class Square : public Shape {
public:
  int area() { return side * side; }
};
class Cube : public Square {
public:
  int area() { return side * side * side; }
  virtual int volume() { return side * side * side; }
};
int main() {
  Cube c;
  c.side = 3;
  Shape* s = &c;
  return s->area() + s->perimeter() + c.volume();
}
