// A friend is granted by name; anyone else is still refused.  And the forms
// this version does not take are named rather than misparsed.
void print_int(int n);
class Box {
private:
  int secret;
public:
  Box() { secret = 1; }
  friend int peek(Box b);
  friend class Other;
};
int peek(Box b) { return b.secret; }
int nosy(Box b) { return b.secret; }
class V { public: int x; int operator[](int i) { return x; } };
int main() { Box b; print_int(nosy(b)); return 0; }
