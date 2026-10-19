/*
 * lib/math/vec.c — Single-source MARV-ABI adapter for `include/math.vec.mvh`
 *
 * Compiles from this single source file with BOTH:
 *   - `wcpl -c lib/math/vec.c -o lib/math.vec.wo` (WASM `.wo` target)
 *   - `gcc  -c lib/math/vec.c -o lib/math/vec.o`  (C99 & RV64G targets)
 *
 * Notice how `cross` explicitly takes `marv_vec3_t *rp` as its first parameter
 * (`rp$` convention), which produces the exact same signature on WCPL, C99,
 * and RV64G without duplicating the function body.
 */

#if defined(__WCPL__)
#pragma module "math.vec"
#include <math.h>
#else
#include <math.h>
#define dot   math_vec_dot
#define cross math_vec_cross
#define norm  math_vec_norm
#endif

typedef struct marv_vec3 {
  double x;
  double y;
  double z;
} marv_vec3_t;

double dot(const marv_vec3_t *a, const marv_vec3_t *b) {
  return a->x * b->x + a->y * b->y + a->z * b->z;
}

void cross(marv_vec3_t *rp, const marv_vec3_t *a, const marv_vec3_t *b) {
  double ax = a->x;
  double ay = a->y;
  double az = a->z;
  double bx = b->x;
  double by = b->y;
  double bz = b->z;
  rp->x = ay * bz - az * by;
  rp->y = az * bx - ax * bz;
  rp->z = ax * by - ay * bx;
}

double norm(const marv_vec3_t *v) {
  return sqrt(v->x * v->x + v->y * v->y + v->z * v->z);
}
