void print_int(int n);
void print_double(double d);
void print_char(char c);
void print_string(char* s);
void print_line();
int main() {
  int    i = 7;
  double d = 2.5;
  float  f = 1.5f;
  char   c = 'A';
  unsigned int u = 10;
  short  sh = 300;

  print_string("i/2   = "); print_int(i / 2);        print_line();
  print_string("i%2   = "); print_int(i % 2);        print_line();
  print_string("u/3   = "); print_int(u / 3);        print_line();
  print_string("i+d   = "); print_double(i + d);     print_line();
  print_string("f*2.0 = "); print_double(f * 2.0);   print_line();
  print_string("d>i   = "); print_int(d > i);        print_line();
  print_string("char  = "); print_char(c);           print_line();
  print_string("c+1   = "); print_char(c + 1);       print_line();
  print_string("short = "); print_int(sh);           print_line();
  print_string("(int)d= "); print_int(d);            print_line();
  print_string("neg   = "); print_double(-d);        print_line();
  return 0;
}
