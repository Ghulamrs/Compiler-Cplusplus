// A friend reads the private parts, whether it is declared here and defined
// at file scope, or defined inline and hoisted there.
void print_int(int n);
void print_line();
class Box {
private:
  int secret;
public:
  Box() { secret = 42; }
  friend int peek(Box b);
  friend int twice(Box b) { return b.secret * 2; }
};
int peek(Box b) { return b.secret; }
int main() {
  Box b;
  print_int(peek(b));
  print_int(twice(b));
  print_line();
  return 0;
}
