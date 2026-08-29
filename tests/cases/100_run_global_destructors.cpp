// A global object is constructed before main and destroyed after it, because
// no scope in the program owns it.
void print_int(int n); void print_string(char* s); void print_line();
class G { public: int v; G(){v=1; print_string("ctor ");} ~G(){ print_string("dtor"); print_line(); } };
G g;
int main(){ print_int(g.v); print_line(); return 0; }
