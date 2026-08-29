// break and continue leave a scope, so they run its destructors.
void print_string(char* s);
void print_line();
class T {
public:
  T()  { print_string("+"); }
  ~T() { print_string("-"); }
};
int main() {
  for (int i = 0; i < 3; i = i + 1) {
    T t;
    if (i == 0) { continue; }
    if (i == 2) { break; }
  }
  print_line();
  return 0;
}
