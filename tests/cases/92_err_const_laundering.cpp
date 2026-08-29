// const may be added by a conversion, never removed -- otherwise one
// assignment would defeat every check above it.
void takesPlain(int* q);
int main() {
  int a = 1;
  int b = 2;
  const int* p = &a;
  int* q = p;                /* dropping const in an initialiser */
  takesPlain(p);             /* dropping const at a call         */
  const int* const r = &a;
  r = &b;                    /* the pointer is const             */
  *r = 9;                    /* the pointee is const             */
  int* const s = &a;
  s = &b;                    /* the pointer is const             */
  return 0;
}
