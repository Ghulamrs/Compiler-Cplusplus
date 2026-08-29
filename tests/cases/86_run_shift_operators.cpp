// << and >> as real integer operators: they bind tighter than a comparison
// and looser than + and -, and >> keeps the sign.
void print_int(int n); void print_line();
int main() {
  print_int(1 << 4);
  print_int(256 >> 3);
  print_int(-16 >> 2);
  print_int(1 << 2 << 1);
  print_int(2 + 1 << 3);
  print_int((1 << 3) > 4);
  print_line();
  return 0;
}
