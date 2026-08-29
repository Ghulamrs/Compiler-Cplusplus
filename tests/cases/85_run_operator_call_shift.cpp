// operator<< and operator() on a class of its own.
void print_int(int n); void print_line();
class Acc { public: int v; Acc(){v=0;} Acc operator<<(int n){ v = v + n; return *this; } int operator()(int k){ return v * k; } };
int main(){ Acc a; a = a << 3 << 4; print_int(a.v); print_int(a(2)); print_line(); return 0; }
