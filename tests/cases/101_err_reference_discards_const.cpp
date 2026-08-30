// A reference binds to an object rather than copying it, so the const on that
// object has to survive the binding.  Both positions must agree: the argument
// path has always rejected this, the local path used to let it through and so
// allowed a const object to be assigned to.
void takesRef(int &r);

int main() {
  const int c = 10;
  int &bad = c;             // discards const
  const int &ok = c;        // keeps it -- allowed
  int plain = 1;
  const int &alsoOk = plain;  // adding const is allowed
  takesRef(c);              // the same violation, as an argument
  return ok + alsoOk;
}
