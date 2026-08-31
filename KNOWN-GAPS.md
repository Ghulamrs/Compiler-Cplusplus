# Known gaps

Every entry below was reproduced against a build of this tree, not read off the
source. Each names the file and the shape of program that shows it, so a fix
starts from a failing case rather than from a description.

They are gaps, not surprises: the language this compiler accepts is a subset by
design, every exclusion is lexed, named once and skipped, and what follows is
the separate list of places where the compiler accepts a program and then does
something other than what the program says.

## What is excluded

This section used to name six exclusions — multiple inheritance, exceptions,
templates, `goto`, `sizeof`, array initialiser lists — in a way that read like
the whole list. It was not the whole list. There are about two dozen, and the
six left out the ones a reader is most likely to write by accident: `?:` and
`&` are both refused, and neither was mentioned.

The diagnostics are where this is decided. To re-derive the list:

```sh
grep -h 'not supported in this version' Compiler++/*.cpp
```

**Keywords**, from `reservedWordHelp` in `Lexer.cpp`:

| | |
|---|---|
| templates | `template` `typename` `export` |
| exceptions | `throw` `try` `catch` |
| namespaces | `namespace`, `using`, and any `a::b` — **except `std::`, which is accepted and dropped, and `using namespace std;`, which is stepped over in silence**: this language's `<iostream>` already puts `cout`, `cin` and `endl` at global scope, so both ask for what is already true |
| named casts | `static_cast` `const_cast` `dynamic_cast` `reinterpret_cast` — *use `(T)value`* |
| storage classes | `volatile` `register` `extern` `auto` |
| each on its own | `static` `mutable` `explicit` `inline` `goto` `sizeof` `enum` `union` `typedef` `wchar_t` `asm` |

**Syntax**, named where it is parsed:

| | |
|---|---|
| declarations | `long long`, default arguments, array parameters, variadic functions, function pointers, bit-fields, more than one declarator in a statement |
| initialisation | brace initialisers, which is also what an array initialiser list is |
| expressions | the comma operator, the conditional operator `?:`, bitwise `&` `\|` `^` |
| classes | multiple inheritance, nested classes, a friend *class*, pure virtual functions |
| statements | labels |
| preprocessor | function-like macros |

**That keyword table belongs to the C layer, and the C++ layer is not bound by
it.** `cxx::Parser` derives from `cc::Parser` and adds classes, references,
`bool`, `new`/`delete`, **operator overloading** and **friend functions** — so
the table's `operator` and `friend` rows say "not supported" about two things
this compiler supports. `operator+` compiles, a friend *function* compiles, and
the corpus has cases for both. Those two messages are reachable only where the
C++ layer has not handled the construct first, which is exactly why
`operator int()` is refused with "operator overloading is not supported": the
reserved-construct skip runs before the `operator` handling. That one is a real
defect and is recorded below.

`bool` is the same rule seen from the other side. It is `cxx::BoolType` in
`AST1.h`, deliberately not a `cc::BuiltinKind` — a C++-layer type with no C-layer
entry — but it was given a token rather than a reserved word, so it never
produced a misleading row here at all. Which is the point: whether a construct
appears in this table says where it was *refused*, not what the compiler
accepts.

**So grep to find the list, then ask a build before believing any one line of
it.** Every row above was checked that way rather than read off the source, and
that is how the `operator` and `friend` rows were caught.

`<<=`, `>>=` and `~` are excluded and have no named diagnostic at all; they are
in the diagnostics section below rather than here, because a construct that is
refused without being named is a defect in the refusing, not an entry in a list.

## Closed

| What | Where it was |
|---|---|
| A generated copy constructor suppressed the implicit default one, so `Derived a;` on a class declaring nothing became an error | `Semantic::selectConstructor`, `cxx::Lowering::emitConstruct` |
| The generated initialiser list named bases and members that had no constructor taking one argument, rejecting code that had always compiled | `Semantic::synthesiseCopyConstructors` |
| A class returned by value was copied with `memcpy`, so its copy constructor never ran — and for a class owning memory the caller was handed a pointer the returning frame then freed | `cc::Lowering::lowerStmt` |
| A class holding an array was copied byte for byte, whole, so a member beside the array never ran its copy constructor -- and a member owning memory was shared between the two objects and freed twice | `Semantic::canSynthesiseCopy`, `cxx::Lowering::emitPrologue` |
| A `.cxb` could corrupt the host heap (static-data length), segfault (`OP_MemCopy` count), abort the process (call and native argument counts), read and write past memory (floating widths), or run silently to a false success (unknown opcodes) | `VM::load`, the dispatch switch |
| `delete` of a pointer inside a block forged a free-list header out of program data | `VM::release` |
| `MIN / -1` and `-MIN` were undefined; on x86-64 the divide faults | `OP_Div`, `OP_Mod`, `OP_Neg` |
| A double `delete` hung the machine past the step limit | `VM::release` |
| Twelve golden files were CRLF while the compiler writes LF, so the suite failed on a correct compiler | `.gitattributes` |

## Open

### Wrong answers on correct programs

**`-run` reports `main returned 0` whatever main returned.** `VM.cpp:660`. The
value is captured, then `__global_fini` is pushed, and when that frame returns
with `frames` empty again the result is overwritten with its own zero.
`int main(){ return 7; }` lowers to `ret 7` and reports 0.

**Unsigned integers narrower than eight bytes are sign-extended.** `CodeGen.cpp:171`
hardcodes the sign-extend bit on every integer load, and `IRInstr` carries no
signedness at all — only `isFloat`. `unsigned char c = 200; int x = c;` gives
**-56**; `unsigned short s = 60000` gives **-5536**. Fixing it means threading a
signedness flag through the IR, which is why it is still here.
`32_types_all_builtins` misses it because its values never set the sign bit.

**A class that declares no constructor never constructs its base or its
class-typed members — but is still destroyed.** `Lower1.cpp:774`. With
`class Inner { int v; Inner(){v=7;} }; class Outer { Inner i; };`, `Outer o;`
leaves `o.i.v` at 0, and `~Inner` runs on an object that was never constructed.

**A shadowed local is destroyed twice and the outer one never.**
`Lower1.cpp:979` resolves a scope-exit destructor by NAME, so when a `return`
unwinds several open blocks the innermost binding wins for all of them.
`K a(1); { K a(2); return 0; }` runs `~K` on the inner object twice.

**`(*p).f()` is devirtualised, `p->f()` is not.** `Lower1.cpp:726` clears the
dynamic type whenever a dot-form base has a class type. That is sound for a
named object, but `*p` has the pointee's STATIC class and an unknown dynamic
one, so the call goes to the base's version.

**A temporary is constructed and never destroyed.** `W mk(){ return W(9); }`
called as `mk();` runs no destructor. A variable initialised from a call no
longer makes a temporary at all, but a discarded one still does, and four of the
six call paths never destroy their by-value argument copies either.

**`a * b;` is parsed as a declaration.** `Parser1.cpp`, the declaration probe:
when the leading identifier is not a known class it skips `*` and `&` and
declares "this is a type", which is exactly what the `classNames` set exists to
prevent. `int a; int b; a * b;` declares `b` as a pointer to class `a`.

### Valid programs rejected

**`const T&` cannot bind to an rvalue.** `Semantic.cpp:1853` and the matching
guard on local bindings test `!isLValue` without exempting a const referent,
though `convertible` reasons about exactly that case. `int g(const int&);
g(3);` is rejected — the most common idiom in the language being targeted.
Fixing it needs a temp slot in lowering as well; `lowerByValueObject` is the
model.

**An out-of-line definition loses its parameter names.** `Semantic.cpp:1373`
copies a name from the definition only when the declaration's is empty, and the
body is analysed against the declaration's. `class A { int f(int a); };
int A::f(int b){ return b; }` errors on `b`.

**Const-qualified pointer overloads are always ambiguous.** The exact-match test
uses `sameType`, which ignores pointee const, so `f(int*)` and `f(const int*)`
never reach the viable pass. `sameDeclaredType` exists and is not used here.

**A `const`-qualified class return type on an out-of-line definition
mis-parses**, and `operator int()` is rejected with "operator overloading is not
supported" because the reserved-construct skip runs before the `operator`
handling.

### Accepted when it should not be

**`A b = p;` where `p` is an `A*`.** `Semantic.cpp:288` — `classOf` silently
strips one pointer level, so `A*` and `A` compare equal in `canConvert`. This
also affects argument passing, returns and `==`.

**`f() const` takes the vtable slot of `virtual f()`.** `Semantic.cpp:702` —
`sameSignature` compares parameters with `sameType` and never compares the
methods' own const-ness. `sameParams`, written for a different caller, gets both
right; the wrong one of the two is wired into override resolution.

**A function name used as a value** is accepted and typed as its return type:
`int f(); int x = f;` compiles.

**A bare-name call inside a method** uses `lookupLocal`, which searches only the
innermost scope, so a local declared in an enclosing block does not suppress the
rewrite to `this->name(...)` and the wrong function is called with no
diagnostic. `Semantic.cpp:2027`.

### Robustness and diagnostics

**Deep nesting needs a 1MB stack, and says so instead of crashing.** Closed as
a crash, open as a requirement. Operand chains, prefix chains, dereference
chains and statement chains are all bounded at 100 now, so no input segfaults
the compiler — and the limit was chosen from the stack, not from taste, because
what the parser accepts three later passes walk again with much larger frames.
Measured on `cout << x` chains, the most expensive link a program is likely to
write: 512KB dies between 60 and 80, 1MB between 120 and 160. 1MB is the
default main-thread stack on Windows and on iOS, so the limit sits at 100 and
everything works there. **512KB does not**, which is what an iOS
`DispatchQueue` worker gets by default: a host wanting to compile off the main
thread must make the thread itself and give it a real stack.

**`<<=`, `>>=` and `~` still have no named diagnostic.** The lexer splits the
first two into two tokens, and `~` is a prefix operator so it can never reach
the check in `parseExpression`, which runs after a complete expression.

**One bad array bound costs five diagnostics** — the offending token is reported
but not consumed. **An unterminated literal's message is built and thrown away**;
an unterminated `/*` gets no diagnostic at all. **Warnings are silenced by the
error cap** (`Diagnostics.cpp:34`): `capped` is set by errors, and once 20 have
printed no warning is ever shown again.

**`main.cpp` argv handling**: `-o` as the last argument becomes the input path,
and every unrecognised token does the same, so `--help` reports "Cannot open
input file: --help". There is no usage text.

**`0779` mis-tokenises** into two numbers; an invalid octal digit is never named.

**`round()` is wrong at the two `floor(x + 0.5)` edge cases** (`VM.cpp:281`):
`round(0.49999999999999994)` gives 1, and `round(4503599627370497.0)` is one too
large.

### Design and structure

**`long long` breaks the C++98 rule.** `Bytecode.h` pins the VM's word with
`long long` (or `__int64` on MSVC), which is the right decision for a fixed
64-bit word but takes the build from zero warnings to twelve `-Wlong-long` under
`-pedantic`. Selecting `long` where `LONG_MAX` is already 64 bits, and falling
back only where it is not, gives the same width with a clean build. Note also
that the widening stops at the VM: `Token::numberValue`, `NumberExpr::value`,
`ArrayType::count` and `IRInstr::imm` are all host `long`, so on LLP64 a literal
is clipped before it ever becomes a word.

**The overload-resolution loop is written five times** — in `findFreeOperator`,
`findIndexOperator`, `findMemberOperator`, `selectConstructor` and
`resolveOverload` — with subtly different exactness tests. Two of the "accepted
when it should not be" entries above are direct consequences. One shared
candidate-ranking routine would remove the bugs rather than fix them.

**`Layout` computes a construction plan that nothing consumes.**
`Layout.cpp` builds `constructionPlan`/`destructionPlan`, including the
vptr-after-base-before-members ordering its header explains at length, and the
only reader is its own `print()`. `Lower1.cpp` re-derives the same ordering by
hand — and the hand-written copy is where the missing-constructor bug lives.

**`analyzeExprImpl` is one 489-line function** with fourteen `dynamic_cast`
dispatch arms, in a 2,200-line file. Natural seams: type rules and conversions,
class and member machinery, declarations and statements, expressions.

**Name mangling lives in `IR.cpp`** and `dynamic_cast`s to `cxx::` types, which
is the one place the C++ layer reaches into the C-level IR. The IR's own data
model is clean; moving `mangle*`/`typeCode` to their own file would let `IR.h`
and `IR.cpp` drop both AST includes.

**There is no conversion ranking**, only two tiers: exact, then merely
possible, with nothing to choose between two possibles. `exactForOverload`
papers over the one collision that bites -- a pointer can become either a
`void*` or a `bool`, and `ostream` offers both -- by calling `void*` exact for
any pointer. The deviation is real and reachable: `f(Base*)` beside
`f(void*)`, called with a `Derived*`, picks `void*` here where C++ picks
`Base*`. A real ranking pass would remove the special case rather than add to
it.

**A global array cannot be read into.** `cin >> buf` asks the block header for
a `new[]` buffer and the frame table for a local, but static data is a flat run
of bytes with no table saying what lives where, so a global is refused. An
image-level table of global offsets and sizes would answer it the same way the
other two are answered.

**Call and constructor arguments are analysed twice**, so every diagnostic
inside them is reported twice.

**`Expr::resolvedType` outlives its owner.** It points into the analyzer's
`ownedTypes`, which is freed in `main` before the AST is deleted. Nothing reads
it in that window today, and nothing enforces the ordering either.
