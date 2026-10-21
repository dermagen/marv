# MARV Examples (`examples/`)

This directory contains single-module and multi-module example programs written in MARV (`.mvs` source files and `.mvh` interface headers).

> **Precondition:** All command lines below assume that the `marv` compiler binary has been built from `m.c r.c v.c a.c` and installed in your `PATH` (for example via `make && sudo make install`; see the top-level [`README.md`](../README.md)).

## Overview of Examples

| Path | Description |
|---|---|
| `queens.mvs` | **Eight Queens Puzzle Solver** — Solves the classic 8-queens problem (92 solutions) using mutual tail recursion (`return_call` and `return_call_indirect`) with bounded $O(1)$ stack space, addressable stack frames, and formatted output via `$stdio:snprintf` and `$stdio:fputs`. |
| `dirent_demo.mvs` | **Directory Traversal & Sorting Demo** — Demonstrates `$dirent:opendir`, `$dirent:readdir`, `$dirent:rewinddir`, `$dirent:telldir`, `$dirent:seekdir`, `$dirent:closedir`, and `$dirent:scandir` using both `$dirent:alphasort` and custom MARV filter/comparison callbacks passed via `ref.func` over the `examples/` directory. |
| `libc_demo.mvs` | **Standard Library Tour** — Exercises `stdlib` (`malloc`, `free`, `qsort`, `bsearch`, `div`, `lldiv`, `strtoll`, `strtod`), `stdio` (`snprintf` variadic calls, `stdout`, `fopen`/`fwrite`/`fclose`), user-defined variadic functions (`va.start`, `va.count`, `va.arg`), `sys.stat` (`stat` record queries), `ctype`, `errno`, and the nested module `math.vec` (`dot`, `cross`, `norm`). |
| `multimod/` | **3-Module Separate Compilation Example** (`vec3.mvh`/`vec3.mvs`, `stats.mvh`/`stats.mvs`, `app.mvh`/`app.mvs`) — Demonstrates separate compilation and linking across three MARV modules that share records (`$vec3`, `$sample`), cross-module function callbacks (`ref.func`), by-value record returns (`rp$`), cross-module variadic functions (`ap$`), exported mutable globals (`$vec3:ops_count`), and exported read-only data segments (`$stats:tag_label`). |

---

## Building and Running the Examples

Run all commands from the repository root:

### 1. Eight Queens (`examples/queens.mvs`)

```sh
# Compile and link to a native executable (CC target)
marv -t cc -o queens examples/queens.mvs
./queens

# Compile and link to a WebAssembly executable (WCPL target)
marv -t wcpl -o queens.wasm examples/queens.mvs
```

### 2. Directory Traversal (`examples/dirent_demo.mvs`)

```sh
marv -t cc -o dirent_demo examples/dirent_demo.mvs
./dirent_demo
```

### 3. Standard Library Tour (`examples/libc_demo.mvs`)

```sh
marv -t cc -o libc_demo examples/libc_demo.mvs
./libc_demo
```

### 4. Multi-Module Separate Compilation (`examples/multimod/`)

You can compile and link all three `.mvs` files in a single command, or compile individual modules to object files with `-c` first and then link them together:

```sh
# Compile vec3.mvs to an object file (.c for cc, or .wo for wcpl)
marv -t cc -c examples/multimod/vec3.mvs

# Compile the remaining modules and link everything into an executable
marv -t cc -o multimod_app \
  examples/multimod/vec3.c \
  examples/multimod/stats.mvs \
  examples/multimod/app.mvs

./multimod_app
```
