// Deleting the same pointer twice used to put the free list into a loop that
// pointed at itself, and the next allocation too big to be satisfied from that
// block then followed `next` round it forever, inside allocate() -- where the
// interpreter's step limit does not reach.  The process had to be killed.
//
// The larger second allocation is the part that matters: a request the freed
// block CAN satisfy returns on the first iteration and never follows the
// cycle, which is why the obvious two-line repro missed this.
class Big {
public:
  int a; int b; int c; int d; int e; int f; int g; int h;
};

int main() {
  int *p = new int;
  *p = 1;
  delete p;
  delete p;
  Big *q = new Big;
  q->a = 2;
  delete q;
  return 0;
}
