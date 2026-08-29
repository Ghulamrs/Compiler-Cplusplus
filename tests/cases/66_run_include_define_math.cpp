// #include is accepted and ignored -- there is no header to read, but every
// program a student has seen opens with one.  #define is a substitution.
// The maths comes from natives, declared without a body like everything else.
#include <cmath>
#include <cmath>
#define PI 3.14159265358979
#define TWO 2
#define TWO_PI (TWO * PI)
#define LABEL "PI stays text in here"

double sqrt(double x);
double pow(double b, double e);
double atan2(double y, double x);
double fabs(double x);
double floor(double x);
int abs(int n);
void print_double(double d);
void print_int(int n);
void print_string(char* s);
void print_line();

int main() {
  print_double(PI);                     print_line();
  print_int(TWO);                       print_line();
  print_double(TWO_PI);                 print_line();
  print_string(LABEL);                  print_line();
  print_double(sqrt(16.0));             print_line();
  print_double(pow(2.0, 10.0));         print_line();
  print_double(atan2(0.0, 1.0));        print_line();
  print_double(fabs(-2.5));             print_line();
  print_double(floor(3.7));             print_line();
  print_int(abs(-7));                   print_line();
  print_double(sqrt(pow(3.0,2.0) + pow(4.0,2.0)));  print_line();
  print_double(PI * pow(2.0, 2.0));     print_line();
  return 0;
}
