// What new[] and delete[] refuse.

class Plain {
public:
    int n;
    Plain(int x) { n = x; }         // no default constructor
};

class Ok {
public:
    int n;
    Ok() { n = 0; }
};

int main() {
    // Every element gets the same constructor and there is nowhere to write
    // arguments for each, so the array form takes none.
    Ok *a = new Ok[4](1);

    // The elements are default-constructed, so a class without a default
    // constructor cannot be allocated as an array.
    Plain *b = new Plain[4];

    // A count is a number of elements, so it has to be a whole one.
    double d = 2.5;
    int *c = new int[d];

    // delete[] wants a pointer, like delete.
    int x = 1;
    delete[] x;

    return 0;
}
