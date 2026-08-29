// Copying a derived object into a base copies its bytes, vptr included.  The
// copy must be the class it was declared as, or it dispatches into one it is
// not -- so the vptr is rewritten after the copy.
void print_int(int n); void print_line();
class V { public: int a; V(){a=0;} virtual int f(){return 10;} virtual ~V(){} };
class D2 : public V { public: D2(){a=0;} virtual int f(){return 20;} };
int main(){ D2 d; V& rv = d; V c = rv;
  print_int(c.f());          /* named object: direct dispatch */
  V* q = &c;
  print_int(q->f());         /* through a pointer: uses the vptr */
  print_line(); return 0; }
