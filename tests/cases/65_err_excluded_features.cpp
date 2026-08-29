// Every excluded feature names itself.  A program reaching for one should be
// told which, not handed a parse error about a token.
#ifdef SOMETHING
namespace N { int x; }
typedef int myint;
enum Color { Red };
union U { int i; };
template<class T> T id(T v) { return v; }
int f(int a, int b = 2) { return a + b; }
int g(int a, ...) { return a; }
class A { public: int x; static int n;
           class Inner { public: int y; };
           int operator[](int i) { return x; } };
class B { public: int y; };
class C : public A, public B { };
int main() {
  try { throw 1; } catch (int e) { }
  long long big = 1;
  int (*fp)(int) = 0;
  goto done;
done:
  return sizeof(int);
}
