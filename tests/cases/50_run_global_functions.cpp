void print_int(int n);
void print_line();

int  isOdd(int n);                 /* forward declaration */
int  fact(int n) { if (n <= 1) { return 1; } return n * fact(n - 1); }
int  isEven(int n) { if (n == 0) { return 1; } return isOdd(n - 1); }
int  isOdd(int n)  { if (n == 0) { return 0; } return isEven(n - 1); }
double area(double r) { return 3.14159 * r * r; }
int  maxOf(int a, int b) { if (a > b) { return a; } return b; }
int  maxOf(int a, int b, int c) { return maxOf(maxOf(a, b), c); }
void shout() { print_int(999); print_line(); }
int  counter = 0;
void bump() { counter++; }

int main() {
  print_int(fact(6));            print_line();
  print_int(isEven(10));         print_int(isOdd(10));   print_line();
  print_int(maxOf(3, 9));        print_int(maxOf(3, 9, 5)); print_line();
  print_int(area(2.0));          print_line();
  shout();
  bump(); bump(); bump();
  print_int(counter);            print_line();
  return 0;
}
