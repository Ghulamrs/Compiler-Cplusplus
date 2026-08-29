// The container idiom: T& operator[](int) beside T operator[](int) const.
// They are two functions, so they need two signatures and two symbols; and a
// friend granted to peek(const A&) is not granted to peek(A&).
void print_int(int n); void print_line();
class A {
private:
  int d[3];
public:
  A(){d[0]=1;d[1]=2;d[2]=3;}
  int& operator[](int i)       { return d[i]; }   /* the pair */
  int  operator[](int i) const { return d[i]; }
  friend int peek(const A& a);
};
int peek(const A& a) { return a.d[0]; }     /* granted   */

int main(){ A m; m[0] = 42; print_int(m[0]);
  const A c; print_int(c[1]);
  print_int(peek(m));
  print_line(); return 0; }
