/* Single-source MARV-ABI adapter for `stat` (WCPL + C99 + RV64G) */
#if defined(__WCPL__)
#pragma module "stat"
#include <wasi/api.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

static_assert(S_ISBLK(S_IFBLK));
static_assert(S_ISCHR(S_IFCHR));
static_assert(S_ISDIR(S_IFDIR));
static_assert(S_ISFIFO(S_IFIFO));
static_assert(S_ISLNK(S_IFLNK));
static_assert(S_ISREG(S_IFREG));
static_assert(S_ISSOCK(S_IFSOCK));

static void timestamp_to_timespec(timestamp_t timestamp, time_t *pt, long *pns) 
{
  /* timestamp is in nanoseconds */
  *pt = (time_t)(timestamp / 1000000000);
  *pns = (long)(timestamp % 1000000000);
}

static void to_public_stat(const filestat_t *in, struct stat *out) 
{
  out->st_dev = in->dev;
  out->st_ino = in->ino;
  out->st_nlink = in->nlink;
  static_assert(sizeof(off_t) == sizeof(filesize_t));
  out->st_size = (off_t)in->size;
  timestamp_to_timespec(in->atim, &out->st_atime, &out->st_atime_ns);
  timestamp_to_timespec(in->mtim, &out->st_mtime, &out->st_mtime_ns);
  timestamp_to_timespec(in->ctim, &out->st_ctime, &out->st_ctime_ns);
  switch (in->filetype) {
    case FILETYPE_BLOCK_DEVICE:
      out->st_mode |= S_IFBLK;
      break;
    case FILETYPE_CHARACTER_DEVICE:
      out->st_mode |= S_IFCHR;
      break;
    case FILETYPE_DIRECTORY:
      out->st_mode |= S_IFDIR;
      break;
    case FILETYPE_REGULAR_FILE:
      out->st_mode |= S_IFREG;
      break;
    case FILETYPE_SOCKET_DGRAM:
    case FILETYPE_SOCKET_STREAM:
      out->st_mode |= S_IFSOCK;
      break;
    case FILETYPE_SYMBOLIC_LINK:
      out->st_mode |= S_IFLNK;
      break;
  }
}

int fstat(int fd, struct stat *ps) 
{
  filestat_t fs;
  errno_t error = fd_filestat_get(fd, &fs);
  if (error != 0) {
    errno = (int)error;
    return -1;
  }
  to_public_stat(&fs, ps);
  return 0;
}

int fstatat(int fd, const char *path, struct stat *ps, int atflag) 
{
  lookupflags_t lflags = 0;
  if ((atflag & AT_SYMLINK_NOFOLLOW) == 0)
    lflags |= LOOKUPFLAGS_SYMLINK_FOLLOW;
  filestat_t fs;
  errno_t error = path_filestat_get(fd, lflags, path, strlen(path), &fs);
  if (error != 0) {
    errno = (int)error;
    return -1;
  }
  to_public_stat(&fs, ps);
  return 0;
}

int stat(const char *path, struct stat *ps) 
{
  char *relpath;
  int dirfd = find_relpath(path, &relpath);
  if (dirfd == -1) {
    errno = ENOTCAPABLE;
    return -1;
  }
  return fstatat(dirfd, relpath, ps, 0);
}

int lstat(const char *path, struct stat *ps) 
{
  char *relpath;
  int dirfd = find_relpath(path, &relpath);
  if (dirfd == -1) {
    errno = ENOTCAPABLE;
    return -1;
  }
  return fstatat(dirfd, relpath, ps, AT_SYMLINK_NOFOLLOW);
}

int mkdirat(int fd, const char *path, mode_t mode) 
{
  errno_t error = path_create_directory(fd, path, strlen(path));
  if (error != 0) {
    errno = (int)error;
    return -1;
  }
  return 0;
}

int mkdir(const char *path, mode_t mode) 
{
  char *relpath;
  int dirfd = find_relpath(path, &relpath);
  if (dirfd == -1) {
    errno = ENOTCAPABLE;
    return -1;
  }
  return mkdirat(dirfd, relpath, mode);
}

#else
/*
 * lib/sys/stat.c — MARV-ABI C library adapter for `sys.stat.mvh`
 *
 * A module described by `sys.stat.mvh` (`(module "stat")`) has its
 * corresponding C implementation in `lib/sys/stat.c`.
 * Adapts host OS `<sys/stat.h>` to MARV's 80-byte WCPL `struct stat` layout.
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
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#if !defined(_WIN32)
#  include <fcntl.h>
#  include <unistd.h>
#endif

extern void errno_sync_from_host(int herr);

/* WCPL / WASI mode_t file type flags */
#define MARV_S_IFBLK  0x6000
#define MARV_S_IFCHR  0x2000
#define MARV_S_IFDIR  0x4000
#define MARV_S_IFLNK  0xa000
#define MARV_S_IFREG  0x8000
#define MARV_S_IFSOCK 0xc000
#define MARV_S_IFIFO  0xc000

/* 80-byte WCPL struct stat layout */
typedef struct marv_stat {
  uint64_t st_dev;      /* 0  */
  uint64_t st_ino;      /* 8  */
  uint64_t st_nlink;    /* 16 */
  uint32_t st_mode;     /* 24 */
  uint32_t _pad0;       /* 28 */
  int64_t  st_size;     /* 32 */
  int64_t  st_atime_s;  /* 40 */
  int64_t  st_mtime_s;  /* 48 */
  int64_t  st_ctime_s;  /* 56 */
  int32_t  st_atime_ns; /* 64 */
  int32_t  st_mtime_ns; /* 68 */
  int32_t  st_ctime_ns; /* 72 */
  int32_t  _pad1;       /* 76 */
} marv_stat_t;

static void host_stat_to_marv(const struct stat *hs, marv_ptr ps) {
  if (!ps) return;
  marv_stat_t ms;
  memset(&ms, 0, sizeof(ms));
  ms.st_dev   = (uint64_t)hs->st_dev;
  ms.st_ino   = (uint64_t)hs->st_ino;
  ms.st_nlink = (uint64_t)hs->st_nlink;

  uint32_t mode = (uint32_t)(hs->st_mode & 07777);
#if defined(S_ISDIR)
  if (S_ISDIR(hs->st_mode))  mode |= MARV_S_IFDIR;
  else if (S_ISREG(hs->st_mode))  mode |= MARV_S_IFREG;
  else if (S_ISLNK(hs->st_mode))  mode |= MARV_S_IFLNK;
  else if (S_ISCHR(hs->st_mode))  mode |= MARV_S_IFCHR;
  else if (S_ISBLK(hs->st_mode))  mode |= MARV_S_IFBLK;
  else if (S_ISFIFO(hs->st_mode)) mode |= MARV_S_IFIFO;
#ifdef S_ISSOCK
  else if (S_ISSOCK(hs->st_mode)) mode |= MARV_S_IFSOCK;
#endif
#endif
  ms.st_mode    = mode;
  ms.st_size    = (int64_t)hs->st_size;
  ms.st_atime_s = (int64_t)hs->st_atime;
  ms.st_mtime_s = (int64_t)hs->st_mtime;
  ms.st_ctime_s = (int64_t)hs->st_ctime;
  memcpy(ps, &ms, sizeof(ms));
}

marv_i32 stat_stat(marv_ptr path, marv_ptr ps) {
  struct stat hs;
  errno = 0;
  int r = stat((const char *)path, &hs);
  if (r != 0) {
    errno_sync_from_host(errno);
    return -1;
  }
  host_stat_to_marv(&hs, ps);
  return 0;
}

marv_i32 stat_lstat(marv_ptr path, marv_ptr ps) {
  struct stat hs;
  errno = 0;
#if defined(_WIN32)
  int r = stat((const char *)path, &hs);
#else
  int r = lstat((const char *)path, &hs);
#endif
  if (r != 0) {
    errno_sync_from_host(errno);
    return -1;
  }
  host_stat_to_marv(&hs, ps);
  return 0;
}

marv_i32 stat_fstat(marv_i32 fd, marv_ptr ps) {
  struct stat hs;
  errno = 0;
  int r = fstat((int)fd, &hs);
  if (r != 0) {
    errno_sync_from_host(errno);
    return -1;
  }
  host_stat_to_marv(&hs, ps);
  return 0;
}

marv_i32 stat_fstatat(marv_i32 fd, marv_ptr path, marv_ptr ps, marv_i32 atflag) {
  (void)fd;
  if (atflag & 1) return stat_lstat(path, ps);
  return stat_stat(path, ps);
}

marv_i32 stat_mkdir(marv_ptr path, marv_i32 mode) {
  errno = 0;
#if defined(_WIN32)
  (void)mode;
  int r = _mkdir((const char *)path);
#else
  int r = mkdir((const char *)path, (mode_t)mode);
#endif
  if (r != 0) errno_sync_from_host(errno);
  return (marv_i32)r;
}

marv_i32 stat_mkdirat(marv_i32 fd, marv_ptr path, marv_i32 mode) {
  (void)fd;
  return stat_mkdir(path, mode);
}

#endif
