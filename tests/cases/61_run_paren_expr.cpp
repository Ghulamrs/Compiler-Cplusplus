// A parenthesised identifier is an expression unless the name is a type.
void print_int(int n);
void print_line();
int ident(int x) { return (x); }
int main() {
  int x = 7;
  print_int((x) + 1);   print_line();
  print_int(ident(5));  print_line();
  int y = (x);
  print_int(y);         print_line();
  print_int((x) * (y)); print_line();
  return 0;
}
