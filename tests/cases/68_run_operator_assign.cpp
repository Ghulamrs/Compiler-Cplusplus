// A declared operator= is the assignment; without one the copy is memberwise.
void print_int(int n);
void print_string(char* s);
void print_line();
class Acc {
public:
  int v;
  Acc() { v = 0; }
  Acc operator=(Acc o) { v = o.v + 1000; print_string("assign "); return o; }
};
class Plain { public: int v; Plain(){v=0;} };
int main() {
  Acc a; a.v = 5;
  Acc b;
  b = a;
  print_int(b.v); print_line();
  Plain p; p.v = 7;
  Plain q;
  q = p;
  print_int(q.v); print_line();
  return 0;
}
