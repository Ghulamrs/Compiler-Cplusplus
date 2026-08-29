class Base {
public:
  int open;
private:
  int secret;
};
class Derived : public Base {
public:
  int leakUnqualified() { return secret; }
  int leakQualified() { return this->secret; }
};
int main() {
  Derived d;
  return d.open;
}
