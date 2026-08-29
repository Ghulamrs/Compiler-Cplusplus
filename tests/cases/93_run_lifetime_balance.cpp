// Every construction pairs with a destruction.  One counter, incremented in
// every constructor and decremented in every destructor, asserted back to zero
// after each shape of object lifetime -- which is the test that would have
// caught by-value parameters, return temporaries, member arrays and globals in
// one go, since none of them were being destroyed.
void print_int(int n);
void print_string(char* s);
void print_line();
int live = 0;
class T {
public:
  int id;
  T()           { id = 0;        live = live + 1; }
  T(int n)      { id = n;        live = live + 1; }
  T(const T& o) { id = o.id + 100; live = live + 1; }
  ~T()          { live = live - 1; }
};
class Holder {
public:
  T one;
  T many[2];
  int tag;
  Holder() { tag = 5; }
};
int byValue(T t) { return t.id; }
T    make(int n) { T t(n); return t; }
int main() {
  print_string("start ");   print_int(live); print_line();
  { T a(5); print_string("byvalue ");  print_int(byValue(a)); print_line(); }
  print_string("after ");   print_int(live); print_line();
  { T b = make(7);          print_string("returned "); print_int(b.id); print_line(); }
  print_string("after ");   print_int(live); print_line();
  { Holder h;               print_string("holder ");   print_int(h.tag); print_line(); }
  print_string("after ");   print_int(live); print_line();
  { T arr[3];               print_string("array ");    print_int(arr[0].id); print_line(); }
  print_string("after ");   print_int(live); print_line();
  return 0;
}
