void print_int(int n);
void print_line();
int main() {
  int  a = 10;
  int* p = &a;
  print_int(*p);          print_line();
  *p = 20;
  print_int(a);           print_line();
  char* s = "abc";
  print_int(s[0]);        print_line();
  print_int(s[2]);        print_line();
  print_int(*(s + 1));    print_line();
  char* t = s + 2;
  long  d = t - s;
  print_int(d);           print_line();
  return 0;
}
