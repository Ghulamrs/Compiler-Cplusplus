// Every maths native, called the way the language provides them: declared
// without a body, which IS the binding.  There is no <cmath> to read.
//
// The values are chosen to be exact in binary floating point, so this case
// tests the natives rather than the printer's rounding.
#include <iostream>
double sqrt(double);  double sin(double);   double cos(double);  double tan(double);
double asin(double);  double acos(double);  double atan(double);
double atan2(double, double);
double sinh(double);  double cosh(double);  double tanh(double);
double pow(double, double);  double fabs(double);
double floor(double); double ceil(double);  double fmod(double, double);
double trunc(double); double round(double);
double log(double);   double log10(double); double exp(double);
int abs(int);

int main() {
    cout << sqrt(16.0) << " " << pow(2.0, 10.0) << " " << fabs(-3.5) << endl;
    cout << sin(0.0) << " " << cos(0.0) << " " << tan(0.0) << endl;
    cout << asin(0.0) << " " << acos(1.0) << " " << atan(0.0) << " " << atan2(0.0, 1.0) << endl;
    cout << sinh(0.0) << " " << cosh(0.0) << " " << tanh(0.0) << endl;
    cout << floor(2.7) << " " << ceil(2.1) << endl;
    // `%` needs integer operands, so fmod is the only floating remainder.
    cout << fmod(7.5, 2.0) << " " << fmod(-7.5, 2.0) << endl;
    // trunc goes toward zero, round goes away from it -- which is the whole
    // difference between them and floor/ceil on a negative number.
    cout << trunc(2.7) << " " << trunc(-2.7) << endl;
    cout << round(2.4) << " " << round(2.5) << " " << round(-2.5) << endl;
    cout << log(1.0) << " " << log10(100.0) << " " << exp(0.0) << endl;
    cout << abs(-7) << endl;
    return 0;
}
