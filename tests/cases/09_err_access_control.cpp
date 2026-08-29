class Point {
public:
  int x;
private:
  int secret;
};
int main() {
  Point p;
  p.secret = 1;
  return p.x;
}
