// Friendship is granted by name AND signature: an overload of a granted
// name is a different function, and is granted nothing.
void print_int(int n);
class Box {
private:
  int secret;
public:
  Box() { secret = 7; }
  friend int peek(Box b);              /* ONLY this one is granted */
};
int peek(Box b) { return b.secret; }             /* legal   */
int peek(Box b, int extra) { return b.secret; }  /* NOT granted -- must fail */
int main() { Box b; print_int(peek(b, 1)); return 0; }
