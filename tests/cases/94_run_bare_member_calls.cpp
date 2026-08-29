// A bare call to your own method IS this->method(): it dispatches virtually,
// picks its overload by argument type, and obeys const.  Resolving it on a
// path of its own gave three different wrong answers.
void print_int(int n); void print_line();
class B { public: virtual int tag() { return 1; } virtual ~B(){}
          int bare() { return tag(); }            /* no this-> */
          int viaThis() { return this->tag(); } };
class D : public B { public: virtual int tag() { return 100; } };
int main(){ D d; B* p = &d; print_int(p->bare()); print_int(p->viaThis()); print_line(); return 0; }
