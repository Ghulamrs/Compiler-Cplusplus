// A 4-byte float parameter has to be narrowed on the way into the frame.
void print_double(double d);
void print_line();
double idf(float f) { return f; }
float scale(float f) { return f * 2.0f; }
int main() {
  print_double(idf(1.5));    print_line();
  print_double(scale(1.25)); print_line();
  return 0;
}
