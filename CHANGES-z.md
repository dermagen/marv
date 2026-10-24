# marv-z: zero-initialised data + macro intrinsics fork

This is a separate copy of the MARV tree from `/home/user/github`,
built as `/home/user/future/marv-z/marv`.  The original tree is
untouched.  Two changes were made, in all three compiler sources
(`src/sf/r.sf`, `src/skint/marv.scm`, `src/skint/marv-simple.scm`),
and the standalone binary was regenerated with `sfc` and rebuilt.

## 1. All-zero data segments use C's static zero initialisation

The C emitter used to print every byte of an all-zero segment:

```c
static MARV_ALIGNED(8) uint8_t pi_roots[1048576] = {0,0,0,...};
```

That one line was 2,097,206 characters for the DS benches.  If every
byte of a data segment is zero, the emitter now prints a bare
declaration:

```c
static MARV_ALIGNED(8) uint8_t pi_roots[1048576];
```

C guarantees zero initialisation of static storage before startup
code runs, and the all-zero bit pattern is this runtime's `#f`, so
the roots array has exactly the intended initial contents.  The
array lands in BSS; no initialised bytes are stored in the
executable.  Segments containing pointer relocations still get the
existing startup constructor, which writes the relocated pointers
into the zero-initialised storage.

The other targets already did the right thing and were verified
unchanged (byte-identical output on the queens example):

- rv64g: `.zero N` for all-zero segments without relocations.
- wcpl: `size=N` instead of an escaped byte string.

## 2. C99 intrinsics are macros on GNU/Clang

The generated C prologue (`*marv-c99-header*`) defined 167
`static inline marv_*` helper functions.  On GCC/Clang these are now
167 function-like macros.  Expression macros that need temporaries
(loads/stores through `memcpy`, rotates, clz/ctz, reinterpret casts)
use GNU statement expressions with named temporaries, so every
argument is still evaluated exactly once and the `memcpy`-based,
alignment-safe definitions are preserved.  Operators, comparisons,
conversions and libm wrappers are plain parenthesised expression
macros.  For compilers without statement expressions, the old
inline-function block remains as an `#else` fallback.

## Measured effect (gcc -O2 -c, generated TU only, median of 7)

| bench  | old (inline + literal) | zero-decl + inline | zero-decl + macros |
|-------:|-----------------------:|-------------------:|-------------------:|
| tak    | 1.023 s / 2,179,713 B  | 0.107 s / 82,557 B | 0.098 s / 94,403 B |
| queens | 1.627 s / 2,762,412 B  | 0.744 s / 665,256 B| 0.730 s / 676,670 B|
| pi     | 1.722 s / 2,855,862 B  | 0.830 s / 758,706 B| 0.703 s / 770,344 B|

The zero-initialisation fix is the big win (source is ~3-26x
smaller, gcc up to ~10x faster on tak).  The macro switch adds a
further ~2-15%; its value is mostly that inlining no longer depends
on the compiler choosing to inline 167 prologue functions at every
one of thousands of call sites.

## Validation

- Full DS bench/test battery built with `marv-z` (ccds target):
  t1-t10, t6b, tak, ltak, queens, rk, pi -- 16/16 PASS, outputs
  byte-identical to Skint.
- MARV examples (dirent_demo, libc_demo, queens) build and run
  under `-t cc`; queens `.wo` and `.s` outputs are byte-identical
  to the original MARV.
- Generated `pi.c`: longest line drops from 2,097,206 to 206
  characters; `pi_roots` is a single bare declaration.

## Rebuilding

```sh
cd src/sf && sfc m.sf r.sf v.sf a.sf && cp m.c r.c v.c a.c ../..
cd ../.. && make marv
```

(Use the `sfc` from a #F installation; in this workspace a copy had
to be made executable first because the installed one lost its +x
bit in a snapshot.)

## 3. Link-mode C splitting (`--split-funcs N`, default 300)

The single-translation-unit C output of a large module is what makes
the host C compiler's memory explode (measured on the CPS-compiled
`marv.scm`: GCC `-O2` needs 1,335 MiB of cc1 RSS for the one
2,630-function file; Clang 1,043 MiB; even `-O0` still needs 1,106 /
761 MiB -- it is translation-unit size, not the optimiser).

MARV already compiles to temporary files in link mode, so the split
lives in the driver (`marv-split-c-temp` in `a.sf` /
`src/skint/marv*.scm`) and costs nothing in `-c` mode:

* New option `--split-funcs N` = maximum number of function
  definitions per generated C piece, default **300**, 0 disables.
  It applies only to link mode with the cc target; `-c` always emits
  exactly one file (verified byte-identical with the option given).
* After a module's temporary C file is generated, if it has more
  than N function definitions it is split into pieces of at most N.
  Piece 0 reuses the original temp file; the linker command receives
  all pieces and they are deleted afterwards like any temp object.
* Per piece: module function prototypes lose `static` (one external
  definition each, in exactly one piece); data/global definitions
  stay in piece 0 and become plain `extern` declarations (no
  initialiser) in the other pieces; `main` and the data-relocation
  constructor stay in piece 0.  Function-pointer identity is
  preserved.

Measured, full `marv-cps.mvs` link build with the default 300
(GCC 14.2, `-O2`): the module is split into 9 pieces, per-piece cc1
peaks 182--218 MiB (max **217 MiB, a 6.1x cut** vs 1,335 MiB), total
link time unchanged in sum and parallelisable.  A symbol cross-check
of the pieces shows 2,635 program symbols defined exactly once and
no unresolved cross-piece references.  Forced splits
(`--split-funcs 5/10`) of the queens example and a DS-compiled queens
produce byte-identical program output; modules under the threshold
are untouched, and `-c` output is byte-identical to the pre-split
build.

Honest caveat: MARV itself peaks at ~1.09 GiB RSS while *emitting*
the 9.3 MB `marv-cps.mvs` (identical in `-c` mode with no splitting),
so at this module size the bottleneck moves from cc1 to MARV's own
front end.  That is pre-existing emitter cost, not splitter overhead
(the split itself takes ~2 s and a few MiB), and is the next thing to
attack if self-hosting at this scale matters.

Also recorded in `/home/user/future/selfgcc-measurement.md`
(GCC/Clang x -O0/-O1/-O2 and the blob-extern experiments behind this).

## Test-suite status (2026-10-23)

`test/run-tests.sh` now runs to completion in this workspace:
**rc=0, all 6 sections pass** (`test/test.scm`: 40 passed, 0 failed;
standalone-binary/Skint parity byte-identical for cc/wcpl/rv64g).
Three environment/fixture gaps had to be closed first -- none of
them was a code bug:

1. `lib/scmi.c`'s `#include "scmrt.h"` is a wcpl *module interface*
   lookup (CWD, then `-I` bases, then `<libdir>include/`; paths must
   end with a separator).  Fixed by staging the MARV headers plus
   prebuilt `scmrt.wo`/`scmi.wo` in `/tmp/wcpllib/` and running with
   `WCPL_LIBRARY_PATH=/tmp/wcpllib/` (note the trailing slash).
2. The repo lacked the `target/` directory the Makefile and suite
   expect; populated `target/*.mvt` from `/home/user/mstd`, with
   `cc.mvt` invoking `cc` (not `gcc`) as the suite asserts.
3. `/home/user/bin/*` tools keep losing their execute bit across
   workspace snapshots; `chmod +x` them before running.

## 4. Big-endian audit, `main()` conformance, transient IR (2026-10-23)

Three independent items, all validated below.

### 4a. Big-endian: audit conclusions

- The `.mvs` data format is **little-endian by definition**
  (`doc/README.md`: `(i16/i32/i64/f32/f64 ...)` items are specified
  as byte sequences).  Emitting data as `uint8_t` byte arrays is
  therefore already the host-portable representation; a big-endian
  host sees exactly the authored bytes.
- Generated code and `lib/scmrt.c` share one heap and must agree on
  layout, so both stay **native-endian**: the 25 load/store macros
  are memcpy-based native accesses, and pointer loads are
  constructor-written native relocations that must never be
  byte-swapped.  The C target is a native-endian runtime; authored
  `(iN ...)` items consumed through word loads follow host
  endianness (the wcpl/rv64g targets are little-endian machines,
  where this matches the spec exactly).
- **Rejected:** re-emitting data as native `uint64_t` words.  It
  would fix word loads of authored data on BE, but scramble
  byte-consumed data (the RDS literal blob is parsed byte-wise by
  `deserialize_sexp`), contradict the spec, and shrink initializers
  by only ~40% (blob 206k -> ~137k chars) -- no huge-data-string
  win, and zero-filled segments are already BSS.

### 4b. The one real BE bug: littab decoding (fixed)

`deserialize_all` in `lib/scmrt.c` read the little-endian-authored
littab through a native `int32_t *`, scrambling every
(cell_offset, blob_offset) pair on a big-endian host.  It now
assembles each i32 from its 4 bytes explicitly.  (`/home/user/github`
untouched; the fix exists only in this fork's `lib/scmrt.c`.)
No native BE host was available to test on; the new code is pure
byte assembly, correct by construction, and the LE-host regression
check is the DS queens_runc end-to-end run below.

### 4c. `main()` now conforms to hosted C99 (Clang accepts it)

Clang hard-errors on the old declaration ("second parameter of
'main' must be of type 'char **'"); `-Wno-main` does not help.
New helper `c99-param-type` in the cc emitter (`src/sf/r.sf`,
mirrored in `src/skint/marv.scm` / `marv-simple.scm`): when a
function is exported as `main`, a `ptr` parameter is declared
`char **`; everything else is unchanged (`marv_i32` is `int32_t`,
so the rest of the signature already conformed).

Validation on the 2,630-function `marv-cps.mvs`: the new `-c`
output differs from the pre-patch output in **exactly 2 lines**
(the `main` prototype and definition); Clang 19.1.7 `-O2 -c`
compiles it cleanly (1,056 MiB peak, no rename hack), while the
pre-patch output still hard-errors as control.  GCC output/object
unchanged in size.

### 4d. Transient per-function IR: MARV's own peak halved

`emit-c99`/`emit-rv64g` previously unstacked **all** functions into
`ir-funcs` before emitting any, so every function's IR was live for
the whole emission.  Both emitters now unstack one function, emit
it, and drop the IR.  Measured on `marv-cps.mvs -c` (same setup as
Sec. 3): peak RSS **1,087.5 -> 524.1 MiB**, wall 44.7 s -> 43.9 s,
output byte-identical except the two `main` lines above.  In split
link mode the whole marv+gcc tree peaks at 784 MiB (was ~1,090 MiB
for MARV alone), still rc=0 with the binary produced.

Remaining memory, for the record: whole-module front end
(parse-module, imports, signature tables) plus the retained
unstacked-func list itself.  A fully streaming front end would
require dropping the `(module ...)` wrapper from the input format
(top-level forms processed as they arrive); that changes the
converter output format and the import/signature model, and is a
design item, not this patch.

### Validation summary (this batch)

- `test/run-tests.sh`: rc=0, 47 [PASS] / 0 [FAIL], test.scm
  40 passed / 0 failed (covers the rv64g transient change via
  Skint-parity diffs on all three targets).
- DS queens_runc built with `ccds` (unsplit and `--split-funcs 10`)
  runs byte-identical to Skint -- exercises the patched
  `deserialize_all` literal startup on every run.
- Full `marv-cps.mvs` link (default split): rc=0, binary produced.

## 5. Native-endian typed data for C targets, `(ptr ...)` items (2026-10-23)

Supersedes the byte-order stance of §4a for *structured data*: declaring
everything little-endian suited only targets that are little-endian
(rv64g, wcpl). For `.c` targets, typed data (`i16`/`i32`/`i64`/`f32`/
`f64`/`ptr` runs) is now emitted so the C compiler lays it out in the
host's native byte order, matching the C code the generated file links
with. Byte streams stay byte streams (below).

### 5a. Emission shape (c99 emitter, all three sources)

Each non-zero segment becomes a union whose first member is a byte view
and whose second is a typed array of the first run, initialised with a
designated initialiser — the C compiler itself produces native-order
bytes:

    static MARV_ALIGNED(8) union { uint8_t _b[MARV_DSZ(MARV_DS_TAIL)];
      int32_t _t[MARV_DSZ(2)]; } ptrtest_tab = {._t={7,8}};

A C union initialiser can only initialise one member, and the typed
struct/anchor alternatives lose bytes to alignment padding, so runs
after the first are emitted as standalone anchor objects
`<seg>_aK` of union type `<seg>_aK_t`, plus an alias pointer
`<seg>_K` cast onto `&<seg> + off`. A constructor `marv_dinit_<seg>()`
memcpy's each anchor into place; `_marv_init_data_relocs` (when pointer
relocations exist) calls every `marv_dinit_*` before storing any pointer
slot, because constructor order between translation units is otherwise
unspecified. All-zero segments still emit as bare zero-initialised byte
arrays (§1). Segment addresses are taken through `(marv_ptr)(void
*)&sym` — the union type is not assignment-compatible with `uint8_t*`
in standard C.

### 5b. `(ptr ...)` data items implemented

`(ptr ...)` was documented but rejected by `expand-data-items`. It now
expands like the other typed runs, with 8-byte slots whose values are
`0`/`(ptr.null)`, a data symbol, `(ref.data $sym [off])`, or
`(ref.func $sym)` (the latter two become startup relocations through
the existing mechanism). `expand-data-runs` counts ptr runs so offsets
stay consistent.

### 5c. What did *not* change: littab

`lib/scmrt.c deserialize_all` still assembles multi-byte values
explicitly from little-endian bytes. The literal table is authored and
serialised as a byte stream (`$littab`), so its encoding is a file
format, not a memory layout; making it native would change the format,
not the semantics. Typed data and littab are separate mechanisms and
are now documented as such (`doc/README.md` §4.1).

### 5d. Validation

- `test/run-tests.sh`: 47 PASS / 0 FAIL (unchanged).
- `/tmp/selfgcc3/nativetest.mvs` (packed i32/i16/i8/i64/f64/f32/ptr
  segment at cumulative offsets): identical typed output under x86-64
  gcc and `powerpc64-linux-gnu-gcc -static` under `qemu-ppc64`
  (`305419896 42 1311768465173148928 1.500000`); only a deliberate
  byte-reinterpretation differs, each way correct for its host.
- `ptrtest2.mvs` (segment `(ptr $tgt 0)`, `ptr.load` + `i32.load`
  through it): prints `exit=111` on both hosts.
- Full `marv-cps.mvs` regenerated with `-t c99 -c`: the 14,660-line
  diff against the pre-change output is exclusively (a) the prologue
  comment/`MARV_DSZ`/`MARV_DS_TAIL` block, (b) segment declaration
  lines (union form, `&sym` casts), (c) `(marv_ptr)(void *)&sym`
  casts at address-taken sites. No instruction bodies changed.
- Split mode against the multi-line emission: DS queens with
  `ccds --split-funcs 10` runs byte-identical to Skint; full
  `marv-cps.mvs` link with `--split-funcs 300` rc=0.

## 6. Why DS (and CPS) chained code overflows the stack on ppc64 ELFv1

Found while cross-checking the §5 work on big-endian PowerPC
(`powerpc64-linux-gnu-gcc -static` + `qemu-ppc64`). It is not an
endianness bug and not a data-layout bug (both ruled out below); it is
a calling-convention limitation of the ELFv1 ABI on current compilers.

### 6a. Symptom and bisect

DS-compiled `queens_runc` linked by hand: LE output byte-identical to
Skint; the BE binary segfaults (rc=139) inside `prog_main` after
`scmrt_init` succeeds. Across the DS battery the split is by *depth*,
not by content: t1–t5, rk, and a small tak(10,6,3) pass BE with
BE==LE output; tak(18,12,6)x10000, ltak, pi, queens overflow. gdb over
the qemu stub shows a ~13,097-frame backtrace in `tak_ds_u10` faulting
at the prologue `stdu r1,-928(r1)` with r1 at the stack guard
(si_addr ~41 MB below the stack mmap under -strace). pi also failed
in one LE manual-link build; that failure is unrelated (link set)
and pi's BE crash reproduces the same stack signature.

### 6b. Mechanism

DS and CPS units transfer control exclusively with
`MARV_MUSTTAIL return f(...)` / `return ((fnptr)p)(...)`, which are
only bounded-stack if the C compiler emits a sibling call.

- GCC gained `musttail` in GCC 15. The Debian 13 cross compiler is
  GCC 14.2: `__has_attribute(musttail)` is false, `MARV_MUSTTAIL`
  expands to nothing, and everything rides on the ordinary
  sibling-call heuristic.
- On ppc64 ELFv1 a function pointer is a descriptor {code, TOC, env};
  every indirect call sequence is `ld r11,0(r10); mtctr r11;
  ld r11,16(r10); ld r2,8(r10); bctrl` and a non-tail call must then
  restore the caller's TOC (`ld r2,40(r1)`). Empirically (probes in
  /tmp/selfgcc3/shin*.c, beetest27–56), GCC 14 sibling-calls *only*
  direct calls within the same TOC — same-TU statics. Indirect calls
  (target loaded from a closure/roots slot, global fnptr, or
  memcpy'd into a typed fnptr variable) and cross-TU direct calls all
  compile to ordinary calls with a full epilogue after the `bctrl`,
  leaking one native frame (~0.4–1.4 KB here) per unit transfer.
  A 4M-transfer mutual recursion that completes on x86-64 segfaults
  under qemu-ppc64; the same probe on ELFv2-style targets is fine
  because there is no per-function TOC.
- Clang 19 states the limitation directly: `'musttail' attribute for
  this call is impossible because indirect calls can not be tail
  called on PPC`. So on ELFv1 there is currently no compiler flag
  spelling that makes generated chained code safe: GCC <15 silently
  degrades, Clang refuses.
- x86-64 has no TOC register; GCC sibling-calls the indirect shape
  unconditionally, which is why the entire LE battery passed.

### 6c. Scope

Anything whose control flow is a chain of MARV units is affected on
ELFv1 + GCC <15: the DS backend everywhere, and the CPS backend
wherever the chain goes through a closure (return_call_indirect) or
across split pieces (cross-TU return_call). Same-TU direct chains do
get sibling calls — which is precisely why shallow tests pass and
only deep ones crash. The §5 native-data work is unaffected:
nativetest and ptrtest2 pass BE, and shallow DS programs are
BE==LE. (Whether GCC 15's musttail handles indirect calls on ELFv1
is unverified — no GCC 15 cross toolchain is packaged for Debian 13.)

### 6d. Options (decision pending — superseded by §7)

1. Document ELFv1 + GCC <15 as unsupported for chained code
   (README/CHANGES note only). BE validation then covers data,
   single-unit, and shallow programs.
2. Add a non-chained fallback (trampoline/driver loop) selectable
   per target for ELFv1-class ABIs. Real fix, but contradicts the
   DS backend's "no trampoline until Halt" design premise and would
   need its own design pass.
3. Require GCC >=15 or Clang for ELFv1 builds (Clang currently
   refuses indirect musttail on PPC, so in practice GCC >=15,
   unverified).

## 7. Bounded-stack trampoline for chained code; segment sizing fix

### 7a. Segment sizing (§5.5, validated)

Data-segment objects are now declared with their full byte run,
`_b[MARV_DSZ(total)]`; the 255-byte `MARV_DS_TAIL` view remains only
on the static-data *anchors*, which are memcpy sources and are never
sized by the view. The old emission (segment object sized by the
tail view) is a genuine out-of-bounds write whenever a segment
exceeds 255 bytes: an ASan replica of the old shape reports
`global-buffer-overflow: WRITE of size 320`. After the fix,
`bigseg4` (a >255-byte segment program) runs rc=42 on LE and BE and
is ASan-clean.

### 7b. The trampoline (Skint-style periodic unwind)

Decision (user-directed, superseding §6d): follow Skint — do not
trampoline every transfer. Chained units keep calling each other
directly so the C stack grows for a while, then one transfer unwinds
all the way back to a driver loop, which re-enters the chain. The
stack depth is therefore bounded by the fuel supply, on every ABI,
with no reliance on sibling calls.

Mechanism, in the generated C (all three sources: `src/sf/r.sf`,
`src/skint/marv.scm`, `src/skint/marv-simple.scm`):

- `MARV_TRAMPOLINE` defaults to 1; `MARV_TC_FUEL` defaults to 512.
- At every chained transfer (direct `return_call` and indirect
  `return_call_indirect`, fingerprinted for both chain signatures —
  DS `(i64 i64 i64 i64 ptr ptr i32) -> ptr` and CPS
  `(ptr i64 ptr ptr i32) -> ptr`), the generated code first does
  `--marv_tc_fuel`; on exhaustion it stashes the callee and
  arguments into `marv_tc_state[16]` (slot 0 = callee function
  pointer as `(marv_i64)(uintptr_t)`, 1 = pending flag, 2 = nargs,
  3.. = arguments, pointers via `uintptr_t`), sets pending, and
  returns `((marv_ptr)0)` — unwinding the whole chain.
- The exported `main` maps to `MARV_MAIN_FN`; a small driver `main`
  calls it, then loops while pending: refuel, clear pending, and
  dispatch on slot 2 (7 → DS signature with slots 3..9, otherwise
  CPS with slots 3..7). Halt/result semantics are unchanged (a
  non-pending return is the program result).
- Where the compiler guarantees `musttail` (`__has_attribute
  (musttail)`, not ppc64 ELFv1), `MARV_TAILCALL_GUARANTEE` is kept
  exactly as before. `-DMARV_NO_TRAMPOLINE` restores pure-musttail
  emission, including on ELFv1 — where Clang then refuses indirect
  musttail at compile time (loud, not silent). In trampoline mode
  `MARV_MUSTTAIL` is empty on ELFv1, so nothing depends on the
  sibling-call heuristic there.

One definition per program. `marv_tc_fuel`/`marv_tc_state` are
declared `extern` in the header; definitions are emitted in every
module, and the link-mode driver keeps exactly one: the first
`.mvs` module of the link keeps them, later modules get externs;
split pieces k>0 also get externs (the array definition line is
handled explicitly, since it has no ` = ` for the generic global
rule); `--split-funcs 0` still rewrites non-first modules. If a raw
C input to the link already defines the state (e.g. a MARV `.c`
from an earlier `-c` compile), the driver pre-scans for it and
*every* `.mvs` module gets externs. Caveat: `-c` output always
carries the definitions (the driver rewrite is link-mode only), so
hand-linking several `-c` objects together must account for that —
combine them through a link-mode build or drop the duplicates.

### 7c. Validation

- Test suite: **47 PASS / 0 FAIL** (`WCPL_LIBRARY_PATH=/tmp/wcpllib/
  bash test/run-tests.sh`), including the 3-module mixed
  `.c` + `.mvs` link that previously failed both ways (undefined,
  then duplicate, `marv_tc_*` references). Mixed, all-`.mvs`,
  `--split-funcs 1`, and `--split-funcs 0` multimod links all build
  and run.
- DS battery (tak, queens, ltak, pi, rk, t1–t5 from
  `/home/user/future/dstests/*.mvs`): 10/10 LE + BE (qemu-ppc64),
  BE output byte-identical to LE; queens matches the pre-trampoline
  golden output. Fuel sites present in every generated file.
- CPS ltak (`/home/user/smarv-bench/ltak-cps.mvs`, 7 fuel sites):
  LE and BE both print `1400`; `--split-funcs 1` link also prints
  `1400`. ptrtest with `--split-funcs 1` still exits 111 (by
  design); DS queens `--split-funcs 10` matches the golden.
- Full `marv-cps.mvs` split link builds rc=0. The resulting binary
  segfaults at startup (rc=139) — the known S4 issue (compiled CPS
  programs miss host `command-line` globals), identical in the
  pre-fix binary; not a split or trampoline defect.
- tak configuration matrix (all print `70000`): gcc x86-64
  trampoline, gcc ppc64/qemu trampoline, clang guaranteed-musttail,
  gcc `-DMARV_NO_TRAMPOLINE`, clang ppc64 trampoline.

### 7d. Cost and limits

Overhead on transfer-saturated tak (x86-64, gcc -O2, best of 3):
trampoline 3.59 s vs `-DMARV_NO_TRAMPOLINE` 2.37 s (~1.5x). The
cost is the per-transfer fuel decrement of a global; raising
`MARV_TC_FUEL` amortises the unwinds but not the check. On ELFv1
the alternative is not "faster" but "crashes", so the default
stands; hosts with a real musttail guarantee can opt out.

Limits of this version: the unwind only resumes chains entered
from the driver `main` (or exported main) chain. Arbitrary C
callbacks that re-enter chained code, and multiple independent
chains driven from hand-written C, are outside v1. The `ccds`
target still needs `-fno-partial-inlining -fno-ipa-cp -fno-ipa-sra`
(GCC IPA clones break tail-transfer shapes independently of this
mechanism).
