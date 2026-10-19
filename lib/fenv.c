/*
 * lib/fenv.c — Single-source MARV-ABI adapter for `include/fenv.mvh`
 *
 * Compiles from this single source file with BOTH:
 *   - `wcpl -c lib/fenv.c -o lib/fenv.wo` (WASM `.wo` target)
 *   - `gcc  -c lib/fenv.c -o lib/fenv.o`  (C99 & RV64G targets)
 */

#include <stdint.h>

#if defined(__WCPL__)
#pragma module "fenv"
#else
#define feclearexcept  fenv_feclearexcept
#define fegetexceptflag fenv_fegetexceptflag
#define feraiseexcept  fenv_feraiseexcept
#define fesetexceptflag fenv_fesetexceptflag
#define fetestexcept   fenv_fetestexcept
#define fegetround     fenv_fegetround
#define fesetround     fenv_fesetround
#define fegetenv       fenv_fegetenv
#define feholdexcept   fenv_feholdexcept
#define fesetenv       fenv_fesetenv
#define feupdateenv    fenv_feupdateenv
#endif

typedef int32_t marv_i32;
typedef void*   marv_ptr;

void feclearexcept(marv_i32 excepts) {
  (void)excepts;
}

void fegetexceptflag(marv_ptr flagp, marv_i32 excepts) {
  (void)excepts;
  if (flagp) *(int32_t *)flagp = 0;
}

void feraiseexcept(marv_i32 excepts) {
  (void)excepts;
}

void fesetexceptflag(marv_ptr flagp, marv_i32 excepts) {
  (void)flagp;
  (void)excepts;
}

marv_i32 fetestexcept(marv_i32 excepts) {
  (void)excepts;
  return 0;
}

marv_i32 fegetround(void) {
  return 0; /* FE_TONEAREST */
}

marv_i32 fesetround(marv_i32 round) {
  return (round == 0) ? 0 : -1;
}

void fegetenv(marv_ptr envp) {
  if (envp) *(int32_t *)envp = 0;
}

marv_i32 feholdexcept(marv_ptr envp) {
  if (envp) *(int32_t *)envp = 0;
  return 0;
}

void fesetenv(marv_ptr envp) {
  (void)envp;
}

void feupdateenv(marv_ptr envp) {
  (void)envp;
}
