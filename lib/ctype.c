/*
 * lib/ctype.c — Single-source MARV-ABI adapter for `include/ctype.mvh`
 *
 * Compiles from this single source file with BOTH:
 *   - `wcpl -c lib/ctype.c -o lib/ctype.wo` (WASM `.wo` target)
 *   - `gcc  -c lib/ctype.c -o lib/ctype.o`  (C99 & RV64G targets)
 */

#include <stdint.h>

#if defined(__WCPL__)
#pragma module "ctype"
#else
#define islower  ctype_islower
#define isupper  ctype_isupper
#define isalpha  ctype_isalpha
#define isdigit  ctype_isdigit
#define isalnum  ctype_isalnum
#define isprint  ctype_isprint
#define iscntrl  ctype_iscntrl
#define isspace  ctype_isspace
#define isgraph  ctype_isgraph
#define ispunct  ctype_ispunct
#define isxdigit ctype_isxdigit
#define isascii  ctype_isascii
#define isblank  ctype_isblank
#define toupper  ctype_toupper
#define tolower  ctype_tolower
#endif

typedef int32_t  marv_i32;
typedef uint32_t marv_u32;

marv_i32 islower(marv_i32 c) {
  return (c >= 'a' && c <= 'z') ? 1 : 0;
}

marv_i32 isupper(marv_i32 c) {
  return (c >= 'A' && c <= 'Z') ? 1 : 0;
}

marv_i32 isalpha(marv_i32 c) {
  return (islower(c) || isupper(c)) ? 1 : 0;
}

marv_i32 isdigit(marv_i32 c) {
  return (((marv_u32)c - (marv_u32)'0') <= 9U) ? 1 : 0;
}

marv_i32 isalnum(marv_i32 c) {
  return (isalpha(c) || isdigit(c)) ? 1 : 0;
}

marv_i32 isprint(marv_i32 c) {
  return (c >= ' ' && c <= '~') ? 1 : 0;
}

marv_i32 iscntrl(marv_i32 c) {
  return !isprint(c) ? 1 : 0;
}

marv_i32 isspace(marv_i32 c) {
  return (c == ' ' || c == '\n' || c == '\t' || c == '\r' || c == '\v' || c == '\f') ? 1 : 0;
}

marv_i32 isgraph(marv_i32 c) {
  return (isprint(c) && !isspace(c)) ? 1 : 0;
}

marv_i32 ispunct(marv_i32 c) {
  return (isprint(c) && !isalnum(c) && !isspace(c)) ? 1 : 0;
}

marv_i32 isxdigit(marv_i32 c) {
  return (isdigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')) ? 1 : 0;
}

marv_i32 isascii(marv_i32 c) {
  return ((c & ~0x7f) == 0) ? 1 : 0;
}

marv_i32 isblank(marv_i32 c) {
  return (c == '\t' || c == ' ') ? 1 : 0;
}

marv_i32 toupper(marv_i32 c) {
  return islower(c) ? (c & ~32) : c;
}

marv_i32 tolower(marv_i32 c) {
  return isupper(c) ? (c | 32) : c;
}
