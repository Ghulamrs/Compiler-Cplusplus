// Every way of writing to a const, each refused.
class A { public: int x; A() { x = 1; } };
int writeThroughRef(const A& a) { a.x = 99; return 0; }
int writeParam(const int n) { n = 5; return n; }
int main() {
  const int k = 10;
  k = 1;
  k++;
  --k;
  const A ca;
  ca.x = 3;
  const int missing;
  const A* cp = &ca;
  cp->x = 4;                       /* writing through a const A* */
  const char* s = "ab";
  s[0] = 88;                       /* writing through a const char* */
  return 0;
}
