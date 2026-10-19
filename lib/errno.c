/* Single-source MARV-ABI adapter for `errno` (WCPL + C99 + RV64G) */
#if defined(__WCPL__)
#pragma module "errno"
/* Errors */

#include <errno.h>
#include <wasi/api.h>

int errno;

/* used by perror */
const char *_emsg[ERRNO_MAXERR+1] = {
  "No error occurred",
  "Argument list too long",
  "Permission denied",
  "Address in use",
  "Address not available",
  "Address family not supported",
  "Resource unavailable, or operation would block",
  "Connection already in progress",
  "Bad file descriptor",
  "Bad message",
  "Device or resource busy",
  "Operation canceled",
  "No child processes",
  "Connection aborted",
  "Connection refused",
  "Connection reset",
  "Resource deadlock would occur",
  "Destination address required",
  "Mathematics argument out of domain of function",
  "Reserved",
  "File exists",
  "Bad address",
  "File too large",
  "Host is unreachable",
  "Identifier removed",
  "Illegal byte sequence",
  "Operation in progress",
  "Interrupted function",
  "Invalid argument",
  "I/O error",
  "Socket is connected",
  "Is a directory",
  "Too many levels of symbolic links",
  "File descriptor value too large",
  "Too many links",
  "Message too large",
  "Reserved",
  "Filename too long",
  "Network is down",
  "Connection aborted by network",
  "Network unreachable",
  "Too many files open in system",
  "No buffer space available",
  "No such device",
  "No such file or directory",
  "Executable file format error",
  "No locks available",
  "Reserved",
  "Not enough space",
  "No message of the desired type",
  "Protocol not available",
  "No space left on device",
  "Function not supported",
  "The socket is not connected",
  "Not a directory or a symbolic link to a directory",
  "Directory not empty",
  "State not recoverable",
  "Not a socket",
  "Not supported, or operation not supported on socket",
  "Inappropriate I/O control operation",
  "No such device or address",
  "Value too large to be stored in data type",
  "Previous owner died",
  "Operation not permitted",
  "Broken pipe",
  "Protocol error",
  "Protocol not supported",
  "Protocol wrong type for socket",
  "Result too large",
  "Read-only file system",
  "Invalid seek",
  "No such process",
  "Reserved",
  "Connection timed out",
  "Text file busy",
  "Cross-device link",
  "Extension: Capabilities insufficient"
};

static_assert(ESUCCESS == ERRNO_SUCCESS);
static_assert(E2BIG == ERRNO_2BIG);
static_assert(EACCES == ERRNO_ACCES);
static_assert(EADDRINUSE == ERRNO_ADDRINUSE);
static_assert(EADDRNOTAVAIL == ERRNO_ADDRNOTAVAIL);
static_assert(EAFNOSUPPORT == ERRNO_AFNOSUPPORT);
static_assert(EAGAIN == ERRNO_AGAIN);
static_assert(EALREADY == ERRNO_ALREADY);
static_assert(EBADF == ERRNO_BADF);
static_assert(EBADMSG == ERRNO_BADMSG);
static_assert(EBUSY == ERRNO_BUSY);
static_assert(ECANCELED == ERRNO_CANCELED);
static_assert(ECHILD == ERRNO_CHILD);
static_assert(ECONNABORTED == ERRNO_CONNABORTED);
static_assert(ECONNREFUSED == ERRNO_CONNREFUSED);
static_assert(ECONNRESET == ERRNO_CONNRESET);
static_assert(EDEADLK == ERRNO_DEADLK);
static_assert(EDESTADDRREQ == ERRNO_DESTADDRREQ);
static_assert(EDOM == ERRNO_DOM);
static_assert(EDQUOT == ERRNO_DQUOT);
static_assert(EEXIST == ERRNO_EXIST);
static_assert(EFAULT == ERRNO_FAULT);
static_assert(EFBIG == ERRNO_FBIG);
static_assert(EHOSTUNREACH == ERRNO_HOSTUNREACH);
static_assert(EIDRM == ERRNO_IDRM);
static_assert(EILSEQ == ERRNO_ILSEQ);
static_assert(EINPROGRESS == ERRNO_INPROGRESS);
static_assert(EINTR == ERRNO_INTR);
static_assert(EINVAL == ERRNO_INVAL);
static_assert(EIO == ERRNO_IO);
static_assert(EISCONN == ERRNO_ISCONN);
static_assert(EISDIR == ERRNO_ISDIR);
static_assert(ELOOP == ERRNO_LOOP);
static_assert(EMFILE == ERRNO_MFILE);
static_assert(EMLINK == ERRNO_MLINK);
static_assert(EMSGSIZE == ERRNO_MSGSIZE);
static_assert(EMULTIHOP == ERRNO_MULTIHOP);
static_assert(ENAMETOOLONG == ERRNO_NAMETOOLONG);
static_assert(ENETDOWN == ERRNO_NETDOWN);
static_assert(ENETRESET == ERRNO_NETRESET);
static_assert(ENETUNREACH == ERRNO_NETUNREACH);
static_assert(ENFILE == ERRNO_NFILE);
static_assert(ENOBUFS == ERRNO_NOBUFS);
static_assert(ENODEV == ERRNO_NODEV);
static_assert(ENOENT == ERRNO_NOENT);
static_assert(ENOEXEC == ERRNO_NOEXEC);
static_assert(ENOLCK == ERRNO_NOLCK);
static_assert(ENOLINK == ERRNO_NOLINK);
static_assert(ENOMEM == ERRNO_NOMEM);
static_assert(ENOMSG == ERRNO_NOMSG);
static_assert(ENOPROTOOPT == ERRNO_NOPROTOOPT);
static_assert(ENOSPC == ERRNO_NOSPC);
static_assert(ENOSYS == ERRNO_NOSYS);
static_assert(ENOTCONN == ERRNO_NOTCONN);
static_assert(ENOTDIR == ERRNO_NOTDIR);
static_assert(ENOTEMPTY == ERRNO_NOTEMPTY);
static_assert(ENOTRECOVERABLE == ERRNO_NOTRECOVERABLE);
static_assert(ENOTSOCK == ERRNO_NOTSOCK);
static_assert(ENOTSUP == ERRNO_NOTSUP);
static_assert(ENOTTY == ERRNO_NOTTY);
static_assert(ENXIO == ERRNO_NXIO);
static_assert(EOVERFLOW == ERRNO_OVERFLOW);
static_assert(EOWNERDEAD == ERRNO_OWNERDEAD);
static_assert(EPERM == ERRNO_PERM);
static_assert(EPIPE == ERRNO_PIPE);
static_assert(EPROTO == ERRNO_PROTO);
static_assert(EPROTONOSUPPORT == ERRNO_PROTONOSUPPORT);
static_assert(EPROTOTYPE == ERRNO_PROTOTYPE);
static_assert(ERANGE == ERRNO_RANGE);
static_assert(EROFS == ERRNO_ROFS);
static_assert(ESPIPE == ERRNO_SPIPE);
static_assert(ESRCH == ERRNO_SRCH);
static_assert(ESTALE == ERRNO_STALE);
static_assert(ETIMEDOUT == ERRNO_TIMEDOUT);
static_assert(ETXTBSY == ERRNO_TXTBSY);
static_assert(EXDEV == ERRNO_XDEV);
static_assert(ENOTCAPABLE == ERRNO_NOTCAPABLE);

#else
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

/* MARV / WCPL / WASI errno constants */
#define MARV_ESUCCESS    0
#define MARV_E2BIG       1
#define MARV_EACCES      2
#define MARV_EAGAIN      6
#define MARV_EBADF       8
#define MARV_EBUSY       10
#define MARV_ECHILD      12
#define MARV_EDEADLK     16
#define MARV_EDOM        18
#define MARV_EEXIST      20
#define MARV_EFAULT      21
#define MARV_EFBIG       22
#define MARV_EILSEQ      25
#define MARV_EINTR       27
#define MARV_EINVAL      28
#define MARV_EIO         29
#define MARV_EISDIR      31
#define MARV_ELOOP       32
#define MARV_EMFILE      33
#define MARV_EMLINK      34
#define MARV_ENAMETOOLONG 37
#define MARV_ENFILE      41
#define MARV_ENODEV      43
#define MARV_ENOENT      44
#define MARV_ENOEXEC     45
#define MARV_ENOLCK      46
#define MARV_ENOMEM      48
#define MARV_ENOSPC      51
#define MARV_ENOSYS      52
#define MARV_ENOTDIR     54
#define MARV_ENOTEMPTY   55
#define MARV_ENOTTY      59
#define MARV_ENXIO       60
#define MARV_EOVERFLOW   61
#define MARV_EPERM       63
#define MARV_EPIPE       64
#define MARV_ERANGE      68
#define MARV_EROFS       69
#define MARV_ESPIPE      70
#define MARV_ESRCH       71
#define MARV_EXDEV       75

/* Public MARV global `$errno:errno` */
marv_i32 errno_errno = 0;

marv_i32 errno_from_host(int herr) {
  if (herr == 0) return MARV_ESUCCESS;
  switch (herr) {
#ifdef E2BIG
    case E2BIG: return MARV_E2BIG;
#endif
#ifdef EACCES
    case EACCES: return MARV_EACCES;
#endif
#ifdef EAGAIN
    case EAGAIN: return MARV_EAGAIN;
#endif
#ifdef EBADF
    case EBADF: return MARV_EBADF;
#endif
#ifdef EBUSY
    case EBUSY: return MARV_EBUSY;
#endif
#ifdef ECHILD
    case ECHILD: return MARV_ECHILD;
#endif
#ifdef EDEADLK
    case EDEADLK: return MARV_EDEADLK;
#endif
#ifdef EDOM
    case EDOM: return MARV_EDOM;
#endif
#ifdef EEXIST
    case EEXIST: return MARV_EEXIST;
#endif
#ifdef EFAULT
    case EFAULT: return MARV_EFAULT;
#endif
#ifdef EFBIG
    case EFBIG: return MARV_EFBIG;
#endif
#ifdef EILSEQ
    case EILSEQ: return MARV_EILSEQ;
#endif
#ifdef EINTR
    case EINTR: return MARV_EINTR;
#endif
#ifdef EINVAL
    case EINVAL: return MARV_EINVAL;
#endif
#ifdef EIO
    case EIO: return MARV_EIO;
#endif
#ifdef EISDIR
    case EISDIR: return MARV_EISDIR;
#endif
#ifdef EMFILE
    case EMFILE: return MARV_EMFILE;
#endif
#ifdef EMLINK
    case EMLINK: return MARV_EMLINK;
#endif
#ifdef ENAMETOOLONG
    case ENAMETOOLONG: return MARV_ENAMETOOLONG;
#endif
#ifdef ENFILE
    case ENFILE: return MARV_ENFILE;
#endif
#ifdef ENODEV
    case ENODEV: return MARV_ENODEV;
#endif
#ifdef ENOENT
    case ENOENT: return MARV_ENOENT;
#endif
#ifdef ENOEXEC
    case ENOEXEC: return MARV_ENOEXEC;
#endif
#ifdef ENOLCK
    case ENOLCK: return MARV_ENOLCK;
#endif
#ifdef ENOMEM
    case ENOMEM: return MARV_ENOMEM;
#endif
#ifdef ENOSPC
    case ENOSPC: return MARV_ENOSPC;
#endif
#ifdef ENOSYS
    case ENOSYS: return MARV_ENOSYS;
#endif
#ifdef ENOTDIR
    case ENOTDIR: return MARV_ENOTDIR;
#endif
#ifdef ENOTEMPTY
    case ENOTEMPTY: return MARV_ENOTEMPTY;
#endif
#ifdef ENOTTY
    case ENOTTY: return MARV_ENOTTY;
#endif
#ifdef ENXIO
    case ENXIO: return MARV_ENXIO;
#endif
#ifdef EOVERFLOW
    case EOVERFLOW: return MARV_EOVERFLOW;
#endif
#ifdef EPERM
    case EPERM: return MARV_EPERM;
#endif
#ifdef EPIPE
    case EPIPE: return MARV_EPIPE;
#endif
#ifdef ERANGE
    case ERANGE: return MARV_ERANGE;
#endif
#ifdef EROFS
    case EROFS: return MARV_EROFS;
#endif
#ifdef ESPIPE
    case ESPIPE: return MARV_ESPIPE;
#endif
#ifdef ESRCH
    case ESRCH: return MARV_ESRCH;
#endif
#ifdef EXDEV
    case EXDEV: return MARV_EXDEV;
#endif
    default: return MARV_EIO;
  }
}

void errno_sync_from_host(int herr) {
  if (herr != 0) {
    errno_errno = errno_from_host(herr);
  }
}
#endif

