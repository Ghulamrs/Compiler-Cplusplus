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

Each must now stop with a diagnostic and exit 3. The expected text is in the
`.txt` beside it.

## Rebuilding one

There is no tool: they were written byte by byte against the format in
`Image::write`, which is magic, version, entry, fini, static data, then each
function. Note that `localOffset` and `localSize` are `vector<int>` while the
file stores 64-bit fields, so a crafted value has to survive that narrowing —
`INT_MIN` does, a 64-bit minimum does not.
