/*
 * lib/dirent.c — MARV-ABI C library adapter for `dirent.mvh`
 *
 * Adapts host OS directory traversal (POSIX `<dirent.h>`, Windows
 * `FindFirstFileA`/`FindNextFileA`, and WCPL/WASI `dirent.h`) to MARV's
 * internal ABI matching WCPL's `struct dirent` layout and constants:
 *
 *   struct marv_dirent {
 *     uint64_t      d_ino;       // offset 0  (8 bytes)
 *     unsigned char d_type;      // offset 8  (1 byte)
 *     char          d_name[256]; // offset 9  (inline NUL-terminated UTF-8 string)
 *   };
 *
 * File type constants match WCPL / WASI `dirent.h`:
 *   DT_UNKNOWN = 0, DT_BLK = 1, DT_CHR = 2, DT_DIR = 3,
 *   DT_REG = 4, DT_FIFO = 6, DT_LNK = 7.
 *
 * Pointer arrays (`scandir` namelist) use uniform 8-byte (`uint64_t`) pointer
 * slots across 32-bit WASM (`.wo`) and 64-bit native (`C99` / `RV64G`).
 */

#if !defined(__WCPL__)
#define _DEFAULT_SOURCE 1
#define _POSIX_C_SOURCE 200809L
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#if defined(__WCPL__)
#pragma module "dirent"
#include <wasi/api.h>
#include <fcntl.h>
#include <unistd.h>
#elif defined(_WIN32)
#include <windows.h>
#else
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

typedef int32_t  marv_i32;
typedef int64_t  marv_i64;
typedef void*    marv_ptr;

#define MARV_DT_UNKNOWN 0
#define MARV_DT_BLK     1
#define MARV_DT_CHR     2
#define MARV_DT_DIR     3
#define MARV_DT_REG     4
#define MARV_DT_FIFO    6
#define MARV_DT_LNK     7

typedef struct marv_dirent {
  uint64_t      d_ino;
  unsigned char d_type;
  char          d_name[256];
} marv_dirent_t;

typedef struct marv_dir {
  marv_dirent_t *entries;
  int32_t        count;
  int32_t        cap;
  int64_t        pos;
  marv_dirent_t  cur;
} marv_dir_t;

static int is_dot_or_dotdot(const char *s) {
  if (s[0] == '.') {
    if (s[1] == '\0') return 1;
    if (s[1] == '.' && s[2] == '\0') return 1;
  }
  return 0;
}

static int append_entry(marv_dir_t *d, uint64_t ino, unsigned char dtype, const char *name) {
  int32_t nlen = 0;
  marv_dirent_t *e;
  if (is_dot_or_dotdot(name)) return 0;
  while (name[nlen] != '\0' && nlen < 255) nlen++;

  if (d->count >= d->cap) {
    int32_t new_cap = (d->cap == 0) ? 16 : (d->cap * 2);
    marv_dirent_t *new_arr = (marv_dirent_t *)realloc(
      d->entries, (size_t)new_cap * sizeof(marv_dirent_t));
    if (!new_arr) return -1;
    d->entries = new_arr;
    d->cap = new_cap;
  }

  e = &d->entries[d->count++];
  memset(e, 0, sizeof(marv_dirent_t));
  e->d_ino  = (ino == 0) ? (uint64_t)d->count : ino;
  e->d_type = dtype;
  memcpy(&e->d_name[0], name, (size_t)nlen);
  e->d_name[nlen] = '\0';
  return 0;
}

#if defined(__WCPL__)
static int populate_dir_entries(marv_dir_t *d, const char *path) {
  int fd = open(path, O_RDONLY | O_DIRECTORY);
  uint8_t buf[4096];
  size_t buf_used = 0;
  size_t offset = 0;
  if (fd < 0) return -1;
  if (fd_readdir(fd, &buf[0], sizeof(buf), DIRCOOKIE_START, &buf_used) != 0) {
    close(fd);
    return -1;
  }
  while (offset + sizeof(dirent_t) <= buf_used) {
    dirent_t de;
    char name[256];
    size_t namlen;
    memcpy(&de, &buf[offset], sizeof(dirent_t));
    offset += sizeof(dirent_t);
    namlen = (size_t)de.d_namlen;
    if (offset + namlen > buf_used) break;
    if (namlen > 255) namlen = 255;
    memcpy(&name[0], &buf[offset], namlen);
    name[namlen] = '\0';
    offset += (size_t)de.d_namlen;
    if (append_entry(d, (uint64_t)de.d_ino, (unsigned char)de.d_type, &name[0]) < 0) {
      close(fd);
      return -1;
    }
  }
  close(fd);
  return 0;
}
#elif defined(_WIN32)
static int populate_dir_entries(marv_dir_t *d, const char *path) {
  char pattern[1024];
  size_t plen = strlen(path);
  WIN32_FIND_DATAA ffd;
  HANDLE hFind;
  if (plen + 3 >= sizeof(pattern)) return -1;
  memcpy(pattern, path, plen);
  if (plen > 0 && (path[plen - 1] == '/' || path[plen - 1] == '\\')) {
    pattern[plen] = '*';
    pattern[plen + 1] = '\0';
  } else {
    pattern[plen] = '/';
    pattern[plen + 1] = '*';
    pattern[plen + 2] = '\0';
  }
  hFind = FindFirstFileA(pattern, &ffd);
  if (hFind == INVALID_HANDLE_VALUE) return -1;
  do {
    unsigned char dt = MARV_DT_REG;
    uint64_t ino;
    if (ffd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
      dt = MARV_DT_LNK;
    } else if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
      dt = MARV_DT_DIR;
    }
    ino = 0;
    if (append_entry(d, ino, dt, ffd.cFileName) < 0) {
      FindClose(hFind);
      return -1;
    }
  } while (FindNextFileA(hFind, &ffd) != 0);
  FindClose(hFind);
  return 0;
}
#else
static int populate_dir_entries(marv_dir_t *d, const char *path) {
  DIR *os_dir = opendir(path);
  struct dirent *de;
  if (!os_dir) return -1;
  while ((de = readdir(os_dir)) != NULL) {
    unsigned char dt = MARV_DT_UNKNOWN;
#if defined(DT_DIR) && defined(DT_REG)
    switch (de->d_type) {
      case DT_DIR:  dt = MARV_DT_DIR;  break;
      case DT_REG:  dt = MARV_DT_REG;  break;
      case DT_LNK:  dt = MARV_DT_LNK;  break;
      case DT_CHR:  dt = MARV_DT_CHR;  break;
      case DT_BLK:  dt = MARV_DT_BLK;  break;
      case DT_FIFO: dt = MARV_DT_FIFO; break;
#ifdef DT_SOCK
      case DT_SOCK: dt = MARV_DT_FIFO; break;
#endif
      default:      dt = MARV_DT_UNKNOWN; break;
    }
#endif
    if (dt == MARV_DT_UNKNOWN) {
      char full[1024];
      size_t plen = strlen(path);
      size_t nlen = strlen(de->d_name);
      struct stat st;
      if (plen + 1 + nlen < sizeof(full)) {
        memcpy(full, path, plen);
        full[plen] = '/';
        memcpy(full + plen + 1, de->d_name, nlen + 1);
        if (stat(full, &st) == 0) {
          if (S_ISDIR(st.st_mode))       dt = MARV_DT_DIR;
          else if (S_ISREG(st.st_mode))  dt = MARV_DT_REG;
          else if (S_ISLNK(st.st_mode))  dt = MARV_DT_LNK;
          else if (S_ISCHR(st.st_mode))  dt = MARV_DT_CHR;
          else if (S_ISBLK(st.st_mode))  dt = MARV_DT_BLK;
          else if (S_ISFIFO(st.st_mode)) dt = MARV_DT_FIFO;
        }
      }
    }
    if (append_entry(d, (uint64_t)de->d_ino, dt, de->d_name) < 0) {
      closedir(os_dir);
      return -1;
    }
  }
  closedir(os_dir);
  return 0;
}
#endif

marv_ptr dirent_opendir(marv_ptr path_ptr) {
  const char *path = (const char *)path_ptr;
  marv_dir_t *d;
  if (!path || path[0] == '\0') return (marv_ptr)0;

  d = (marv_dir_t *)calloc(1, sizeof(marv_dir_t));
  if (!d) return (marv_ptr)0;

  if (populate_dir_entries(d, path) < 0) {
    free(d->entries);
    free(d);
    return (marv_ptr)0;
  }

  d->pos = 0;
  return (marv_ptr)d;
}

marv_ptr dirent_fdopendir(marv_i32 fd) {
  (void)fd;
  return dirent_opendir((marv_ptr)".");
}

marv_ptr dirent_opendirat(marv_i32 dirfd, marv_ptr dirname) {
  (void)dirfd;
  return dirent_opendir(dirname);
}

marv_i32 dirent_dirfd(marv_ptr dirp) {
  return dirp ? 0 : -1;
}

marv_ptr dirent_readdir(marv_ptr dir_ptr) {
  marv_dir_t *d = (marv_dir_t *)dir_ptr;
  if (!d || d->pos < 0 || d->pos >= (int64_t)d->count) {
    return (marv_ptr)0;
  }
  d->cur = d->entries[(int32_t)(d->pos++)];
  return (marv_ptr)&d->cur;
}

void dirent_rewinddir(marv_ptr dir_ptr) {
  marv_dir_t *d = (marv_dir_t *)dir_ptr;
  if (d) d->pos = 0;
}

marv_i64 dirent_telldir(marv_ptr dir_ptr) {
  marv_dir_t *d = (marv_dir_t *)dir_ptr;
  return d ? (marv_i64)d->pos : (marv_i64)-1;
}

void dirent_seekdir(marv_ptr dir_ptr, marv_i64 loc) {
  marv_dir_t *d = (marv_dir_t *)dir_ptr;
  if (!d) return;
  if (loc < 0) loc = 0;
  if (loc > (int64_t)d->count) loc = (int64_t)d->count;
  d->pos = loc;
}

marv_i32 dirent_closedir(marv_ptr dir_ptr) {
  marv_dir_t *d = (marv_dir_t *)dir_ptr;
  if (!d) return -1;
  free(d->entries);
  free(d);
  return 0;
}

marv_i32 dirent_fdclosedir(marv_ptr dir_ptr) {
  return dirent_closedir(dir_ptr);
}

marv_i64 dirent_d_ino(marv_ptr ent_ptr) {
  const marv_dirent_t *e = (const marv_dirent_t *)ent_ptr;
  return e ? (marv_i64)e->d_ino : 0;
}

marv_i32 dirent_d_type(marv_ptr ent_ptr) {
  const marv_dirent_t *e = (const marv_dirent_t *)ent_ptr;
  return e ? (marv_i32)e->d_type : MARV_DT_UNKNOWN;
}

marv_i32 dirent_d_namlen(marv_ptr ent_ptr) {
  const marv_dirent_t *e = (const marv_dirent_t *)ent_ptr;
  return e ? (marv_i32)strlen(&e->d_name[0]) : 0;
}

marv_ptr dirent_d_name(marv_ptr ent_ptr) {
  marv_dirent_t *e = (marv_dirent_t *)ent_ptr;
  return e ? (marv_ptr)&e->d_name[0] : (marv_ptr)0;
}

/*
 * `dirent_alphasort(da, db)`
 * `da` and `db` are pointers to 8-byte pointer slots (`uint64_t *`), matching
 * POSIX `alphasort(const struct dirent **, const struct dirent **)` under
 * MARV's uniform 8-byte pointer slot ABI.
 */
marv_i32 dirent_alphasort(marv_ptr da, marv_ptr db) {
  const uint64_t *sa = (const uint64_t *)da;
  const uint64_t *sb = (const uint64_t *)db;
  const marv_dirent_t *ea;
  const marv_dirent_t *eb;
  if (!sa || !sb) return 0;
  ea = (const marv_dirent_t *)(uintptr_t)(*sa);
  eb = (const marv_dirent_t *)(uintptr_t)(*sb);
  if (!ea || !eb) return 0;
  return (marv_i32)strcmp(&ea->d_name[0], &eb->d_name[0]);
}

typedef marv_i32 (*marv_dirent_filter_fn)(marv_ptr ent);
typedef marv_i32 (*marv_dirent_compar_fn)(marv_ptr da, marv_ptr db);

marv_i32 dirent_scandir(
    marv_ptr path_ptr,
    marv_ptr namelist_out,
    marv_ptr filter_fn,
    marv_ptr compar_fn) {
  marv_dir_t *d = (marv_dir_t *)dirent_opendir(path_ptr);
  marv_dirent_filter_fn filt = (marv_dirent_filter_fn)filter_fn;
  marv_dirent_compar_fn cmp  = (marv_dirent_compar_fn)compar_fn;
  uint64_t *slots = NULL;
  int32_t n = 0;
  int32_t i;

  if (!d) return -1;

  slots = (uint64_t *)calloc((size_t)(d->count > 0 ? d->count : 1), sizeof(uint64_t));
  if (!slots) {
    dirent_closedir((marv_ptr)d);
    return -1;
  }

  for (i = 0; i < d->count; i++) {
    marv_dirent_t *src = &d->entries[i];
    if (!filt || (*filt)((marv_ptr)src) != 0) {
      marv_dirent_t *copy = (marv_dirent_t *)malloc(sizeof(marv_dirent_t));
      if (!copy) {
        int32_t k;
        for (k = 0; k < n; k++) {
          free((void *)(uintptr_t)slots[k]);
        }
        free(slots);
        dirent_closedir((marv_ptr)d);
        return -1;
      }
      *copy = *src;
      slots[n++] = (uint64_t)(uintptr_t)copy;
    }
  }

  dirent_closedir((marv_ptr)d);

  if (cmp && n > 1) {
    int32_t a;
    for (a = 1; a < n; a++) {
      uint64_t key_slot = slots[a];
      int32_t b = a - 1;
      while (b >= 0) {
        uint64_t cur_slot = slots[b];
        if ((*cmp)((marv_ptr)&cur_slot, (marv_ptr)&key_slot) > 0) {
          slots[b + 1] = slots[b];
          b--;
        } else {
          break;
        }
      }
      slots[b + 1] = key_slot;
    }
  }

  if (namelist_out) {
    uint64_t list_slot = (uint64_t)(uintptr_t)slots;
    memcpy(namelist_out, &list_slot, 8);
  } else {
    for (i = 0; i < n; i++) {
      free((void *)(uintptr_t)slots[i]);
    }
    free(slots);
  }

  return (marv_i32)n;
}

marv_i32 dirent_scandirat(
    marv_i32 dirfd,
    marv_ptr path_ptr,
    marv_ptr namelist_out,
    marv_ptr filter_fn,
    marv_ptr compar_fn) {
  (void)dirfd;
  return dirent_scandir(path_ptr, namelist_out, filter_fn, compar_fn);
}

void dirent_freelist(marv_ptr namelist, marv_i32 count) {
  uint64_t *slots = (uint64_t *)namelist;
  int32_t i;
  if (!slots) return;
  for (i = 0; i < count; i++) {
    void *ent = (void *)(uintptr_t)slots[i];
    if (ent) free(ent);
  }
  free(slots);
}

#if defined(__WCPL__)
/* Under WCPL (`#pragma module "dirent"`), export unprefixed names so
 * `$dirent:opendir`, `$dirent:scandir`, etc. resolve directly in `.wo`! */
marv_ptr fdopendir(marv_i32 fd) { return dirent_fdopendir(fd); }
marv_i32 fdclosedir(marv_ptr d) { return dirent_fdclosedir(d); }
marv_ptr opendirat(marv_i32 fd, marv_ptr p) { return dirent_opendirat(fd, p); }
marv_ptr opendir(marv_ptr p) { return dirent_opendir(p); }
marv_i32 dirfd(marv_ptr d) { return dirent_dirfd(d); }
marv_ptr readdir(marv_ptr d) { return dirent_readdir(d); }
void     rewinddir(marv_ptr d) { dirent_rewinddir(d); }
marv_i64 telldir(marv_ptr d) { return dirent_telldir(d); }
void     seekdir(marv_ptr d, marv_i64 loc) { dirent_seekdir(d, loc); }
marv_i32 closedir(marv_ptr d) { return dirent_closedir(d); }
marv_i64 d_ino(marv_ptr e) { return dirent_d_ino(e); }
marv_i32 d_type(marv_ptr e) { return dirent_d_type(e); }
marv_i32 d_namlen(marv_ptr e) { return dirent_d_namlen(e); }
marv_ptr d_name(marv_ptr e) { return dirent_d_name(e); }
marv_i32 alphasort(marv_ptr a, marv_ptr b) { return dirent_alphasort(a, b); }
marv_i32 scandirat(marv_i32 fd, marv_ptr p, marv_ptr nl, marv_ptr f, marv_ptr c) {
  return dirent_scandirat(fd, p, nl, f, c);
}
marv_i32 scandir(marv_ptr p, marv_ptr nl, marv_ptr f, marv_ptr c) {
  return dirent_scandir(p, nl, f, c);
}
void     freelist(marv_ptr nl, marv_i32 n) { dirent_freelist(nl, n); }
#endif
