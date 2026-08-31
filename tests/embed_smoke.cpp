// Can this compiler be LINKED INTO something else?
//
// Built by tests/run_embed.sh, which compiles it against
// dist/compilerpp_amalgamated.cpp with -DCOMPILERPP_NO_MAIN.  That is the
// whole claim under test: an application has a main() of its own, two of them
// do not link, and until the guard existed the single-file distribution could
// not be dropped into one.
//
// It is also the only place a VM is used more than once.  The command-line
// driver builds a fresh one per run and exits afterwards, so nothing else here
// can see state carried from one run into the next -- and an embedder does
// exactly that, which is why it belongs in a test rather than in a comment.
//
// C++98 only, like everything else.

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// The amalgamation is included as a translation unit, not linked: that is how
// an application would take it, and it is what makes -DCOMPILERPP_NO_MAIN the
// thing being tested.
#include "../dist/compilerpp_amalgamated.cpp"

namespace {

int failures = 0;

void check(const std::string &what, const std::string &got, const std::string &want) {
    if (got == want) {
        std::cout << "ok       " << what << std::endl;
    } else {
        std::cout << "FAIL     " << what << std::endl;
        std::cout << "         wanted [" << want << "]" << std::endl;
        std::cout << "         got    [" << got << "]" << std::endl;
        ++failures;
    }
}

// Everything an embedder needs, in the order main.cpp does it -- minus the
// argument handling and the exits, which are the parts that belong to a
// command line and not to a library.
bool compileToImage(const std::string &sourceText, Image &image, std::string &diagnostics) {
    std::ostringstream errors;
    std::streambuf *savedErr = std::cerr.rdbuf(errors.rdbuf());

    std::string source = sourceText;
    Diagnostics diag("embedded");
    int preludeLines = 0;
    const std::string prelude = preludeFor(source, preludeLines);
    if (!prelude.empty()) {
        source = prelude + source;
        diag.setLineOffset(preludeLines);
    }
    source = expandDefines(source, diag);

    bool built = false;
    {
        cxx::Parser parser(source, diag);
        std::vector<cc::Decl*> unit = parser.parseTranslationUnit();
        {
            SemanticAnalyzer sem(diag);
            sem.analyze(unit);
            Layout layout(diag);
            layout.computeAll(sem.classMap());
            if (!diag.hadError()) {
                IRModule module;
                {
                    cxx::Lowering lower(module, layout, diag, sem.classMap());
                    lower.lowerClasses();
                    lower.lowerUnit(unit);
                }
                if (!diag.hadError()) {
                    CodeGen gen(diag);
                    gen.generate(module, image);
                    built = !diag.hadError();
                }
            }
            // The AST dies INSIDE the analyser's scope.  Expr::resolvedType
            // points into the analyser's owned types, so destroying the tree
            // after it would leave every node's type dangling -- harmless in a
            // process that is about to exit, and not something to inherit.
            for (std::size_t i = 0; i < unit.size(); ++i) delete unit[i];
            unit.clear();
        }
    }

    std::cerr.rdbuf(savedErr);
    diagnostics = errors.str();
    return built;
}

// Runs an image on a VM the caller owns, capturing what the program prints.
std::string runCapturing(VM &vm, const Image &image, bool &ok) {
    std::ostringstream out;
    std::streambuf *savedOut = std::cout.rdbuf(out.rdbuf());
    vm.run(image, ok);
    std::cout.rdbuf(savedOut);
    return out.str();
}

const char *HELLO =
    "#include <iostream>\n"
    "int main(){ cout << \"one\" << endl; return 0; }\n";

const char *OTHER =
    "#include <iostream>\n"
    "int main(){ cout << \"two\" << endl; return 0; }\n";

// Traps partway down a call chain, so the frames it leaves behind are many
// and their func indices are high -- which is what makes a stale one point
// past the end of a smaller image's function table.
// Enough of a program to need a heap, a copy constructor, a destructor and
// some recursion -- the shapes that actually use the machine's memory.
const char *SIZED =
    "#include <iostream>\n"
    "class M { public: int n; double *a;\n"
    "  M(int k){ n=k; a=new double[k]; int i=0; while(i<k){a[i]=i;i=i+1;} }\n"
    "  M(const M &o){ n=o.n; a=new double[n]; int i=0; while(i<n){a[i]=o.a[i];i=i+1;} }\n"
    "  ~M(){ delete[] a; } };\n"
    "int fib(int n){ if (n < 2) return n; return fib(n-1) + fib(n-2); }\n"
    "int main(){ M x(400); { M y = x; cout << y.a[399] << endl; } cout << fib(18) << endl; return 0; }\n";

const char *LOOPS =
    "#include <iostream>\n"
    "int main(){ int i=0; int s=0; while (i < 1000) { s = s + i; i = i + 1; } cout << s << endl; return 0; }\n";

const char *TRAPS =
    "#include <iostream>\n"
    "int deep(int n){ if (n > 0) return deep(n - 1); int *p = 0; return *p; }\n"
    "int main(){ cout << deep(20) << endl; return 0; }\n";

} // namespace

int main() {
    // 1. It links at all, which is the guard doing its job.
    std::cout << "ok       links with -DCOMPILERPP_NO_MAIN" << std::endl;

    Image first;
    std::string diagnostics;
    if (!compileToImage(HELLO, first, diagnostics)) {
        std::cout << "FAIL     compiling a program returned errors: "
                  << diagnostics << std::endl;
        return 1;
    }
    std::cout << "ok       compiles in memory, no file touched" << std::endl;

    Image second;
    if (!compileToImage(OTHER, second, diagnostics)) {
        std::cout << "FAIL     compiling a second program returned errors: "
                  << diagnostics << std::endl;
        return 1;
    }

    // 2. Output is captured rather than printed, which is what a view needs.
    {
        VM vm;
        bool ok = false;
        check("output is captured, not printed", runCapturing(vm, first, ok), "one\n");
    }

    // 3. ONE VM, two different images.  run() has to start from nothing each
    //    time or the second run indexes the first one's frames.
    {
        VM vm;
        bool ok = false;
        check("one VM, first image",  runCapturing(vm, first,  ok), "one\n");
        check("one VM, second image", runCapturing(vm, second, ok), "two\n");
    }

    // 4. The same, with a TRAP in between.  A run that fails leaves its frames
    //    behind, and those are what the next run walks into.
    {
        Image trapping;
        if (!compileToImage(TRAPS, trapping, diagnostics)) {
            std::cout << "FAIL     compiling the trapping program returned errors: "
                      << diagnostics << std::endl;
            return 1;
        }
        VM vm;
        bool ok = true;
        runCapturing(vm, trapping, ok);
        if (ok) {
            std::cout << "FAIL     the trapping program was not trapped" << std::endl;
            ++failures;
        } else {
            std::cout << "ok       a trap is reported, not fatal" << std::endl;
        }
        bool ok2 = false;
        check("a good run after a trap", runCapturing(vm, first, ok2), "one\n");
        if (!ok2) {
            std::cout << "FAIL     the run after a trap reported failure" << std::endl;
            ++failures;
        } else {
            std::cout << "ok       and it reports success" << std::endl;
        }
    }

    // 5. The machine's size and patience are the HOST's to choose.  The
    //    defaults are a 4MB machine, which is most of what a run costs in
    //    memory and far more than a student program needs; an application on a
    //    phone would rather spend 256KB.
    {
        Image sized;
        if (!compileToImage(SIZED, sized, diagnostics)) {
            std::cout << "FAIL     compiling the sized program: " << diagnostics << std::endl;
            return 1;
        }
        VM big;
        bool ok = false;
        const std::string wanted = runCapturing(big, sized, ok);

        MachineLimits small;
        small.memory    = 256 * 1024;
        small.callStack =  64 * 1024;
        VM vm;
        vm.setLimits(small);
        bool ok2 = false;
        check("a 256KB machine gives the same answer", runCapturing(vm, sized, ok2), wanted);
        if (!ok2) { std::cout << "FAIL     the small machine did not succeed" << std::endl; ++failures; }
    }

    // 6. And a host can choose badly.  A call stack that does not fit leaves
    //    the heap past the end of memory, so it is refused by name rather than
    //    trusted -- the same treatment a bad image gets.
    {
        Image image;
        std::string ignored;
        compileToImage(HELLO, image, ignored);
        MachineLimits impossible;
        impossible.memory    = 32 * 1024;
        impossible.callStack = 64 * 1024;       // larger than the memory holding it
        VM vm;
        vm.setLimits(impossible);
        bool ok = true;
        runCapturing(vm, image, ok);
        if (ok) {
            std::cout << "FAIL     an impossible machine was not refused" << std::endl;
            ++failures;
        } else {
            std::cout << "ok       an impossible machine is refused, not fatal" << std::endl;
        }
    }

    // 7. The step budget is what stops a runaway program, and the default is
    //    tighter than it looks -- a million-iteration loop spends 49 million
    //    steps of the 50 million allowed.  A host that wants real loops to
    //    finish raises it; one that wants a quick answer lowers it.
    {
        Image image;
        std::string ignored;
        compileToImage(LOOPS, image, ignored);
        MachineLimits patient;
        patient.maxSteps = 100L * 1000 * 1000;
        VM vm;
        vm.setLimits(patient);
        bool ok = false;
        check("a raised step budget lets a loop finish", runCapturing(vm, image, ok), "499500\n");

        MachineLimits impatient;
        impatient.maxSteps = 1000;
        VM vm2;
        vm2.setLimits(impatient);
        bool ok2 = true;
        runCapturing(vm2, image, ok2);
        if (ok2) {
            std::cout << "FAIL     a 1000-step budget did not stop the loop" << std::endl;
            ++failures;
        } else {
            std::cout << "ok       a lowered step budget stops one" << std::endl;
        }
    }

    // 8. Repeatedly, in one process, which is the shape of an application.
    {
        bool same = true;
        for (int i = 0; i < 50; ++i) {
            Image image;
            std::string ignored;
            if (!compileToImage(HELLO, image, ignored)) { same = false; break; }
            VM vm;
            bool ok = false;
            if (runCapturing(vm, image, ok) != "one\n") { same = false; break; }
        }
        check("50 compile-and-run cycles agree", same ? "yes" : "no", "yes");
    }

    std::cout << std::endl;
    if (failures == 0) {
        std::cout << "embeddable: links, captures, and survives reuse" << std::endl;
    } else {
        std::cout << "embedding FAILED " << failures << " check(s)" << std::endl;
    }
    return failures == 0 ? 0 : 1;
}
