// Two shapes a beginner writes constantly, each of which used to cost more
// lines than it was worth -- and in one case never said what was actually
// wrong.
//
// `vector<int> v;` is a template instantiation, which this version does not
// have.  Nothing recognised it: the '<' was read as a comparison, so the type
// never existed and what came out was four errors about expressions, not one
// about templates.  It has to be a probe, because `a < b` looks identical up
// to the '<'; what separates them is a name being declared after the matching
// '>', which is the same test `Widget w;` already relied on.
//
// `int rows, cols;` is one rule broken once.  As a local it has said so for a
// while.  As a FIELD it said "expected ';' after field rows, found ','", which
// names the punctuation instead of the rule, and then read the rest of the
// list as declarations of its own.
//
// What follows each is one more line -- the name that was skipped is genuinely
// not declared -- and that one stays.  It is a true statement about the
// program, not the parser losing its place.

#include <iostream>

class Holder {
public:
    int rows, cols;                 // one line: the rule, named
};

int main() {
    std::vector<int> v;             // one line: templates, named
    map<int,int> m;                 // and again without the qualifier

    // Still comparisons, not templates: nothing is declared after them.
    int a = 1;
    int b = 2;
    cout << (a < b) << endl;
    cout << (b > a) << endl;

    return 0;
}
