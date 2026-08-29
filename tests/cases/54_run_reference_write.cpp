// A store through a reference is as wide as the referent, not as wide as the
// reference.  An 8-byte store here overwrites whatever sits next door.
void print_int(int n);
void print_line();
void set(int &r) { r = 5; }
int main() {
  int a = 1;
  int b = 2;
  set(a);
  print_int(a); print_int(b); print_line();
  int arr[3];
  arr[0] = 7; arr[1] = 8; arr[2] = 9;
  set(arr[1]);
  print_int(arr[0]); print_int(arr[1]); print_int(arr[2]); print_line();
  int &r = a;
  r = 11;
  print_int(a); print_int(b); print_line();
  return 0;
}
