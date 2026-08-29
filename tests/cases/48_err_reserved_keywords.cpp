template <class T> T id(T x) { return x; }
namespace N { int a; }
class A { static int s; friend class B; };
int main() {
  try { throw 1; } catch (int e) { }
  static int k;
  goto done;
  int n = sizeof(int);
  enum Color { Red };
  union U { int a; };
  typedef int myint;
  const_cast<int>(1);
  done: return 0;
}
