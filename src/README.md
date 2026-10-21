# MARV Compiler Command-Line Interface

The MARV compiler and build driver is provided in two forms:

1. **Standalone C Sources (`m.c`, `r.c`, `v.c`, `a.c` / `src/sf/`)** — Compiled by the [#F (`sfc`)](https://github.com/false-schemers/sharpF) Scheme-to-C compiler from the four compilation units in `src/sf/`:
   - `m.sf` — the LibL runtime library (base unit),
   - `r.sf` — the compiler proper (parser, flattening, emitters),
   - `v.sf` — interface loading, lowering, target descriptions, and the linker runner,
   - `a.sf` — the driver (command-line parsing and `main`).

   Compile the generated C files in the repository root via `make` or directly with a C compiler:
   ```sh
   cc -O2 -o marv m.c r.c v.c a.c -lm
   ```
   Or on Windows with MSVC (`cl`):
   ```bat
   cl /nologo /O2 /Fe:marv.exe m.c r.c v.c a.c
   ```
   To regenerate the C sources from `src/sf/` with `sfc` and refresh the copies in the repository root:
   ```sh
   cd src/sf && sfc m.sf r.sf v.sf a.sf && cp m.c r.c v.c a.c ../..
   ```
2. **R7RS Scheme Source (`src/skint/marv.scm`)** — Runs directly as a standard R7RS Scheme program on [Skint](https://github.com/false-schemers/skint):
   ```sh
   skint -p src/skint/marv.scm ...
   ```

## Synopsis

```sh
marv -t TARGDES [-c] [OPTION]... FILE...
```

## Options

| Option | Long Option | Argument | Description |
|---|---|---|---|
| `-t` | `--target=TARGDES` | `TARGDES` | **Required.** Target designation name or path mapped to a `.mvt` target description file. |
| `-T` | `--target-dir=DIR` | `DIR` | Add `DIR` to the `.mvt` target description lookup path (default: `("target/" "<PREFIX>/target")` or `("target/" "targets/")`). |
| `-c` | `--compile` | *(none)* | Compile one `.mvs` file into a target object file without linking. |
| `-o` | `--output=FILE` | `FILE` | Output pathname (optional override in `-c` mode; required executable name when `-c` is omitted). |
| `-L` | `--system-dir=DIR` | `DIR` | Add `DIR` to the system `.mvh` header and `.c` adapter lookup path (default: `("lib/" "<PREFIX>/lib")` or `("lib/" "libs/")`). |
| `-I` | `--include-dir=DIR` | `DIR` | Add `DIR` to the project/local `.mvh` header lookup path (default: `("./")`). |
| `-v` | `--verbose` | *(none)* | Print each compilation step line and the linker command line to `stderr` before executing. |
| `-h` | `--help` | *(none)* | Display command-line usage and options. |

Options follow standard POSIX syntax (parsed via `(skint getopt)`): short flags may be clustered (e.g. `-vc`), and option arguments may be passed either separately (`-t cc`, `-o app`) or attached (`-tcc`, `--target=cc`, `--output=app`).

---

## Operating Modes

### 1. Compile-Only Mode (`-c`)

When `-c` is given, `marv` accepts a single `.mvs` source file and compiles it to the object format specified by the target's `.mvt` file:

- `.wo` for `wcpl` targets
- `.s` for `rv64g` targets
- `.c` for `cc` targets

By default, the output file is written to the **same directory** as the input `.mvs` file with the same base name and the target's object extension. You can override the output location with `-o FILE`.

```sh
# Generates examples/queens.wo, examples/queens.c, and examples/queens.s
marv -T target -t wcpl  -c examples/queens.mvs
marv -T target -t cc    -c examples/queens.mvs
marv -T target -t rv64g -c examples/queens.mvs

# Override output file path with -o
marv -T target -t wcpl -c -o /tmp/queens.wo examples/queens.mvs
```

### 2. Compile-and-Link Mode (without `-c`)

When `-c` is omitted, `-o FILE` is required and names the output executable (a native binary for `cc`/`rv64g` targets, or a `.wasm` binary for `wcpl`).

Input files on the command line can be:
- One or more `.mvs` source files,
- Pre-compiled target object files (any file whose extension is not `.mvs`, such as `.wo`, `.c`, `.s`, or `.o`), or
- A mix of `.mvs` files and target object files.

In compile-and-link mode, `marv`:
1. Compiles each `.mvs` file on the command line to a temporary target object file in the system temporary directory (`TMPDIR`, `TMP`, or `TEMP`, falling back to `/tmp` on Unix-like systems or `.` on Windows).
2. Inspects the `(include system $...)` dependencies of the input modules, locates the corresponding system `.mvh` headers in the `-L` lookup directories (default `("lib/" "<PREFIX>/lib")` or `("lib/" "libs/")`), and pairs each with its `.c` adapter file in the same directory (`lib/<module>.c`).
3. Invokes the target compiler/linker defined in the `.mvt` file with `-o FILE`, the module object files, and the required system `.c` adapter files.
4. Deletes the temporary object files created during the run when finished.

```sh
# Build a native executable from three .mvs modules
marv -T target -t cc -o app \
  examples/multimod/vec3.mvs examples/multimod/stats.mvs examples/multimod/app.mvs

# Build a WebAssembly (.wasm) binary mixing a pre-compiled .wo object and .mvs sources
marv -T target -t wcpl -o app.wasm \
  examples/multimod/vec3.wo examples/multimod/stats.mvs examples/multimod/app.mvs
```

---

## Lookup Directory Lists

`marv` maintains three configurable search path lists:

1. **Target Description Directories (`-T`)** — Defaults to `("target/" "<PREFIX>/target")` when the compiler is built with `-D PREFIX=...` (e.g. `/usr/local/share/marv/` via `Makefile`), or `("target/" "targets/")` if `-D PREFIX` was not given. Searched when resolving `-t TARGDES` to `<dir>/<TARGDES>.mvt` (after checking `<TARGDES>.mvt` directly).
2. **System Header & Adapter Directories (`-L`)** — Defaults to `("lib/" "<PREFIX>/lib")` when the compiler is built with `-D PREFIX=...`, or `("lib/" "libs/")` if `-D PREFIX` was not given. Searched when resolving `(include system $foo.bar)` to `<dir>/foo.bar.mvh` and its companion adapter `<dir>/foo.bar.c`.
3. **Local Header Directories (`-I`, default `("./")`)** — Searched (after the directory of the including file) when resolving `(include $foo.bar)` to `<dir>/foo.bar.mvh`.
