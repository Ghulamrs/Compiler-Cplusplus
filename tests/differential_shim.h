#include <iostream>
#include <string>
#include <cmath>
#include <cstdlib>
using namespace std;
// One print_int, taking int, because that is the only signature the corpus
// ever declares -- `void print_int(int n);` heads 58 of the cases and
// `void print_int(long n);` heads none.  A second overload taking long added
// nothing and made three calls ambiguous that are not ambiguous in the case
// itself: print_int(unsigned/3) and print_int(someDouble) each convert equally
// well to int and to long, so the host refused to compile what this compiler
// resolves against the one declaration it was given.
void print_int(int n){ std::cout << n; }
void print_char(int c){ std::cout << static_cast<char>(c); }
void print_char(char c){ std::cout << c; }
void print_double(double d){ std::cout << d; }
void print_string(char* s){ if(s) std::cout << s; }
void print_string(const char* s){ if(s) std::cout << s; }
void print_line(){ std::cout << std::endl; }
void err_int(int n){ std::cerr << n; }
void err_char(int c){ std::cerr << static_cast<char>(c); }
void err_double(double d){ std::cerr << d; }
void err_string(char* s){ if(s) std::cerr << s; }
void err_string(const char* s){ if(s) std::cerr << s; }
void err_line(){ std::cerr << std::endl; }
void print_pointer(void* p){ std::cout << "PTR"; (void)p; }
void err_pointer(void* p){ std::cerr << "PTR"; (void)p; }
long read_int(){ long v=0; if(!(std::cin>>v)) return 0; return v; }
double read_double(){ double v=0; if(!(std::cin>>v)) return 0; return v; }
int read_char(){ int c=std::cin.get(); return c<0?0:c; }
void read_string(char* s,int max){ std::string t; if(std::cin>>t){ int n=(int)t.size(); if(n>max-1)n=max-1; for(int i=0;i<n;++i)s[i]=t[i]; s[n]=0;} }
void read_line(char* s,int max){ std::string t; if(std::getline(std::cin,t)){ int n=(int)t.size(); if(n>max-1)n=max-1; for(int i=0;i<n;++i)s[i]=t[i]; s[n]=0;} }
int input_good(){ return std::cin.good()?1:0; }
