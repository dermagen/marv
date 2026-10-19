/* Single-source MARV-ABI adapter for `string` (WCPL + C99 + RV64G) */
#if defined(__WCPL__)
#pragma module "string"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <sys/intrs.h>

void bzero(void *dst, size_t n)
{
  _bzero(dst, n);
}

void *memccpy(void *dst, const void *src, int c, size_t n)
{
  char *q = dst; const char *p = src;
  char ch;
  while (n--) {
    *q++ = ch = *p++;
    if (ch == (char)c) return q;
  }
  return NULL;
}

void *memchr(const void *s, int c, size_t n)
{
  const char *sp = s;
  while (n--) {
    if (*sp == (char)c) return (void *)sp;
    sp++;
  }
  return NULL;
}

int memcmp(const void *s1, const void *s2, size_t n)
{
  const char *c1 = s1, *c2 = s2;
  int d = 0;
  while (n--) {
    d = (unsigned char)*c1++ - (unsigned char)*c2++;
    if (d) break;
  }
  return d;
}

void *memcpy(void *dst, const void *src, size_t n)
{
  _bcopy(dst, src, n);
  return dst;
}

/* See http://www-igm.univ-mlv.fr/~lecroq/string/  */
void *memmem(const void *haystack, size_t n, const void *needle, size_t m)
{
  const unsigned char *y = (const unsigned char *)haystack;
  const unsigned char *x = (const unsigned char *)needle;
  size_t j, k, l;
  if (m > n || !m || !n)
    return NULL;
  if (1 != m) {
    if (x[0] == x[1]) { 
      k = 2; l = 1;
    } else {
      k = 1; l = 2;
    }
    j = 0;
    while (j <= n - m) {
      if (x[1] != y[j + 1]) {
        j += k;
      } else {
        if (!memcmp(x + 2, y + j + 2, m - 2) && x[0] == y[j])
          return (void *)&y[j];
        j += l;
      }
    }
  } else {
    do {
      if (*y == *x) return (void *)y;
      y++;
    } while (--n);
  }
  return NULL;
}

void *memmove(void *dst, const void *src, size_t n)
{
  _bcopy(dst, src, n);
  return dst;
}

void *memset(void *dst, int c, size_t n)
{
  _bfill(dst, c, n);
  return dst;
}

void memswap(void *m1, void *m2, size_t n)
{
  char *p = m1, *q = m2;
  while (n--) {
    int tmp = *p;
    *p = *q; *q = tmp;
    p++; q++;
  }
}

int strcasecmp(const char *s1, const char *s2)
{
  const unsigned char *c1 = (const unsigned char *)s1;
  const unsigned char *c2 = (const unsigned char *)s2;
  unsigned char ch; int d = 0;
  while (1) {
    d = tolower(ch = *c1++) - tolower(*c2++);
    if (d || !ch) break;
  }
  return d;
}

char *strcat(char *dst, const char *src)
{
  strcpy(strchr(dst, '\0'), src);
  return dst;
}

char *strchr(const char *s, int c)
{
  while (*s != (char)c) {
    if (!*s) return NULL;
    s++;
  }
  return (char *)s;
}

int strcmp(const char *s1, const char *s2)
{
  const unsigned char *c1 = (const unsigned char *)s1;
  const unsigned char *c2 = (const unsigned char *)s2;
  unsigned char ch; int d = 0;
  while (1) {
    d = (int)(ch = *c1++) - (int)*c2++;
    if (d || !ch) break;
  }
  return d;
}

char *strcpy(char *dst, const char *src)
{
  char *q = dst, ch; const char *p = src;
  do { *q++ = ch = *p++; } while (ch);
  return dst;
}

size_t strcspn(const char *s1, const char *s2)
{
  const char *s = s1, *c;
  while (*s1) {
    for (c = s2; *c; c++) {
      if (*s1 == *c) break;
    }
    if (*c) break;
    s1++;
  }
  return s1 - s;
}

char *strdup(const char *s)
{
  size_t l = strlen(s) + 1;
  char *d = malloc(l);
  if (d) _bcopy(d, s, l);
  return d;
}

size_t strlen(const char *s)
{
  const char *ss = s;
  while (*ss) ss++;
  return ss - s;
}

int strncasecmp(const char *s1, const char *s2, size_t n)
{
  const unsigned char *c1 = (const unsigned char *)s1;
  const unsigned char *c2 = (const unsigned char *)s2;
  unsigned char ch; int d = 0;
  while (n--) {
    d = toupper(ch = *c1++) - toupper(*c2++);
    if (d || !ch) break;
  }
  return d;
}

char *strncat(char *dst, const char *src, size_t n)
{
  char *q = strchr(dst, '\0');
  const char *p = src;
  char ch;
  while (n--) {
    *q++ = ch = *p++;
    if (!ch)
      return dst;
  }
  *q = '\0';
  return dst;
}

int strncmp(const char *s1, const char *s2, size_t n)
{
  const unsigned char *c1 = (const unsigned char *)s1;
  const unsigned char *c2 = (const unsigned char *)s2;
  unsigned char ch; int d = 0;
  while (n--) {
    d = (int)(ch = *c1++) - (int)*c2++;
    if (d || !ch) break;
  }
  return d;
}

char *strncpy(char *dst, const char *src, size_t n)
{
  char ch, *q = dst; const char *p = src;
  while (n) {
    n--; *q++ = ch = *p++;
    if (!ch) break;
  }
  /* The specs say strncpy() fills the entire buffer with 0 */
  _bzero(q, n);
  return dst;
}

char *strndup(const char *s, size_t n)
{
  size_t sl = strlen(s), l = (n > sl ? sl : n) + 1;
  char *d = malloc(l);
  if (!d) return NULL;
  _bcopy(d, s, l);
  d[n] = '\0';
  return d;
}

size_t strnlen(const char *s, size_t maxlen)
{
  const char *ss = s;
  while ((maxlen > 0) && *ss) { ss++; maxlen--; }
  return ss - s;
}

char *strpbrk(const char *s1, const char *s2)
{
  const char *c = s2;
  if (!*s1) return (char *)NULL;
  while (*s1) {
    for (c = s2; *c; c++) {
      if (*s1 == *c) break;
    }
    if (*c) break;
    s1++;
  }
  if (*c == '\0') s1 = NULL;
  return (char *)s1;
}

char *strrchr(const char *s, int c)
{
  const char *found = NULL;
  while (*s) {
    if (*s == (char)c) found = s;
    s++;
  }
  return (char *)found;
}

size_t strspn(const char *s1, const char *s2)
{
  const char *s = s1, *c;
  while (*s1) {
    for (c = s2; *c; c++) {
      if (*s1 == *c) break;
    }
    if (*c == '\0') break;
    s1++;
  }
  return s1 - s;
}

char *strstr(const char *haystack, const char *needle)
{
  return (char *)memmem(haystack, strlen(haystack), needle,
                        strlen(needle));
}

static char *strsep(char **stringp, const char *delim)
{
  char *s = *stringp, *e;
  if (!s) return NULL;
  e = strpbrk(s, delim);
  if (e) *e++ = '\0';
  *stringp = e;
  return s;
}

char *strtok_r(char *s, const char *delim, char **holder)
{
  if (s) *holder = s;
  do s = strsep(holder, delim); while (s && !*s);
  return s;
}

static char *strtok_holder;

char *strtok(char *s, const char *delim)
{
  char *holder = s ? s : strtok_holder; 
  do s = strsep(&holder, delim); while (s && !*s);
  strtok_holder = holder;
  return s;
}

int strcoll(const char *s1, const char *s2)
{
  /* fake */
  return strcmp(s1, s2);
}

size_t strxfrm(char *dest, const char *src, size_t n)
{
  /* fake */
  strncpy(dest, src, n);
  return strlen(src);
}

const char *strerror(int errno)
{
  if (errno < 0 || errno > ERRNO_MAXERR) return "?unknown error";
  return _emsg[errno];
}
#else
/*
 * lib/string.c — MARV-ABI C library adapter for `string.mvh`
 */

#include "marv.h"
#include <stdlib.h>
#include <string.h>

extern marv_i32 ctype_tolower(marv_i32 c);

marv_ptr string_memcpy(marv_ptr dst, marv_ptr src, marv_i32 n) {
  return (marv_ptr)memcpy(dst, src, (size_t)(uint32_t)n);
}

marv_ptr string_memmove(marv_ptr dst, marv_ptr src, marv_i32 n) {
  return (marv_ptr)memmove(dst, src, (size_t)(uint32_t)n);
}

marv_ptr string_strcpy(marv_ptr dst, marv_ptr src) {
  return (marv_ptr)strcpy((char *)dst, (const char *)src);
}

marv_ptr string_strncpy(marv_ptr dst, marv_ptr src, marv_i32 n) {
  return (marv_ptr)strncpy((char *)dst, (const char *)src, (size_t)(uint32_t)n);
}

marv_ptr string_strcat(marv_ptr dst, marv_ptr src) {
  return (marv_ptr)strcat((char *)dst, (const char *)src);
}

marv_ptr string_strncat(marv_ptr dst, marv_ptr src, marv_i32 n) {
  return (marv_ptr)strncat((char *)dst, (const char *)src, (size_t)(uint32_t)n);
}

marv_i32 string_memcmp(marv_ptr s1, marv_ptr s2, marv_i32 n) {
  const unsigned char *c1 = (const unsigned char *)s1;
  const unsigned char *c2 = (const unsigned char *)s2;
  size_t len = (size_t)(uint32_t)n;
  while (len--) {
    int d = (int)*c1++ - (int)*c2++;
    if (d != 0) return (marv_i32)d;
  }
  return 0;
}

marv_i32 string_strcmp(marv_ptr s1, marv_ptr s2) {
  const unsigned char *c1 = (const unsigned char *)s1;
  const unsigned char *c2 = (const unsigned char *)s2;
  while (*c1 && (*c1 == *c2)) {
    c1++; c2++;
  }
  return (marv_i32)((int)*c1 - (int)*c2);
}

marv_i32 string_strcoll(marv_ptr s1, marv_ptr s2) {
  return string_strcmp(s1, s2);
}

marv_i32 string_strncmp(marv_ptr s1, marv_ptr s2, marv_i32 n) {
  const unsigned char *c1 = (const unsigned char *)s1;
  const unsigned char *c2 = (const unsigned char *)s2;
  size_t len = (size_t)(uint32_t)n;
  if (len == 0) return 0;
  while (--len && *c1 && (*c1 == *c2)) {
    c1++; c2++;
  }
  return (marv_i32)((int)*c1 - (int)*c2);
}

marv_i32 string_strxfrm(marv_ptr dst, marv_ptr src, marv_i32 n) {
  return (marv_i32)strxfrm((char *)dst, (const char *)src, (size_t)(uint32_t)n);
}

marv_ptr string_memchr(marv_ptr s, marv_i32 c, marv_i32 n) {
  return (marv_ptr)memchr(s, (int)c, (size_t)(uint32_t)n);
}

marv_ptr string_strchr(marv_ptr s, marv_i32 c) {
  return (marv_ptr)strchr((const char *)s, (int)c);
}

marv_i32 string_strcspn(marv_ptr s, marv_ptr cs) {
  return (marv_i32)strcspn((const char *)s, (const char *)cs);
}

marv_ptr string_strpbrk(marv_ptr s, marv_ptr cs) {
  return (marv_ptr)strpbrk((const char *)s, (const char *)cs);
}

marv_ptr string_strrchr(marv_ptr s, marv_i32 c) {
  return (marv_ptr)strrchr((const char *)s, (int)c);
}

marv_i32 string_strspn(marv_ptr s, marv_ptr cs) {
  return (marv_i32)strspn((const char *)s, (const char *)cs);
}

marv_ptr string_strstr(marv_ptr s, marv_ptr ss) {
  return (marv_ptr)strstr((const char *)s, (const char *)ss);
}

marv_ptr string_strtok_r(marv_ptr s, marv_ptr sep, marv_ptr lastp) {
  char *str = (char *)s;
  const char *delim = (const char *)sep;
  if (!str && lastp) str = (char *)marv_ptr_load(lastp, 0);
  if (!str) return MARV_NULL;
  str += strspn(str, delim);
  if (*str == '\0') {
    if (lastp) marv_ptr_store(lastp, 0, MARV_NULL);
    return MARV_NULL;
  }
  char *tok = str;
  str = strpbrk(tok, delim);
  if (str) {
    *str++ = '\0';
    if (lastp) marv_ptr_store(lastp, 0, (marv_ptr)str);
  } else {
    if (lastp) marv_ptr_store(lastp, 0, MARV_NULL);
  }
  return (marv_ptr)tok;
}

static marv_ptr marv_strtok_last = MARV_NULL;
marv_ptr string_strtok(marv_ptr s, marv_ptr sep) {
  return string_strtok_r(s, sep, (marv_ptr)&marv_strtok_last);
}

marv_ptr string_memset(marv_ptr dst, marv_i32 c, marv_i32 n) {
  return (marv_ptr)memset(dst, (int)c, (size_t)(uint32_t)n);
}

marv_i32 string_strlen(marv_ptr s) {
  return (marv_i32)strlen((const char *)s);
}

marv_ptr string_strerror(marv_i32 errnum) {
  return (marv_ptr)strerror((int)errnum);
}

marv_ptr string_strdup(marv_ptr s) {
  const char *src = (const char *)s;
  size_t len = strlen(src) + 1;
  char *dst = (char *)malloc(len);
  if (dst) memcpy(dst, src, len);
  return (marv_ptr)dst;
}

marv_ptr string_strndup(marv_ptr s, marv_i32 n) {
  const char *src = (const char *)s;
  size_t maxlen = (size_t)(uint32_t)n;
  size_t len = 0;
  while (len < maxlen && src[len]) len++;
  char *dst = (char *)malloc(len + 1);
  if (dst) {
    memcpy(dst, src, len);
    dst[len] = '\0';
  }
  return (marv_ptr)dst;
}

marv_ptr string_memccpy(marv_ptr dst, marv_ptr src, marv_i32 c, marv_i32 n) {
  char *q = (char *)dst;
  const char *p = (const char *)src;
  size_t len = (size_t)(uint32_t)n;
  while (len--) {
    char ch = *p++;
    *q++ = ch;
    if (ch == (char)c) return (marv_ptr)q;
  }
  return MARV_NULL;
}

marv_ptr string_memmem(marv_ptr haystack, marv_i32 n, marv_ptr needle, marv_i32 m) {
  const unsigned char *y = (const unsigned char *)haystack;
  const unsigned char *x = (const unsigned char *)needle;
  size_t yn = (size_t)(uint32_t)n;
  size_t xm = (size_t)(uint32_t)m;
  if (xm > yn || xm == 0 || yn == 0) return MARV_NULL;
  for (size_t j = 0; j <= yn - xm; j++) {
    if (memcmp(y + j, x, xm) == 0) return (marv_ptr)(y + j);
  }
  return MARV_NULL;
}

marv_i32 string_strnlen(marv_ptr s, marv_i32 maxlen) {
  const char *p = (const char *)s;
  size_t max = (size_t)(uint32_t)maxlen;
  size_t len = 0;
  while (len < max && p[len]) len++;
  return (marv_i32)len;
}

void string_memswap(marv_ptr s1, marv_ptr s2, marv_i32 n) {
  char *p = (char *)s1;
  char *q = (char *)s2;
  size_t len = (size_t)(uint32_t)n;
  while (len--) {
    char tmp = *p;
    *p++ = *q;
    *q++ = tmp;
  }
}

void string_bzero(marv_ptr s, marv_i32 n) {
  memset(s, 0, (size_t)(uint32_t)n);
}

marv_i32 string_strcasecmp(marv_ptr s1, marv_ptr s2) {
  const unsigned char *c1 = (const unsigned char *)s1;
  const unsigned char *c2 = (const unsigned char *)s2;
  for (;;) {
    unsigned char u1 = *c1++;
    unsigned char u2 = *c2++;
    marv_i32 d = ctype_tolower((marv_i32)u1) - ctype_tolower((marv_i32)u2);
    if (d != 0 || u1 == 0) return d;
  }
}

marv_i32 string_strncasecmp(marv_ptr s1, marv_ptr s2, marv_i32 n) {
  const unsigned char *c1 = (const unsigned char *)s1;
  const unsigned char *c2 = (const unsigned char *)s2;
  size_t len = (size_t)(uint32_t)n;
  while (len--) {
    unsigned char u1 = *c1++;
    unsigned char u2 = *c2++;
    marv_i32 d = ctype_tolower((marv_i32)u1) - ctype_tolower((marv_i32)u2);
    if (d != 0 || u1 == 0) return d;
  }
  return 0;
}

#endif
