// operator[] both ways: by value, and returning T& so the subscript can be
// assigned to.  a[i] is a node now rather than sugar for *(a+i), because a
// class may overload it and the parser does not know types.
void print_int(int n); void print_char(int c); void print_line();
class Str {
private:
  char buf[8];
  int  n;
public:
  Str() { n = 0; }
  void push(char c) { buf[n] = c; n = n + 1; }
  int  size() { return n; }
  char  operator[](int i)      { return buf[i]; }
};
class Table {
private:
  int cells[4];
public:
  Table() { for (int i = 0; i < 4; i++) { cells[i] = 0; } }
  int& operator[](int i) { return cells[i]; }     /* reference: assignable */
};
int main() {
  Str s;
  s.push('a'); s.push('b'); s.push('c');
  print_int(s.size());
  print_char(s[0]); print_char(s[1]); print_char(s[2]);
  print_line();
  Table t;
  t[1] = 42;
  t[2] = t[1] + 1;
  print_int(t[0]); print_int(t[1]); print_int(t[2]);
  print_line();
  return 0;
}
