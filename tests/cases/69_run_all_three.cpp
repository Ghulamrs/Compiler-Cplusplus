// The three additions working together, on top of everything else: macros,
// maths natives, overloaded operators returning objects, and arrays of
// objects with object members.
#include <cmath>
#include <cmath>
#define PI 3.14159265358979

double sqrt(double x);
double pow(double b, double e);
void print_double(double d);
void print_int(int n);
void print_string(char* s);
void print_line();

class Vec {
public:
  double x;
  double y;
  Vec() { x = 0.0; y = 0.0; }
  Vec operator+(Vec o) { Vec r; r.x = x + o.x; r.y = y + o.y; return r; }
  Vec operator*(double k) { Vec r; r.x = x * k; r.y = y * k; return r; }
  bool operator==(Vec o) { return x == o.x && y == o.y; }
  double length() { return sqrt(pow(x, 2.0) + pow(y, 2.0)); }
};

class Circle {
public:
  Vec centre;
  double r;
  Circle() { r = 1.0; }
  double area() { return PI * pow(r, 2.0); }
};

int main() {
  Vec a; a.x = 3.0; a.y = 4.0;
  Vec b; b.x = 1.0; b.y = 2.0;
  print_double(a.length());          print_line();
  Vec c = a + b;
  print_double(c.x); print_double(c.y); print_line();
  Vec d = (a + b) * 2.0;
  print_double(d.x); print_double(d.y); print_line();
  print_int(a == b);                 print_line();
  Circle circles[3];
  for (int i = 0; i < 3; i++) { circles[i].r = i + 1; }
  for (int i = 0; i < 3; i++) { print_double(circles[i].area()); print_line(); }
  circles[1].centre = a;
  print_double(circles[1].centre.length()); print_line();
  return 0;
}
