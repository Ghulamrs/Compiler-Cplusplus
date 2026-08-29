void print_int(int n);
void print_string(char* s);
void print_line();

class Shape {
public:
  int side;
  Shape(int s) : side(s) { }
  virtual int area() { return 0; }
  virtual ~Shape() { print_string("~Shape "); }
};
class Square : public Shape {
public:
  Square(int s) : Shape(s) { }
  int area() { return side * side; }
  ~Square() { print_string("~Square "); }
};
class Cube : public Square {
public:
  Cube(int s) : Square(s) { }
  int area() { return side * side * side; }
  ~Cube() { print_string("~Cube "); }
};

int main() {
  Cube c(3);
  Shape* s = &c;
  print_string("virtual through base: ");
  print_int(s->area());
  print_line();

  Shape* heap = new Square(4);
  print_string("heap dispatch: ");
  print_int(heap->area());
  print_line();
  print_string("delete runs: ");
  delete heap;
  print_line();

  print_string("scope exit runs: ");
  return s->area();
}
