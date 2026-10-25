# Scheme Runtime Support (`srt/`)

The `srt/` directory holds the runtime support for Scheme code compiled to
MARV. It is not part of MARV's standard library. `lib/` is a cover for the
host C library, and every module there corresponds to a C header a C
programmer would recognise; the modules here instead define one particular
language's object representation, heap and instruction set, and they are
maintained with the S-MARV compiler rather than with MARV itself.

The split is a directory split only. These are ordinary MARV system modules
in every other respect: a `.mvh` interface, a single-source `.c` adapter
beside it where there is one, and the same `rp$` and `ap$` conventions.

## Contents

| File | What it is |
| --- | --- |
| `scmrt.mvh`, `scmrt.c` | The runtime: the heap, the Cheney collector, the payload and wrapper services. |
| `scmrt.h` | The `$scmrt` payload and wrapper services as other WCPL C modules see them. |
| `scmn.mvh` | Object primitives: the NaN-boxed representation, the accessors, the constructors. |
| `scmi.mvh`, `scmi.c` | Instruction macros and the callable slow paths behind them. |

## Using them

`marv` resolves a system include to `<dir>/<mod>.mvh` and its adapter to
`<dir>/<mod>.c` in the same directory, searching the system directories in
order, so pointing it at `srt/` is enough:

```
marv -L srt/ -I srt/ -t wcpl -c -o pi.wo pi.mvs
```

`-L` covers `(include system $scmrt)` and `(include system $scmi)`; `-I`
covers `(include $scmn)`, which generated code writes as a local include.

When compiling `scmi.c` with `wcpl`, the include base needs `srt/` as well,
because `scmi.c` includes `scmrt.h` by name:

```
wcpl -I srt/ -c -o scmi.wo srt/scmi.c
```

## Why they are separate

Generated code and `scmrt.c` share one heap and must agree on the object
layout down to the bit. That agreement is a property of the Scheme
implementation, not of MARV, and it changes whenever the compiler's
representation changes. Keeping these modules out of `lib/` keeps that
coupling visible, and keeps a MARV release from implying anything about the
Scheme runtime's stability.

`make install` installs `lib/` and `target/`, not this directory. An
installed `marv` therefore does not find `$scmrt` on its default search
path; a build that needs these modules names the directory with `-L` and
`-I`, as above. (`scmrt.h` was never installed either, since the install
rule copies only `.mvh` and `.c` files, so compiling `scmi.c` against an
installed tree never worked.)
