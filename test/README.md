# MARV Test Suite (`test/`)

The `test/` directory contains the automated test suite for the MARV compiler, RV64G assembly simulator, standard library headers/adapters, and CLI driver:

- `test/test.scm` — Unit tests for `.mvh` header expansion, `.wo`/`cc`/`rv64g` code generation, `.mvt` target loading, and execution of generated RV64G assembly on the built-in RV64G LP64D simulator (`test/rv64-sim.scm`).
- `test/rv64-sim.scm` — Self-contained R7RS RISC-V 64-bit (`RV64G` LP64D) assembly simulator used to verify register allocation, stack frames, tail calls, and standard library calls without requiring physical RISC-V hardware.
- `test/run-tests.sh` — End-to-end test runner that verifies:
  1. Single-source compilation of all 13 `lib/*.c` adapter files under both `cc` and `wcpl -c`.
  2. All unit and RV64G simulation assertions in `test/test.scm`.
  3. CLI `-c` compile-only mode across `cc`, `wcpl`, and `rv64g`.
  4. CLI compile-and-link mode across single-module and 3-module examples (including mixed object + `.mvs` inputs and automatic standard library `lib/*.c` dependency resolution).

## Running the Tests

From the repository root (`github/`), run:

```sh
bash test/run-tests.sh
```

Or run the Scheme unit and RV64G simulation test suite directly with `skint`:

```sh
skint test/test.scm
```
