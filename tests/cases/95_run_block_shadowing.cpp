// An inner block may shadow an outer name, and the outer one comes back when
// the block ends.  Lowering used to erase the name instead of restoring it.
void print_int(int n); void print_line();
int main(){ int x=10; print_int(x); { int x=99; print_int(x); { int x=7; print_int(x); } print_int(x); } print_int(x); print_line(); return 0; }
