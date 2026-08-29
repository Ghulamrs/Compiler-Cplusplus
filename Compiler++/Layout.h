// Layout.h
//
// PASS 4 -- the object model.  Where does each field sit inside an object, how
// big is an object, and which function does a virtual call actually reach?
//
// This is the first pass that thinks about machine representation rather than
// meaning, and it is where the decision to allow only SINGLE inheritance pays
// for itself.  With one base, the rules fit in a paragraph:
//
//   * A derived object BEGINS with its base subobject.  So the address of a
//     Derived is also the address of its Base, and the upcast the semantic
//     pass allows costs nothing at all at runtime -- no pointer adjustment.
//   * If a class or any of its bases has a virtual function, the object starts
//     with a hidden pointer to its class's vtable.  Because the base subobject
//     is at offset 0, a base that already has a vptr SHARES it with the derived
//     class; only a class that introduces the first virtual function in its
//     chain adds one.
//   * A vtable is its base's vtable, copied, with overridden slots replaced and
//     newly introduced virtuals appended.  So a slot index means the same thing
//     in every class of the chain, which is exactly what makes a virtual call
//     "load the vptr, index by a constant, call".
//
// Multiple inheritance would break all three of those sentences at once.
//
// C++98 only.

#ifndef LAYOUT_H
#define LAYOUT_H

#include <map>
#include <string>
#include <vector>

#include "AST1.h"
#include "Diagnostics.h"

struct FieldLayout {
    std::string name;
    std::string ownerClass;     // the class that declared it
    cc::Type *type;             // not owned
    int offset;
    int size;
};

struct ClassLayout {
    std::string name;
    int size;                   // bytes, including the vptr and any padding
    int align;
    bool hasVPtr;
    int firstOwnField;          // index into `fields` where this class's own start
    std::vector<FieldLayout> fields;             // base fields first
    std::vector<cxx::MethodDecl*> vtable;        // slot -> final override
    ClassLayout() : size(0), align(1), hasVPtr(false), firstOwnField(0) {}
};

class Layout {
public:
    explicit Layout(Diagnostics &d);

    // Computes a layout for every class, base classes first.
    void computeAll(const std::map<std::string, cxx::ClassDecl*> &classes);

    const ClassLayout *forClass(const std::string &name) const;

    // Byte size of any type; 0 and a diagnostic for something with no size.
    int sizeOf(cc::Type *t) const;
    int alignOf(cc::Type *t) const;

    void print() const;

    // The sizes this compiler targets.  A teaching VM, so they are simply
    // stated rather than probed from a host machine.
    static const int PointerSize = 8;
    static const int IntSize = 4;
    static const int CharSize = 1;
    static const int BoolSize = 1;

private:
    Diagnostics &diag;
    std::map<std::string, ClassLayout> layouts;

    // Depth-first, so a base is always laid out before anything derived from it.
    void computeFor(cxx::ClassDecl *cd);
    static int roundUp(int value, int alignment);

    Layout(const Layout &);
    Layout &operator=(const Layout &);
};

#endif
