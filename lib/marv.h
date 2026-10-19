/*
 * marv.h - Restricted C99 target runtime & UB-free intrinsics for MARV
 *
 * All modules linked together follow MARV calling conventions:
 *   - Scalars: marv_i32, marv_i64, marv_f32, marv_f64, marv_ptr
 *   - Pointers occupy 8-byte slots in memory (marv_ptr_load / marv_ptr_store)
 *   - Variadic functions take a trailing `marv_ptr ap` pointing to an array of
 *     16-byte `marv_va_arg_t` slots in the caller's stack frame (with ap[-1]
 *     holding the argument count, matching WCPL's va_arg_t model)
 *   - By-value record returns take a leading destination pointer `marv_ptr rp`
 *     and return void (matching WCPL's rp$ model)
 */

#ifndef MARV_H_
#define MARV_H_

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <math.h>

typedef int32_t  marv_i32;
typedef uint32_t marv_u32;
typedef int64_t  marv_i64;
typedef uint64_t marv_u64;
typedef float    marv_f32;
typedef double   marv_f64;
typedef void*    marv_ptr;

#define MARV_NULL ((marv_ptr)0)

/*
 * WCPL-compatible 16-byte vararg slot (`va_arg_t`).
 * In MARV's calling convention, a variadic function receives `marv_ptr ap`
 * pointing to `slots[0]`, while `slots[-1].va_int` holds the variadic argument
 * count (`va_count()`).
 */
typedef union marv_va_arg {
  marv_i32 va_int;
  marv_u32 va_uint;
  marv_i64 va_llong;
  marv_u64 va_ullong;
  marv_f64 va_double;
  uint64_t va_ptr_slot;
  uint64_t _pad[2]; /* 16-byte stride matching WCPL va_arg_t */
} marv_va_arg_t;

#define MARV_VA_COUNT(ap) \
  (((const marv_va_arg_t *)(uintptr_t)(ap))[-1].va_int)

#define MARV_VA_PTR(slot_ptr) \
  ((marv_ptr)(uintptr_t)((slot_ptr)->va_ptr_slot))

#if defined(_MSC_VER) && !defined(__cplusplus) && (_MSC_VER < 1900)
#define inline __inline
#endif

#if defined(__GNUC__) || defined(__clang__)
#define MARV_ALIGNED(n) __attribute__((aligned(n)))
#define MARV_UNUSED __attribute__((unused))
#elif defined(_MSC_VER)
#define MARV_ALIGNED(n) __declspec(align(n))
#define MARV_UNUSED
#else
#define MARV_ALIGNED(n)
#define MARV_UNUSED
#endif

#if defined(__has_attribute)
#  if __has_attribute(musttail)
#    define MARV_MUSTTAIL __attribute__((musttail))
#  else
#    define MARV_MUSTTAIL
#  endif
#else
#  define MARV_MUSTTAIL
#endif

/* --------------------------------------------------------------------------
 * Pointer Operations (Uniform 8-Byte Memory Slot across 32/64-bit targets)
 * -------------------------------------------------------------------------- */

static inline marv_ptr marv_ptr_add(marv_ptr p, marv_i32 off) {
  return (marv_ptr)((uint8_t*)p + (intptr_t)off);
}

static inline marv_ptr marv_ptr_add64(marv_ptr p, marv_i64 off) {
  return (marv_ptr)((uint8_t*)p + (intptr_t)off);
}

static inline marv_ptr marv_ptr_sub(marv_ptr p, marv_i32 off) {
  return (marv_ptr)((uint8_t*)p - (intptr_t)off);
}

static inline marv_i32 marv_ptr_diff(marv_ptr p1, marv_ptr p2) {
  return (marv_i32)((const uint8_t*)p1 - (const uint8_t*)p2);
}

static inline marv_i64 marv_ptr_diff64(marv_ptr p1, marv_ptr p2) {
  return (marv_i64)((const uint8_t*)p1 - (const uint8_t*)p2);
}

static inline marv_i32 marv_ptr_eqz(marv_ptr p) { return p == NULL ? 1 : 0; }
static inline marv_i32 marv_ptr_eq(marv_ptr a, marv_ptr b)   { return a == b ? 1 : 0; }
static inline marv_i32 marv_ptr_ne(marv_ptr a, marv_ptr b)   { return a != b ? 1 : 0; }
static inline marv_i32 marv_ptr_lt_u(marv_ptr a, marv_ptr b) { return (uintptr_t)a <  (uintptr_t)b ? 1 : 0; }
static inline marv_i32 marv_ptr_gt_u(marv_ptr a, marv_ptr b) { return (uintptr_t)a >  (uintptr_t)b ? 1 : 0; }
static inline marv_i32 marv_ptr_le_u(marv_ptr a, marv_ptr b) { return (uintptr_t)a <= (uintptr_t)b ? 1 : 0; }
static inline marv_i32 marv_ptr_ge_u(marv_ptr a, marv_ptr b) { return (uintptr_t)a >= (uintptr_t)b ? 1 : 0; }

static inline marv_ptr marv_ptr_of_i64(marv_i64 v) { return (marv_ptr)(uintptr_t)(uint64_t)v; }
static inline marv_i64 marv_i64_of_ptr(marv_ptr p) { return (marv_i64)(uint64_t)(uintptr_t)p; }

/* 8-byte uniform pointer slot load/store */
static inline marv_ptr marv_ptr_load(marv_ptr base, marv_i64 offset) {
  uint64_t slot = 0;
  memcpy(&slot, (const uint8_t*)base + offset, 8);
  return (marv_ptr)(uintptr_t)slot;
}

static inline void marv_ptr_store(marv_ptr base, marv_i64 offset, marv_ptr val) {
  uint64_t slot = (uint64_t)(uintptr_t)val;
  memcpy((uint8_t*)base + offset, &slot, 8);
}

/* --------------------------------------------------------------------------
 * Memory Loads & Stores (UB-free via memcpy)
 * -------------------------------------------------------------------------- */

static inline marv_i32 marv_i32_load(marv_ptr base, marv_i64 off) {
  marv_i32 v; memcpy(&v, (const uint8_t*)base + off, 4); return v;
}
static inline marv_i32 marv_i32_load8_s(marv_ptr base, marv_i64 off) {
  int8_t v; memcpy(&v, (const uint8_t*)base + off, 1); return (marv_i32)v;
}
static inline marv_i32 marv_i32_load8_u(marv_ptr base, marv_i64 off) {
  uint8_t v; memcpy(&v, (const uint8_t*)base + off, 1); return (marv_i32)v;
}
static inline marv_i32 marv_i32_load16_s(marv_ptr base, marv_i64 off) {
  int16_t v; memcpy(&v, (const uint8_t*)base + off, 2); return (marv_i32)v;
}
static inline marv_i32 marv_i32_load16_u(marv_ptr base, marv_i64 off) {
  uint16_t v; memcpy(&v, (const uint8_t*)base + off, 2); return (marv_i32)v;
}

static inline marv_i64 marv_i64_load(marv_ptr base, marv_i64 off) {
  marv_i64 v; memcpy(&v, (const uint8_t*)base + off, 8); return v;
}
static inline marv_i64 marv_i64_load8_s(marv_ptr base, marv_i64 off) {
  int8_t v; memcpy(&v, (const uint8_t*)base + off, 1); return (marv_i64)v;
}
static inline marv_i64 marv_i64_load8_u(marv_ptr base, marv_i64 off) {
  uint8_t v; memcpy(&v, (const uint8_t*)base + off, 1); return (marv_i64)v;
}
static inline marv_i64 marv_i64_load16_s(marv_ptr base, marv_i64 off) {
  int16_t v; memcpy(&v, (const uint8_t*)base + off, 2); return (marv_i64)v;
}
static inline marv_i64 marv_i64_load16_u(marv_ptr base, marv_i64 off) {
  uint16_t v; memcpy(&v, (const uint8_t*)base + off, 2); return (marv_i64)v;
}
static inline marv_i64 marv_i64_load32_s(marv_ptr base, marv_i64 off) {
  int32_t v; memcpy(&v, (const uint8_t*)base + off, 4); return (marv_i64)v;
}
static inline marv_i64 marv_i64_load32_u(marv_ptr base, marv_i64 off) {
  uint32_t v; memcpy(&v, (const uint8_t*)base + off, 4); return (marv_i64)v;
}

static inline marv_f32 marv_f32_load(marv_ptr base, marv_i64 off) {
  marv_f32 v; memcpy(&v, (const uint8_t*)base + off, 4); return v;
}
static inline marv_f64 marv_f64_load(marv_ptr base, marv_i64 off) {
  marv_f64 v; memcpy(&v, (const uint8_t*)base + off, 8); return v;
}

static inline void marv_i32_store(marv_ptr base, marv_i64 off, marv_i32 v) {
  memcpy((uint8_t*)base + off, &v, 4);
}
static inline void marv_i32_store8(marv_ptr base, marv_i64 off, marv_i32 v) {
  uint8_t b = (uint8_t)v; memcpy((uint8_t*)base + off, &b, 1);
}
static inline void marv_i32_store16(marv_ptr base, marv_i64 off, marv_i32 v) {
  uint16_t h = (uint16_t)v; memcpy((uint8_t*)base + off, &h, 2);
}

static inline void marv_i64_store(marv_ptr base, marv_i64 off, marv_i64 v) {
  memcpy((uint8_t*)base + off, &v, 8);
}
static inline void marv_i64_store8(marv_ptr base, marv_i64 off, marv_i64 v) {
  uint8_t b = (uint8_t)v; memcpy((uint8_t*)base + off, &b, 1);
}
static inline void marv_i64_store16(marv_ptr base, marv_i64 off, marv_i64 v) {
  uint16_t h = (uint16_t)v; memcpy((uint8_t*)base + off, &h, 2);
}
static inline void marv_i64_store32(marv_ptr base, marv_i64 off, marv_i64 v) {
  uint32_t w = (uint32_t)v; memcpy((uint8_t*)base + off, &w, 4);
}

static inline void marv_f32_store(marv_ptr base, marv_i64 off, marv_f32 v) {
  memcpy((uint8_t*)base + off, &v, 4);
}
static inline void marv_f64_store(marv_ptr base, marv_i64 off, marv_f64 v) {
  memcpy((uint8_t*)base + off, &v, 8);
}

/* --------------------------------------------------------------------------
 * 32-bit Integer Intrinsics (Two's Complement, Masked Shifts)
 * -------------------------------------------------------------------------- */

static inline marv_i32 marv_i32_add(marv_i32 a, marv_i32 b) { return (marv_i32)((marv_u32)a + (marv_u32)b); }
static inline marv_i32 marv_i32_sub(marv_i32 a, marv_i32 b) { return (marv_i32)((marv_u32)a - (marv_u32)b); }
static inline marv_i32 marv_i32_mul(marv_i32 a, marv_i32 b) { return (marv_i32)((marv_u32)a * (marv_u32)b); }
static inline marv_i32 marv_i32_div_s(marv_i32 a, marv_i32 b) { return a / b; }
static inline marv_i32 marv_i32_div_u(marv_i32 a, marv_i32 b) { return (marv_i32)((marv_u32)a / (marv_u32)b); }
static inline marv_i32 marv_i32_rem_s(marv_i32 a, marv_i32 b) { return a % b; }
static inline marv_i32 marv_i32_rem_u(marv_i32 a, marv_i32 b) { return (marv_i32)((marv_u32)a % (marv_u32)b); }
static inline marv_i32 marv_i32_and(marv_i32 a, marv_i32 b) { return a & b; }
static inline marv_i32 marv_i32_or(marv_i32 a, marv_i32 b)  { return a | b; }
static inline marv_i32 marv_i32_xor(marv_i32 a, marv_i32 b) { return a ^ b; }
static inline marv_i32 marv_i32_shl(marv_i32 a, marv_i32 b)   { return (marv_i32)((marv_u32)a << (b & 31)); }
static inline marv_i32 marv_i32_shr_s(marv_i32 a, marv_i32 b) { return a >> (b & 31); }
static inline marv_i32 marv_i32_shr_u(marv_i32 a, marv_i32 b) { return (marv_i32)((marv_u32)a >> (b & 31)); }
static inline marv_i32 marv_i32_rotl(marv_i32 a, marv_i32 b) {
  marv_u32 u = (marv_u32)a, k = (marv_u32)b & 31u;
  return (marv_i32)((u << k) | (u >> ((32u - k) & 31u)));
}
static inline marv_i32 marv_i32_rotr(marv_i32 a, marv_i32 b) {
  marv_u32 u = (marv_u32)a, k = (marv_u32)b & 31u;
  return (marv_i32)((u >> k) | (u << ((32u - k) & 31u)));
}
static inline marv_i32 marv_i32_clz(marv_i32 a) {
  return a == 0 ? 32 : (marv_i32)__builtin_clz((marv_u32)a);
}
static inline marv_i32 marv_i32_ctz(marv_i32 a) {
  return a == 0 ? 32 : (marv_i32)__builtin_ctz((marv_u32)a);
}
static inline marv_i32 marv_i32_popcnt(marv_i32 a) {
  return (marv_i32)__builtin_popcount((marv_u32)a);
}

static inline marv_i32 marv_i32_eqz(marv_i32 a) { return a == 0 ? 1 : 0; }
static inline marv_i32 marv_i32_eq(marv_i32 a, marv_i32 b)   { return a == b ? 1 : 0; }
static inline marv_i32 marv_i32_ne(marv_i32 a, marv_i32 b)   { return a != b ? 1 : 0; }
static inline marv_i32 marv_i32_lt_s(marv_i32 a, marv_i32 b) { return a <  b ? 1 : 0; }
static inline marv_i32 marv_i32_lt_u(marv_i32 a, marv_i32 b) { return (marv_u32)a <  (marv_u32)b ? 1 : 0; }
static inline marv_i32 marv_i32_gt_s(marv_i32 a, marv_i32 b) { return a >  b ? 1 : 0; }
static inline marv_i32 marv_i32_gt_u(marv_i32 a, marv_i32 b) { return (marv_u32)a >  (marv_u32)b ? 1 : 0; }
static inline marv_i32 marv_i32_le_s(marv_i32 a, marv_i32 b) { return a <= b ? 1 : 0; }
static inline marv_i32 marv_i32_le_u(marv_i32 a, marv_i32 b) { return (marv_u32)a <= (marv_u32)b ? 1 : 0; }
static inline marv_i32 marv_i32_ge_s(marv_i32 a, marv_i32 b) { return a >= b ? 1 : 0; }
static inline marv_i32 marv_i32_ge_u(marv_i32 a, marv_i32 b) { return (marv_u32)a >= (marv_u32)b ? 1 : 0; }

/* --------------------------------------------------------------------------
 * 64-bit Integer Intrinsics
 * -------------------------------------------------------------------------- */

static inline marv_i64 marv_i64_add(marv_i64 a, marv_i64 b) { return (marv_i64)((marv_u64)a + (marv_u64)b); }
static inline marv_i64 marv_i64_sub(marv_i64 a, marv_i64 b) { return (marv_i64)((marv_u64)a - (marv_u64)b); }
static inline marv_i64 marv_i64_mul(marv_i64 a, marv_i64 b) { return (marv_i64)((marv_u64)a * (marv_u64)b); }
static inline marv_i64 marv_i64_div_s(marv_i64 a, marv_i64 b) { return a / b; }
static inline marv_i64 marv_i64_div_u(marv_i64 a, marv_i64 b) { return (marv_i64)((marv_u64)a / (marv_u64)b); }
static inline marv_i64 marv_i64_rem_s(marv_i64 a, marv_i64 b) { return a % b; }
static inline marv_i64 marv_i64_rem_u(marv_i64 a, marv_i64 b) { return (marv_i64)((marv_u64)a % (marv_u64)b); }
static inline marv_i64 marv_i64_and(marv_i64 a, marv_i64 b) { return a & b; }
static inline marv_i64 marv_i64_or(marv_i64 a, marv_i64 b)  { return a | b; }
static inline marv_i64 marv_i64_xor(marv_i64 a, marv_i64 b) { return a ^ b; }
static inline marv_i64 marv_i64_shl(marv_i64 a, marv_i64 b)   { return (marv_i64)((marv_u64)a << (b & 63)); }
static inline marv_i64 marv_i64_shr_s(marv_i64 a, marv_i64 b) { return a >> (b & 63); }
static inline marv_i64 marv_i64_shr_u(marv_i64 a, marv_i64 b) { return (marv_i64)((marv_u64)a >> (b & 63)); }
static inline marv_i64 marv_i64_rotl(marv_i64 a, marv_i64 b) {
  marv_u64 u = (marv_u64)a, k = (marv_u64)b & 63u;
  return (marv_i64)((u << k) | (u >> ((64u - k) & 63u)));
}
static inline marv_i64 marv_i64_rotr(marv_i64 a, marv_i64 b) {
  marv_u64 u = (marv_u64)a, k = (marv_u64)b & 63u;
  return (marv_i64)((u >> k) | (u << ((64u - k) & 63u)));
}
static inline marv_i64 marv_i64_clz(marv_i64 a) {
  return a == 0 ? 64 : (marv_i64)__builtin_clzll((marv_u64)a);
}
static inline marv_i64 marv_i64_ctz(marv_i64 a) {
  return a == 0 ? 64 : (marv_i64)__builtin_ctzll((marv_u64)a);
}
static inline marv_i64 marv_i64_popcnt(marv_i64 a) {
  return (marv_i64)__builtin_popcountll((marv_u64)a);
}

static inline marv_i32 marv_i64_eqz(marv_i64 a) { return a == 0 ? 1 : 0; }
static inline marv_i32 marv_i64_eq(marv_i64 a, marv_i64 b)   { return a == b ? 1 : 0; }
static inline marv_i32 marv_i64_ne(marv_i64 a, marv_i64 b)   { return a != b ? 1 : 0; }
static inline marv_i32 marv_i64_lt_s(marv_i64 a, marv_i64 b) { return a <  b ? 1 : 0; }
static inline marv_i32 marv_i64_lt_u(marv_i64 a, marv_i64 b) { return (marv_u64)a <  (marv_u64)b ? 1 : 0; }
static inline marv_i32 marv_i64_gt_s(marv_i64 a, marv_i64 b) { return a >  b ? 1 : 0; }
static inline marv_i32 marv_i64_gt_u(marv_i64 a, marv_i64 b) { return (marv_u64)a >  (marv_u64)b ? 1 : 0; }
static inline marv_i32 marv_i64_le_s(marv_i64 a, marv_i64 b) { return a <= b ? 1 : 0; }
static inline marv_i32 marv_i64_le_u(marv_i64 a, marv_i64 b) { return (marv_u64)a <= (marv_u64)b ? 1 : 0; }
static inline marv_i32 marv_i64_ge_s(marv_i64 a, marv_i64 b) { return a >= b ? 1 : 0; }
static inline marv_i32 marv_i64_ge_u(marv_i64 a, marv_i64 b) { return (marv_u64)a >= (marv_u64)b ? 1 : 0; }

/* --------------------------------------------------------------------------
 * Floating-Point Intrinsics (f32 / f64)
 * -------------------------------------------------------------------------- */

static inline marv_f32 marv_f32_add(marv_f32 a, marv_f32 b) { return a + b; }
static inline marv_f32 marv_f32_sub(marv_f32 a, marv_f32 b) { return a - b; }
static inline marv_f32 marv_f32_mul(marv_f32 a, marv_f32 b) { return a * b; }
static inline marv_f32 marv_f32_div(marv_f32 a, marv_f32 b) { return a / b; }
static inline marv_f32 marv_f32_sqrt(marv_f32 a)            { return sqrtf(a); }
static inline marv_f32 marv_f32_abs(marv_f32 a)             { return fabsf(a); }
static inline marv_f32 marv_f32_neg(marv_f32 a)             { return -a; }
static inline marv_f32 marv_f32_copysign(marv_f32 a, marv_f32 b) { return copysignf(a, b); }
static inline marv_f32 marv_f32_ceil(marv_f32 a)            { return ceilf(a); }
static inline marv_f32 marv_f32_floor(marv_f32 a)           { return floorf(a); }
static inline marv_f32 marv_f32_trunc(marv_f32 a)           { return truncf(a); }
static inline marv_f32 marv_f32_nearest(marv_f32 a)         { return rintf(a); }
static inline marv_f32 marv_f32_min(marv_f32 a, marv_f32 b) { return fminf(a, b); }
static inline marv_f32 marv_f32_max(marv_f32 a, marv_f32 b) { return fmaxf(a, b); }

static inline marv_i32 marv_f32_eq(marv_f32 a, marv_f32 b) { return a == b ? 1 : 0; }
static inline marv_i32 marv_f32_ne(marv_f32 a, marv_f32 b) { return a != b ? 1 : 0; }
static inline marv_i32 marv_f32_lt(marv_f32 a, marv_f32 b) { return a <  b ? 1 : 0; }
static inline marv_i32 marv_f32_gt(marv_f32 a, marv_f32 b) { return a >  b ? 1 : 0; }
static inline marv_i32 marv_f32_le(marv_f32 a, marv_f32 b) { return a <= b ? 1 : 0; }
static inline marv_i32 marv_f32_ge(marv_f32 a, marv_f32 b) { return a >= b ? 1 : 0; }

static inline marv_f64 marv_f64_add(marv_f64 a, marv_f64 b) { return a + b; }
static inline marv_f64 marv_f64_sub(marv_f64 a, marv_f64 b) { return a - b; }
static inline marv_f64 marv_f64_mul(marv_f64 a, marv_f64 b) { return a * b; }
static inline marv_f64 marv_f64_div(marv_f64 a, marv_f64 b) { return a / b; }
static inline marv_f64 marv_f64_sqrt(marv_f64 a)            { return sqrt(a); }
static inline marv_f64 marv_f64_abs(marv_f64 a)             { return fabs(a); }
static inline marv_f64 marv_f64_neg(marv_f64 a)             { return -a; }
static inline marv_f64 marv_f64_copysign(marv_f64 a, marv_f64 b) { return copysign(a, b); }
static inline marv_f64 marv_f64_ceil(marv_f64 a)            { return ceil(a); }
static inline marv_f64 marv_f64_floor(marv_f64 a)           { return floor(a); }
static inline marv_f64 marv_f64_trunc(marv_f64 a)           { return trunc(a); }
static inline marv_f64 marv_f64_nearest(marv_f64 a)         { return rint(a); }
static inline marv_f64 marv_f64_min(marv_f64 a, marv_f64 b) { return fmin(a, b); }
static inline marv_f64 marv_f64_max(marv_f64 a, marv_f64 b) { return fmax(a, b); }

static inline marv_i32 marv_f64_eq(marv_f64 a, marv_f64 b) { return a == b ? 1 : 0; }
static inline marv_i32 marv_f64_ne(marv_f64 a, marv_f64 b) { return a != b ? 1 : 0; }
static inline marv_i32 marv_f64_lt(marv_f64 a, marv_f64 b) { return a <  b ? 1 : 0; }
static inline marv_i32 marv_f64_gt(marv_f64 a, marv_f64 b) { return a >  b ? 1 : 0; }
static inline marv_i32 marv_f64_le(marv_f64 a, marv_f64 b) { return a <= b ? 1 : 0; }
static inline marv_i32 marv_f64_ge(marv_f64 a, marv_f64 b) { return a >= b ? 1 : 0; }

/* --------------------------------------------------------------------------
 * Conversions & Reinterpretations
 * -------------------------------------------------------------------------- */

static inline marv_i32 marv_i32_wrap_i64(marv_i64 a)       { return (marv_i32)(marv_u32)(marv_u64)a; }
static inline marv_i64 marv_i64_extend_i32_s(marv_i32 a)   { return (marv_i64)a; }
static inline marv_i64 marv_i64_extend_i32_u(marv_i32 a)   { return (marv_i64)(marv_u32)a; }
static inline marv_i32 marv_i32_extend8_s(marv_i32 a)      { return (marv_i32)(int8_t)a; }
static inline marv_i32 marv_i32_extend16_s(marv_i32 a)     { return (marv_i32)(int16_t)a; }
static inline marv_i64 marv_i64_extend8_s(marv_i64 a)      { return (marv_i64)(int8_t)a; }
static inline marv_i64 marv_i64_extend16_s(marv_i64 a)     { return (marv_i64)(int16_t)a; }
static inline marv_i64 marv_i64_extend32_s(marv_i64 a)     { return (marv_i64)(int32_t)a; }

static inline marv_i32 marv_i32_trunc_f32_s(marv_f32 a)    { return (marv_i32)a; }
static inline marv_i32 marv_i32_trunc_f32_u(marv_f32 a)    { return (marv_i32)(marv_u32)a; }
static inline marv_i32 marv_i32_trunc_f64_s(marv_f64 a)    { return (marv_i32)a; }
static inline marv_i32 marv_i32_trunc_f64_u(marv_f64 a)    { return (marv_i32)(marv_u32)a; }
static inline marv_i64 marv_i64_trunc_f32_s(marv_f32 a)    { return (marv_i64)a; }
static inline marv_i64 marv_i64_trunc_f32_u(marv_f32 a)    { return (marv_i64)(marv_u64)a; }
static inline marv_i64 marv_i64_trunc_f64_s(marv_f64 a)    { return (marv_i64)a; }
static inline marv_i64 marv_i64_trunc_f64_u(marv_f64 a)    { return (marv_i64)(marv_u64)a; }

static inline marv_f32 marv_f32_convert_i32_s(marv_i32 a)  { return (marv_f32)a; }
static inline marv_f32 marv_f32_convert_i32_u(marv_i32 a)  { return (marv_f32)(marv_u32)a; }
static inline marv_f32 marv_f32_convert_i64_s(marv_i64 a)  { return (marv_f32)a; }
static inline marv_f32 marv_f32_convert_i64_u(marv_i64 a)  { return (marv_f32)(marv_u64)a; }
static inline marv_f32 marv_f32_demote_f64(marv_f64 a)     { return (marv_f32)a; }

static inline marv_f64 marv_f64_convert_i32_s(marv_i32 a)  { return (marv_f64)a; }
static inline marv_f64 marv_f64_convert_i32_u(marv_i32 a)  { return (marv_f64)(marv_u32)a; }
static inline marv_f64 marv_f64_convert_i64_s(marv_i64 a)  { return (marv_f64)a; }
static inline marv_f64 marv_f64_convert_i64_u(marv_i64 a)  { return (marv_f64)(marv_u64)a; }
static inline marv_f64 marv_f64_promote_f32(marv_f32 a)    { return (marv_f64)a; }

static inline marv_i32 marv_i32_reinterpret_f32(marv_f32 a) {
  marv_i32 r; memcpy(&r, &a, 4); return r;
}
static inline marv_i64 marv_i64_reinterpret_f64(marv_f64 a) {
  marv_i64 r; memcpy(&r, &a, 8); return r;
}
static inline marv_f32 marv_f32_reinterpret_i32(marv_i32 a) {
  marv_f32 r; memcpy(&r, &a, 4); return r;
}
static inline marv_f64 marv_f64_reinterpret_i64(marv_i64 a) {
  marv_f64 r; memcpy(&r, &a, 8); return r;
}

#endif /* MARV_H_ */
