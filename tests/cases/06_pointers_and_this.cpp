class Node {
public:
  int value;
  Node* next;
  int get() { return this->value; }
};
int main() {
  Node n;
  n.value = 7;
  Node* p = &n;
  return p->value + n.get();
}
