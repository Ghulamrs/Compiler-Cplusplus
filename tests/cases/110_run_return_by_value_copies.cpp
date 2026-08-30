// `return obj;` is a copy, and a copy is a construction: the copy constructor
// has to make it.  Copying the bytes instead was a silent wrong answer for any
// class whose copy does more than move them -- and for a class that owns
// memory it was worse than wrong, because the destructors that run immediately
// after the return then free what the caller has just been handed.
//
// The result is built straight into the caller's variable, so there is no
// temporary to copy out of and none left undestroyed: the counter below
// returns to zero after every shape.

void print_int(int);
void print_string(char *s);
void print_line();

int live = 0;

// A class whose copy has to run: it owns the thing it points at.
class Res {
public:
    int *p;
    Res(int v)          { p = new int; *p = v;  live = live + 1; }
    Res(const Res &o)   { p = new int; *p = *o.p; live = live + 1; }
    ~Res()              { delete p;             live = live - 1; }
    int get()           { return *p; }
};

class Holder {
public:
    Res r;
    int k;
    Holder(int v) : r(v) { k = 9; }
};

Holder makeHolder(int v) { Holder h(v); return h; }

// A class that records the copy, so the constructor running is observable.
class Tag {
public:
    int id;
    Tag(int n)          { id = n;        live = live + 1; }
    Tag(const Tag &o)   { id = o.id + 100; live = live + 1; }
    ~Tag()              { live = live - 1; }
};

Tag makeTag(int n) { Tag t(n); return t; }

int main() {
    print_string("start ");  print_int(live);  print_line();

    {
        // The owned pointer must be the copy's own, not the dead frame's.
        Holder u = makeHolder(1);
        print_string("owned ");  print_int(u.k);  print_int(u.r.get());  print_line();
    }
    print_string("after ");  print_int(live);  print_line();

    {
        Tag t = makeTag(7);
        print_string("copied ");  print_int(t.id);  print_line();
    }
    print_string("after ");  print_int(live);  print_line();

    {
        // Straight into the variable here too: one construction, no temporary.
        Tag d = Tag(3);
        print_string("direct ");  print_int(d.id);  print_line();
    }
    print_string("after ");  print_int(live);  print_line();
    return 0;
}
