void print_int(int n);
void print_string(char* s);
void print_line();
class Point {
public:
  int x;
  int y;
  Point(int a, int b);
  int  sum();
  int  scaled(int k);
  virtual int area();
  virtual ~Point();
};
Point::Point(int a, int b) : x(a), y(b) { }
int Point::sum() { return x + y; }
int Point::scaled(int k) { return sum() * k; }
int Point::area() { return x * y; }
Point::~Point() { print_string("~Point "); }

class Cube : public Point {
public:
  Cube(int a) : Point(a, a) { }
  int area();
  ~Cube() { print_string("~Cube "); }
};
int Cube::area() { return x * y * x; }

int main() {
  Point p(3, 4);
  print_int(p.sum());       print_line();
  print_int(p.scaled(2));   print_line();
  Cube c(2);
  Point* base = &c;
  print_int(base->area());  print_line();
  print_string("exit: ");
  return 0;
}
