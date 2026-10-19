/* Single-source MARV-ABI adapter for `fcntl` (WCPL + C99 + RV64G) */
#if defined(__WCPL__)
#pragma module "fcntl"
#include <wasi/api.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>

static_assert(O_APPEND == FDFLAGS_APPEND);
static_assert(O_DSYNC == FDFLAGS_DSYNC);
static_assert(O_NONBLOCK == FDFLAGS_NONBLOCK);
static_assert(O_RSYNC == FDFLAGS_RSYNC);
static_assert(O_SYNC == FDFLAGS_SYNC);
static_assert(O_CREAT == OFLAGS_CREAT << 12);
static_assert(O_DIRECTORY == OFLAGS_DIRECTORY << 12);
static_assert(O_EXCL == OFLAGS_EXCL << 12);
static_assert(O_TRUNC == OFLAGS_TRUNC << 12);

static_assert(POSIX_FADV_DONTNEED == ADVICE_DONTNEED);
static_assert(POSIX_FADV_NOREUSE == ADVICE_NOREUSE);
static_assert(POSIX_FADV_NORMAL == ADVICE_NORMAL);
static_assert(POSIX_FADV_RANDOM == ADVICE_RANDOM);
static_assert(POSIX_FADV_SEQUENTIAL == ADVICE_SEQUENTIAL);
static_assert(POSIX_FADV_WILLNEED == ADVICE_WILLNEED);


typedef struct preopen {
  const char *prefix;
  fd_t fd;
} preopen_t;

static preopen_t *preopens;
static size_t num_preopens;
static size_t preopen_capacity;
static int preopens_populated;
static int preopen_sep = '/';

static int resize_preopens(void) 
{
  size_t start_capacity = 4;
  size_t old_capacity = preopen_capacity;
  size_t new_capacity = old_capacity == 0 ? start_capacity : old_capacity * 2;

  preopen_t *old_preopens = preopens;
  preopen_t *new_preopens = calloc(sizeof(preopen_t), new_capacity);
  if (new_preopens == NULL) return -1;

  memcpy(new_preopens, old_preopens, num_preopens * sizeof(preopen_t));
  preopens = new_preopens;
  preopen_capacity = new_capacity;
  free(old_preopens);

  return 0;
}

static const char *strip_prefixes(const char *path) 
{
  while (1) {
    if (path[0] == preopen_sep) {
      path++;
    } else if (path[0] == '.' && path[1] == preopen_sep) {
      path += 2;
    } else if (path[0] == '.' && path[1] == 0) {
      path++;
    } else {
      break;
    }
  }
  return path;
}

static int register_preopened_fd(fd_t fd, const char *relprefix) 
{
  if (num_preopens == preopen_capacity && resize_preopens() != 0) return -1;
  char *prefix = strdup(strip_prefixes(relprefix));
  if (prefix == NULL) return -1;
  preopen_t *pp = &preopens[num_preopens++];
  pp->prefix = prefix;
  pp->fd = fd;
  return 0;
}

static int populate_preopens(void)
{
  fd_t fd;
  if (preopens_populated) return 0;
  preopens_populated = true;
  for (fd = 3; fd != 0; ++fd) {
    prestat_t prestat;
    errno_t ret = fd_prestat_get(fd, &prestat);
    if (ret == ERRNO_BADF) break;
    if (ret != ERRNO_SUCCESS) return 1;
    if (prestat.tag == PREOPENTYPE_DIR) {
      char *prefix = malloc(prestat.u.dir.pr_name_len + 1);
      if (!prefix) return 2;
      ret = fd_prestat_dir_name(fd, (uint8_t *)prefix, prestat.u.dir.pr_name_len);
      if (ret != ERRNO_SUCCESS) return 3;
      prefix[prestat.u.dir.pr_name_len] = '\0';
      if (prestat.u.dir.pr_name_len > 0 && prefix[prestat.u.dir.pr_name_len-1] == '\\') {
        preopen_sep = '\\'; /* windows path conventions */
      }
      register_preopened_fd(fd, prefix);
      free(prefix);                                       
    }
  }
  return 0;
}

static bool prefix_matches(const char *prefix, size_t prefix_len, const char *path) 
{
  if (path[0] != preopen_sep && prefix_len == 0) return true;
  if (memcmp(path, prefix, prefix_len) != 0) return false;
  size_t i = prefix_len;
  while (i > 0 && prefix[i-1] == preopen_sep) --i;
  char last = path[i];
  return last == preopen_sep || last == '\0';
}

static int find_abspath(const char *path, const char **abs_prefix, const char **relative_path) 
{
  while (*path == preopen_sep) ++path;
  size_t match_len = 0, i;
  int fd = -1;
  for (i = num_preopens; i > 0; --i) {
    const preopen_t *pre = &preopens[i-1];
    const char *prefix = pre->prefix;
    size_t len = strlen(prefix);
    if ((fd == -1 || len > match_len) && prefix_matches(prefix, len, path)) {
      fd = pre->fd;
      match_len = len;
      *abs_prefix = prefix;
    }
  }
  if (fd == -1) {
    errno = ENOENT;
    return -1;
  }
  const char *computed = path + match_len;
  while (*computed == preopen_sep) ++computed;
  if (*computed == '\0') computed = ".";
  *relative_path = computed;
  return fd;
}

int find_relpath(const char *path, char **relative_path) 
{
  const char *abs_prefix; 
  if (!preopens_populated) populate_preopens(); // todo: check ret value!
  return find_abspath(path, &abs_prefix, (const char**)relative_path);
}

int openat(int fd, const char *path, int oflags) 
{
  rights_t max =
      ~(RIGHTS_FD_DATASYNC | RIGHTS_FD_READ |
        RIGHTS_FD_WRITE | RIGHTS_FD_ALLOCATE |
        RIGHTS_FD_READDIR | RIGHTS_FD_FILESTAT_SET_SIZE);
  switch (oflags & O_ACCMODE) {
    case O_RDONLY:
    case O_RDWR:
    case O_WRONLY:
      if ((oflags & O_RDONLY) != 0) {
        max |= RIGHTS_FD_READ | RIGHTS_FD_READDIR;
      }
      if ((oflags & O_WRONLY) != 0) {
        max |= RIGHTS_FD_DATASYNC | RIGHTS_FD_WRITE |
               RIGHTS_FD_ALLOCATE |
               RIGHTS_FD_FILESTAT_SET_SIZE;
      }
      break;
    case O_EXEC:
      break;
    case O_SEARCH:
      break;
    default:
      errno = EINVAL;
      return -1;
  }

  fdstat_t fsb_cur;
  errno_t error = fd_fdstat_get(fd, &fsb_cur);
  if (error != 0) {
    errno = (int)error;
    return -1;
  }

  lookupflags_t lookup_flags = 0;
  if ((oflags & O_NOFOLLOW) == 0)
    lookup_flags |= LOOKUPFLAGS_SYMLINK_FOLLOW;

  fdflags_t fs_flags = oflags & 0xfff;
  rights_t fs_rights_base = max & fsb_cur.fs_rights_inheriting;
  rights_t fs_rights_inheriting = fsb_cur.fs_rights_inheriting;
  fd_t newfd;
  error = path_open(fd, lookup_flags, path, strlen(path),
                    (oflags_t)((oflags >> 12) & 0xfff),
                    fs_rights_base, fs_rights_inheriting, fs_flags,
                    &newfd);
  if (error != 0) {
    errno = (int)error;
    return -1;
  }
  return newfd;
}

int open(const char *path, int oflags) 
{
  char *relpath;
  int dirfd = find_relpath(path, &relpath);
  if (dirfd == -1) {
    errno = ENOTCAPABLE;
    return -1;
  }
  return openat(dirfd, relpath, oflags);
}

int fcntl(int fd, int cmd, ...) 
{
  switch (cmd) {
    case F_GETFD:
      return FD_CLOEXEC;
    case F_SETFD:
      return 0;
    case F_GETFL: {
      fdstat_t fds;
      errno_t error = fd_fdstat_get(fd, &fds);
      if (error != 0) {
        errno = error;
        return -1;
      }
      int oflags = fds.fs_flags;
      if ((fds.fs_rights_base & (RIGHTS_FD_READ | RIGHTS_FD_READDIR)) != 0) {
        if ((fds.fs_rights_base & RIGHTS_FD_WRITE) != 0) {
          oflags |= O_RDWR;
        } else {
          oflags |= O_RDONLY;
        }
      } else if ((fds.fs_rights_base & RIGHTS_FD_WRITE) != 0) {
        oflags |= O_WRONLY;
      } else {
        oflags |= O_SEARCH;
      }
      return oflags;
    }
    case F_SETFL: {
      va_list ap;
      va_start(ap, cmd);
      int flags = va_arg(ap, int);
      va_end(ap);
      fdflags_t fs_flags = flags & 0xfff;
      errno_t error = fd_fdstat_set_flags(fd, fs_flags);
      if (error != 0) {
        errno = error;
        return -1;
      }
      return 0;
    }
  }
  errno = EINVAL;
  return -1;
}


int posix_fadvise(int fd, off_t offset, off_t len, int advice) 
{
  if (offset < 0 || len < 0)
    return EINVAL;
  return fd_advise(fd, offset, len, (advice_t)advice);
}

int posix_fallocate(int fd, off_t offset, off_t len) 
{
  if (offset < 0 || len < 0)
    return EINVAL;
  return fd_allocate(fd, offset, len);
}


#else
/*
 * lib/fcntl.c — MARV-ABI C library adapter for `fcntl.mvh`
 *
 * Translates WCPL/WASI open flags and fcntl commands to the host OS, and
 * accepts variadic arguments via `marv_ptr ap` (WCPL 16-byte `va_arg_t` model).
 */

#define _DEFAULT_SOURCE 1
#define _POSIX_C_SOURCE 200809L
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

typedef union marv_va_arg {
  marv_i32 va_int;
  marv_u32 va_uint;
  marv_i64 va_llong;
  marv_u64 va_ullong;
  marv_f64 va_double;
  uint64_t va_ptr_slot;
  uint64_t _pad[2];
} marv_va_arg_t;

#define MARV_VA_COUNT(ap) \
  (((const marv_va_arg_t *)(uintptr_t)(ap))[-1].va_int)

#define MARV_VA_PTR(slot_ptr) \
  ((marv_ptr)(uintptr_t)((slot_ptr)->va_ptr_slot))

#if defined(_MSC_VER) && !defined(__cplusplus) && (_MSC_VER < 1900)
#define inline __inline
#endif

static inline marv_ptr marv_ptr_load(marv_ptr base, marv_i64 offset) {
  uint64_t slot = 0;
  memcpy(&slot, (const uint8_t*)base + offset, 8);
  return (marv_ptr)(uintptr_t)slot;
}

static inline void marv_ptr_store(marv_ptr base, marv_i64 offset, marv_ptr val) {
  uint64_t slot = (uint64_t)(uintptr_t)val;
  memcpy((uint8_t*)base + offset, &slot, 8);
}

static inline marv_i64 marv_i64_load(marv_ptr base, marv_i64 off) {
  marv_i64 v; memcpy(&v, (const uint8_t*)base + off, 8); return v;
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
static inline void marv_f32_store(marv_ptr base, marv_i64 off, marv_f32 v) {
  memcpy((uint8_t*)base + off, &v, 4);
}
static inline void marv_f64_store(marv_ptr base, marv_i64 off, marv_f64 v) {
  memcpy((uint8_t*)base + off, &v, 8);
}
#include <errno.h>

#if !defined(_WIN32)
#  include <fcntl.h>
#  include <unistd.h>
#endif

extern void errno_sync_from_host(int herr);

/* WCPL / WASI open flag constants */
#define MARV_O_APPEND    0x00000001
#define MARV_O_NONBLOCK  0x00000004
#define MARV_O_SYNC      0x00000010
#define MARV_O_CREAT     0x00001000
#define MARV_O_DIRECTORY 0x00002000
#define MARV_O_EXCL      0x00004000
#define MARV_O_TRUNC     0x00008000
#define MARV_O_RDONLY    0x04000000
#define MARV_O_WRONLY    0x10000000
#define MARV_O_RDWR      0x14000000

#if !defined(_WIN32)
static int marv_oflags_to_host(marv_i32 mf) {
  int hf = 0;
  if ((mf & MARV_O_RDWR) == MARV_O_RDWR) hf |= O_RDWR;
  else if (mf & MARV_O_WRONLY) hf |= O_WRONLY;
  else hf |= O_RDONLY;

  if (mf & MARV_O_APPEND)   hf |= O_APPEND;
  if (mf & MARV_O_NONBLOCK) hf |= O_NONBLOCK;
  if (mf & MARV_O_CREAT)    hf |= O_CREAT;
  if (mf & MARV_O_EXCL)     hf |= O_EXCL;
  if (mf & MARV_O_TRUNC)    hf |= O_TRUNC;
#ifdef O_DIRECTORY
  if (mf & MARV_O_DIRECTORY) hf |= O_DIRECTORY;
#endif
  return hf;
}
#endif

marv_i32 fcntl_open(marv_ptr path, marv_i32 oflags) {
#if defined(_WIN32)
  (void)path; (void)oflags;
  return -1;
#else
  errno = 0;
  int fd = open((const char *)path, marv_oflags_to_host(oflags), 0666);
  if (fd < 0) errno_sync_from_host(errno);
  return (marv_i32)fd;
#endif
}

marv_i32 fcntl_openat(marv_i32 dirfd, marv_ptr path, marv_i32 oflags) {
#if defined(_WIN32)
  (void)dirfd; (void)path; (void)oflags;
  return -1;
#else
  errno = 0;
  int fd = openat((int)dirfd, (const char *)path, marv_oflags_to_host(oflags), 0666);
  if (fd < 0) errno_sync_from_host(errno);
  return (marv_i32)fd;
#endif
}

marv_i32 fcntl_fcntl(marv_i32 fd, marv_i32 cmd, marv_ptr ap) {
#if defined(_WIN32)
  (void)fd; (void)cmd; (void)ap;
  return 0;
#else
  const marv_va_arg_t *va = (const marv_va_arg_t *)(uintptr_t)ap;
  errno = 0;
  int r = -1;
  switch (cmd) {
    case 1: /* F_GETFD */
      r = fcntl((int)fd, F_GETFD);
      break;
    case 2: /* F_SETFD */
      r = fcntl((int)fd, F_SETFD, va ? (int)va[0].va_int : 0);
      break;
    case 3: /* F_GETFL */
      r = fcntl((int)fd, F_GETFL);
      break;
    case 4: /* F_SETFL */
      r = fcntl((int)fd, F_SETFL, marv_oflags_to_host(va ? va[0].va_int : 0));
      break;
    default:
      r = -1;
      break;
  }
  if (r < 0) errno_sync_from_host(errno);
  return (marv_i32)r;
#endif
}

marv_i32 fcntl_posix_fadvise(marv_i32 fd, marv_i64 offset, marv_i64 len, marv_i32 advice) {
  (void)fd; (void)offset; (void)len; (void)advice;
  return 0;
}

marv_i32 fcntl_posix_fallocate(marv_i32 fd, marv_i64 offset, marv_i64 len) {
  (void)fd; (void)offset; (void)len;
  return 0;
}

#endif
