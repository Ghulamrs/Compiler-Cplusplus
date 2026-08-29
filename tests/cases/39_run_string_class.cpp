void print_string(char* s);
void print_int(int n);
void print_line();

/* A String written IN this language: classes, ctor/dtor, new/delete and
   char* are enough, without operator overloading. */
class String {
public:
  char* data;
  int   len;

  String(char* s) {
    data = s;
    len = 0;
    while (data[len] != 0) { len = len + 1; }
  }
  int length() { return len; }
  char at(int i) { return data[i]; }
  virtual ~String() { }
};

int main() {
  String s("compiler");
  print_string("text:   "); print_string(s.data);   print_line();
  print_string("length: "); print_int(s.length());  print_line();
  print_string("s[0]:   "); print_int(s.at(0));     print_line();
  String* heap = new String("heap");
  print_string("heap:   "); print_int(heap->length()); print_line();
  delete heap;
  return s.length();
}
