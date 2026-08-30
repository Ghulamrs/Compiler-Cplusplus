// The other two, which are declaration forms rather than operators.
class Shape {
public:
  int tag;
  Shape() { tag = 0; }
  virtual int area() = 0;       // pure virtual
};
int total(int values[4]);       // array parameter
int main() { return 0; }
