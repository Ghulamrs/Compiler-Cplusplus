// `delete` of a pointer that is not what `new` returned.
//
// Being INSIDE the heap was taken as being a block, so deleting a pointer to a
// field wrote a free-list header over that field: a size and a next made of
// the program's own data, which the next allocation then followed.  It could
// hand back a pointer aliasing a live object, or one below the heap entirely.
// Neither reaches memory outside the machine, but the heap stops meaning
// anything, and nothing said so.
//
// The blocks are walked from the bottom of the heap now -- each says how long
// it is -- and only a block START is something `new` returned.

void print_string(char *s);
void print_line();

class Pair {
public:
    int a;
    int b;
};

int main() {
    Pair *p = new Pair;
    p->a = 33;
    p->b = 44;

    print_string("allocated");
    print_line();

    // &p->b is inside the block, but it is not the block.
    delete &p->b;

    print_string("unreachable");
    print_line();
    return 0;
}
