// All three positions of const on a pointer, and what each one freezes.
//
//   const int* p        the POINTEE   -- p may move, *p may not be written
//   int* const p        the POINTER   -- p may not move, *p may be written
//   const int* const p  both
void print_int(int n);
void print_line();
int main() {
  int a = 1;
  int b = 2;

  const int* p = &a;
  print_int(*p);
  p = &b;                    /* the pointer is free */
  print_int(*p);

  int* const q = &a;
  print_int(*q);
  *q = 7;                    /* the pointee is free */
  print_int(*q);

  const int* const r = &b;
  print_int(*r);             /* only reading */

  int c = 3;
  const int* s = &c;         /* adding const on the way in is fine */
  print_int(*s);
  print_line();
  return 0;
}
