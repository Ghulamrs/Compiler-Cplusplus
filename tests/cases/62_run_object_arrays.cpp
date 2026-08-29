// An array of objects is n objects: each constructed, each destroyed, in
// reverse.  Every dimension counts -- g[2][2] is four of them.
void print_int(int n);
void print_string(char* s);
void print_line();
class Cell {
public:
  int v;
  Cell() { v = 1; print_string("+"); }
  ~Cell() { print_string("-"); }
  int get() { return v; }
};
int main() {
  {
    Cell a[3];
    Cell g[2][2];
    print_line();
    a[1].v = 9;
    g[1][1].v = 8;
    print_int(a[0].get()); print_int(a[1].get()); print_int(g[1][1].v); print_int(g[0][0].v);
    print_line();
  }
  print_line();
  return 0;
}
