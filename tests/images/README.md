# Malformed images

`Bytecode.h` says a `.cxb` may have come from anywhere and that nothing in it
is trusted. These files are what that claim means: each is a hand-built image
that is well-formed enough to load, and wrong in one specific way that used to
reach memory outside the VM's own array.

They are committed as bytes rather than generated, because the point is the
exact file — a generator would have to be trusted not to drift.

| File | What is wrong | What it used to do |
|---|---|---|
| `bad_frame_layout.cxb` | `localOffset` describes 1 slot, `localSize`/`localFloat`/`localObject` claim 5 | the argument loop was bounded by `localSize` but indexed `localOffset`, reading past the end of a 2-element vector and using what it found as a frame offset |
| `bad_object_offset.cxb` | a by-value object parameter whose slot offset is `INT_MIN` | `dst` was checked only against an upper bound, so a negative offset passed and `memmove` wrote in front of memory |
| `memcopy_count_overflows.cxb` | an `OP_MemCopy` whose count is the largest positive word | the bound was written `src + n > mem.size()`, and that sum wraps negative, so the check passed and `memmove` ran with the count — SIGSEGV |
| `call_argument_count.cxb` | a `call` claiming 3,640,655,872 arguments | the count sized a `std::vector`, which threw `length_error`; nothing catches it, so the process aborted. One flipped byte in a valid image is enough to reach this |
| `native_argument_count.cxb` | a `native` claiming 2,147,483,632 arguments | the pop loop ran the whole count, setting the trap on the first empty stack and then continuing for two thousand million more iterations — 47 seconds of CPU, outside the interpreter's step limit |
| `float_width.cxb` | a floating store of 5 bytes | the bounds check used the stated width but the store always moved 8, writing past the end of memory for every width below 8 |
| `unknown_opcode.cxb` | opcode 200, which is not one of the 61 that exist | the dispatch switch had no `default:`, so the instruction was a silent no-op: the image RAN, did nothing, and reported success |

Each must now stop with a diagnostic and exit 3. The expected text is in the
`.txt` beside it.

One case has no file here: a static-data section larger than the machine's
four megabytes, which used to be copied into a fixed-size buffer and corrupt
the host's heap. Triggering it needs a genuinely oversized section, and a
five-megabyte fixture is not worth carrying in the repository — the guard is
at the top of `VM::load`, beside the other load-time checks.

## Rebuilding one

There is no tool: they were written byte by byte against the format in
`Image::write`, which is magic, version, entry, fini, static data, then each
function. Note that `localOffset` and `localSize` are `vector<int>` while the
file stores 64-bit fields, so a crafted value has to survive that narrowing —
`INT_MIN` does, a 64-bit minimum does not.
