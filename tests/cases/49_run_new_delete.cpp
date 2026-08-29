void print_int(int n);
void print_string(char* s);
void print_line();
class Node {
public:
  int value;
  Node* next;
  Node(int v) : value(v) { next = 0; }
  virtual int get() { return value; }
  virtual ~Node() { print_string("~Node "); }
};
class Doubled : public Node {
public:
  Doubled(int v) : Node(v) { }
  int get() { return value * 2; }
  ~Doubled() { print_string("~Doubled "); }
};
int main() {
  Node* a = new Node(5);
  Node* b = new Doubled(5);
  print_int(a->get()); print_int(b->get()); print_line();

  /* a linked list on the heap */
  Node* head = new Node(1);
  head->next = new Node(2);
  head->next->next = new Node(3);
  int total = 0;
  Node* p = head;
  while (p != 0) { total += p->value; p = p->next; }
  print_string("sum="); print_int(total); print_line();

  print_string("deleting: ");
  delete b;
  delete a;
  print_line();

  /* reuse: freeing then allocating should not grow the heap forever */
  int i = 0;
  while (i < 20) { Node* t = new Node(i); delete t; i++; }
  print_string("20 new/delete cycles survived"); print_line();
  return total;
}
