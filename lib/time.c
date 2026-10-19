/* Single-source MARV-ABI adapter for `time` (WCPL + C99 + RV64G) */
#if defined(__WCPL__)
#pragma module "time"
#include <wasi/api.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>

#define TZ_STRLEN_MAX 255
#define ISDIGIT(c) ((c) >= '0' && (c) <= '9')
/* 2000-03-01 (mod 400 year, immediately after feb29 */
#define LEAPOCH (946684800LL + 86400*(31+29))
#define DAYS_PER_400Y (365*400 + 97)
#define DAYS_PER_100Y (365*100 + 24)
#define DAYS_PER_4Y   (365*4   + 1)
#define DAYS_PER_WEEK 7
/* year range is from 1901 to 2038, so no century can occur which is not a leap year
 * #define is_leap(y)  ((y) % 4 == 0 && ((y) % 100 != 0 || (y) % 400 == 0)) */
#define is_leap(y)  ((y) % 4 == 0)

static int days_in_month[] = {
  31, 30, 31, 30, 31, 31, 30, 31, 30, 31, 31, 29
};

static const int mon_days[] = {
  31,
  31 + 28,
  31 + 28 + 31,
  31 + 28 + 31 + 30,
  31 + 28 + 31 + 30 + 31,
  31 + 28 + 31 + 30 + 31 + 30,
  31 + 28 + 31 + 30 + 31 + 30 + 31,
  31 + 28 + 31 + 30 + 31 + 30 + 31 + 31,
  31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30,
  31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31,
  31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30,
  31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30 + 31
};

static const char *wday_name[7] = {
  "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

static const char *wday_full_name[] = { 
  "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" 
};

static const char *mon_name[12] = {
  "Jan", "Feb", "Mar", "Apr", "May", "Jun",
  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

static const char *mon_full_name[] = { 
  "January", "February", "March", "April", "May", "June", 
  "July", "August", "Sepember", "October", "November", "December" 
};

/* hidden state for non-reentrant functions */
static char asctime_buf[26];
static struct tm gmtime_buf;

/* timezone internals */
static char _tzname[2][TZ_STRLEN_MAX + 1];
static char	_tzval[TZ_STRLEN_MAX + 1];
static int	_tzset = 0;
static char _tzdflt[] = {(char)' ', (char)' ', (char)' ', (char)0};
/* public globals */
long  timezone = 0;
char* tzname[2] = { &_tzdflt[0], &_tzdflt[0] };


time_t time(time_t *pt) 
{
  timestamp_t ts = 0;
  errno_t error = clock_time_get(CLOCKID_REALTIME, CLOCKS_PER_SEC, &ts);
  if (!error) ts /= CLOCKS_PER_SEC; else ts = -1LL;
  if (pt != NULL) *pt = (time_t)ts; 
  return (time_t)ts;
}

clock_t clock(void) 
{
  timestamp_t ts = 0;
  errno_t error = clock_time_get(CLOCKID_PROCESS_CPUTIME_ID, 1LL, &ts);
  if (error) return (clock_t)-1LL;
  return (clock_t)ts;
}

double difftime(time_t t1, time_t t0)
{
  return t1-t0;
}

static int secs_to_tm(long long t, struct tm *ptm)
{
  long long days, secs, years;
  int remdays, remsecs, remyears;
  int qc_cycles, c_cycles, q_cycles;
  int months;
  int wday, yday, leap;

  /* reject time_t values whose year would overflow int */
  if (t < INT_MIN * 31622400LL || t > INT_MAX * 31622400LL)
    return -1;

  secs = t - LEAPOCH;
  days = secs / 86400;
  remsecs = (int)(secs % 86400);
  if (remsecs < 0) {
    remsecs += 86400;
    --days;
  }

  wday = (int)((3+days) % 7);
  if (wday < 0) wday += 7;

  qc_cycles = (int)(days / DAYS_PER_400Y);
  remdays = (int)(days % DAYS_PER_400Y);
  if (remdays < 0) {
    remdays += DAYS_PER_400Y;
    --qc_cycles;
  }

  c_cycles = remdays / DAYS_PER_100Y;
  if (c_cycles == 4) --c_cycles;
  remdays -= c_cycles * DAYS_PER_100Y;

  q_cycles = remdays / DAYS_PER_4Y;
  if (q_cycles == 25) --q_cycles;
  remdays -= q_cycles * DAYS_PER_4Y;

  remyears = remdays / 365;
  if (remyears == 4) --remyears;
  remdays -= remyears * 365;

  leap = !remyears && (q_cycles || !c_cycles);
  yday = remdays + 31 + 28 + leap;
  if (yday >= 365+leap) yday -= 365+leap;

  years = remyears + 4*q_cycles + 100*c_cycles + 400LL*qc_cycles;

  for (months=0; days_in_month[months] <= remdays; months++)
    remdays -= days_in_month[months];

  if (months >= 10) {
    months -= 12;
    ++years;
  }

  if (years+100 > INT_MAX || years+100 < INT_MIN)
    return -1;

  ptm->tm_year = (int)years + 100;
  ptm->tm_mon = months + 2;
  ptm->tm_mday = remdays + 1;
  ptm->tm_wday = wday;
  ptm->tm_yday = yday;

  ptm->tm_hour = remsecs / 3600;
  ptm->tm_min = remsecs / 60 % 60;
  ptm->tm_sec = remsecs % 60;

  return 0;
}

struct tm *gmtime_r(const time_t *pt, struct tm *ptm)
{
  if (secs_to_tm(*pt, ptm) < 0) {
    errno = EOVERFLOW;
    return NULL;
  }
  ptm->tm_isdst = 0;
  return ptm;
}

static const char *get_tz_name(const char *src, char *dest)
{
  int tzlen = 0;
  if (*src != ':') {
    while (*src != '\0' && *src != ',' && *src != '+' && *src != '-' 
        && !ISDIGIT(*src) && tzlen < TZ_STRLEN_MAX) {
      *dest++ = *src++;
      ++tzlen;
    }
  }
  if (tzlen == 0) {
    strcpy(dest, _tzdflt);
  } else {
    *dest = '\0';
  }
  return src;
}

static const char *get_tz_offset(const char *src, int *error)
{
  if (*src == '\0') {
    timezone = 0;
    *error   = 0;
  } else {
    enum { Hours, Minutes, Seconds };
    int tim[3]; tim[0] = tim[1] = tim[2] = 0; /* = { 0, 0, 0 }; */
    long mul = 1L; int i;
    if (*src == '-') {
      mul = -1L;
      ++src;
    } else if (*src == '+') {
      ++src;
    }
    for (i = 0; i < 3; ++i) {  /* sizeof(tim) / sizeof(tim[0]) == 3 */
      if (*src == '\0') { /* invalid */
        mul = 0L;
      } else if (ISDIGIT(*src)) {
        tim[i] = *src++ - '0';
        if (ISDIGIT(*src)) {
          tim[i] = 10 * tim[i] + *src++ - '0';
          if (ISDIGIT(*src)) { /* not more than two digits allowed */
            mul = 0L;
          }
        }
      }
      if (mul == 0L) { /* invalid offset format */
        break;
      } else if (*src == ':') { /* next field */
        ++src;
      } else { /* end of offset information */
        break;
      }
    }
    if (tim[Minutes] > 59 || tim[Seconds] > 59) { /* invalid minutes or seconds */
      mul = 0L;
    }
    timezone = mul * ((long)tim[Seconds] + 60L * (long)tim[Minutes] + 3600L * (long)tim[Hours]);
    *error = (mul == 0L);
  }
  return src;
}

/* tzset() relies on TZ env variable */
void tzset(void)
{
  const char* tzval = getenv("TZ");
  if (tzval == NULL) tzval = "";
  if (!_tzset || strcmp(_tzval, tzval) != 0) {
    int error;
    _tzset = (strlen(tzval) <= TZ_STRLEN_MAX);
    if (_tzset) strcpy(_tzval, tzval);
    tzval = get_tz_name(tzval, _tzname[0]);
    tzval = get_tz_offset(tzval, &error);
    if (error) {
      tzname[0] = _tzdflt;
      tzname[1] = _tzdflt;
    } else {
      tzval = get_tz_name(tzval, _tzname[1]);
      tzname[0] = _tzname[0];
      tzname[1] = _tzname[1];
    }
  }
}

time_t mktime(struct tm *tm)
{
  long tyears, tdays, leaps, utc_hrs;
  int leap_year;

  tzset();
  leap_year = is_leap(tm->tm_year + 1900);
  tyears = tm->tm_year - 70;
  leaps = (tyears + 2) / 4;
  if (tyears < 0) {
    leaps = (tyears - 2) / 4;
    if (leap_year && tm->tm_mon > 1) ++leaps;
  } else {
    leaps = (tyears + 2) / 4; /* no of next two lines until year 2100. */
    if (leap_year && (tm->tm_mon == 0 || tm->tm_mon == 1)) --leaps;
  }
  tdays = (tm->tm_mon > 0) ? mon_days[tm->tm_mon - 1] : 0;
  tdays += tm->tm_mday - 1; /* days of month passed. */
  tdays += (tyears * 365) + leaps;
  utc_hrs = tm->tm_hour;

  return (tdays * 86400L) + (utc_hrs * 3600L) + ((long)tm->tm_min * 60L) + (long)tm->tm_sec + timezone;
}

char *asctime_r(const struct tm *ptm, char *buf)
{
  snprintf(buf, 26, 
    "%s %s%3d %.2d:%.2d:%.2d %d\n",
    wday_name[ptm->tm_wday], mon_name[ptm->tm_mon],
    ptm->tm_mday, ptm->tm_hour,
    ptm->tm_min, ptm->tm_sec,
    1900 + ptm->tm_year);
  return buf;
}

struct tm *localtime_r(const time_t *pt, struct tm *ptm)
{
  time_t t = *pt;
  tzset();
  t -= (time_t)timezone;
  return gmtime_r(&t, ptm);
}

char *ctime_r(const time_t *pt, char *buf)
{
  struct tm tm, *ptm = localtime_r(pt, &tm);
  return ptm ? asctime_r(ptm, buf) : (char*)NULL;
}


/* non-reentrant time utils */

struct tm *gmtime(const time_t *pt)
{
  return gmtime_r(pt, &gmtime_buf);
}

char *asctime(const struct tm *ptm)
{
  return asctime_r(ptm, &asctime_buf[0]);
}

struct tm *localtime(const time_t* pt)
{
  return localtime_r(pt, &gmtime_buf);
}

char *ctime(const time_t *pt)
{
  struct tm *ptm = localtime_r(pt, &gmtime_buf);
  if (!ptm) return NULL;
  return asctime_r(ptm, &asctime_buf[0]);
}


/* formatting */

static int get_week(const struct tm *tp, int start_monday)
{
  int week = tp->tm_yday + DAYS_PER_WEEK;
  if (start_monday) {
    if (tp->tm_wday == 0) week -= DAYS_PER_WEEK - 1;
    else week -= tp->tm_wday - 1;
  } else {
    week -= tp->tm_wday;
  }
  return week / DAYS_PER_WEEK;
}

size_t strftime(char *s, size_t smax, const char *fmt, const struct tm *tp)
{
  char *ptr = s; size_t count = 0;

  tzset();
  do {
    if (*fmt == '%') {
      const char *addstr = NULL;
      int addlen = -1;
      char addval[80];
      int week;
      switch (*++fmt) {
        case 'a':
          addlen = 3; /* abbrevation = first three characters */
          /* fall thru */
        case 'A':
          if (tp->tm_wday < 0 || tp->tm_wday > 6) {
            addstr = "?";
            addlen = -1;
          } else {
            addstr = wday_full_name[tp->tm_wday];
          }
          break;
        case 'b':
          addlen = 3; /* abbrevation = first three characters */
          /* fall thru */
        case 'B':
          if (tp->tm_mon < 0 || tp->tm_mon > 11) {
            addstr = "?";
            addlen = -1;
          } else {
            addstr = mon_full_name[tp->tm_mon];
          }
          break;
        case 'c':
          strftime(addval, 80, "%a %b %d %x %Y", tp);
          break;
        case 'd':
          addval[0] = '0' + (tp->tm_mday / 10);
          addval[1] = '0' + (tp->tm_mday % 10);
          addval[2] = '\0';
          break;
        case 'H':
          addval[0] = '0' + (tp->tm_hour / 10);
          addval[1] = '0' + (tp->tm_hour % 10);
          addval[2] = '\0';
          break;
        case 'I': {
            int hour12 = tp->tm_hour % 12;
            if (hour12 == 0) hour12 = 12;
            addval[0] = '0' + (hour12 / 10);
            addval[1] = '0' + (hour12 % 10);
            addval[2] = '\0';
          }
          break;
        case 'j':
          addval[0] = '0' + ((tp->tm_yday + 1) / 100);
          addval[1] = '0' + (((tp->tm_yday + 1) % 100) / 10);
          addval[2] = '0' + ((tp->tm_yday + 1) % 10);
          addval[3] = '\0';
          break;
        case 'm':
          addval[0] = '0' + ((tp->tm_mon + 1) / 10);
          addval[1] = '0' + ((tp->tm_mon + 1) % 10);
          addval[2] = '\0';
          break;
        case 'M':
          addval[0] = '0' + (tp->tm_min / 10);
          addval[1] = '0' + (tp->tm_min % 10);
          addval[2] = '\0';
          break;
        case 'p':
          if (tp->tm_hour > 0 && tp->tm_hour < 13) addstr = "AM";
          else addstr = "PM";
          break;
        case 'S':
          addval[0] = '0' + (tp->tm_sec / 10);
          addval[1] = '0' + (tp->tm_sec % 10);
          addval[2] = '\0';
          break;
        case 'U':
        case 'W':
          week = get_week(tp, *fmt == 'U' ? 0 : 1);
          addval[0] = '0' + (week / 10);
          addval[1] = '0' + (week % 10);
          addval[2] = '\0';
          break;
        case 'w':
          addval[0] = '0' + (tp->tm_wday / 10);
          addval[1] = '0' + (tp->tm_wday % 10);
          addval[2] = '\0';
          break;
        case 'X':
          strftime(addval, 80, "%d.%m.%Y", tp);
          break;
        case 'x':
          strftime(addval, 80, "%H:%M:%S", tp);
          break;
        case 'y':
          addval[0] = '0' + ((tp->tm_year % 100) / 10);
          addval[1] = '0' + ((tp->tm_year % 100) % 10);
          addval[2] = '\0';
          break;
        case 'Y':
          addval[0] = '0' + ((tp->tm_year + 1900) / 1000);
          addval[1] = '0' + (((tp->tm_year + 1900) % 1000) / 100);
          addval[2] = '0' + (((tp->tm_year + 1900) % 100) / 10);
          addval[3] = '0' + ((tp->tm_year + 1900) % 10);
          addval[4] = '\0';
          break;
        case 'Z':
          addstr = tzname[tp->tm_isdst > 0];
          break;
        case '%':
          addstr = "%";
          break;
        default:
          addval[0] = '%';
          addval[1] = *fmt;
          addval[2] = '\0';
          break;
      }
      if (addstr == NULL) addstr = addval;
      if (*addstr != '\0') {
        size_t len = (addlen < 0) ? strlen(addstr) : addlen;
        if (count + len > smax) len = smax - count;
        strncpy(ptr, addstr, len);
        ptr += len;
        count += len;
      }
      ++fmt;
    } else {
      if (count <= smax) *ptr++ = *fmt;
      ++fmt;
    }
  } while (fmt[-1] != '\0');

  if (count > smax) count = 0;
  return count;
}

#else
/*
 * lib/time.c — MARV-ABI C library adapter for `time.mvh`
 *
 * Uses WCPL's exact 36-byte `struct tm` layout and deterministic UTC calendar
 * math (`secs_to_tm` / `tm_to_secs`) so date/time conversions match across
 * `.wo`, `C99`, and `RV64G`.
 */

#include "marv.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

typedef struct marv_tm {
  marv_i32 tm_sec;
  marv_i32 tm_min;
  marv_i32 tm_hour;
  marv_i32 tm_mday;
  marv_i32 tm_mon;
  marv_i32 tm_year;
  marv_i32 tm_wday;
  marv_i32 tm_yday;
  marv_i32 tm_isdst;
} marv_tm_t;

#define MARV_LEAPOCH (946684800LL + 86400LL * (31 + 29))
#define MARV_DAYS_PER_400Y (365 * 400 + 97)
#define MARV_DAYS_PER_100Y (365 * 100 + 24)
#define MARV_DAYS_PER_4Y   (365 * 4 + 1)

static const int marv_days_in_month[12] = {
  31, 30, 31, 30, 31, 31, 30, 31, 30, 31, 31, 29
};

static const int marv_mon_days[12] = {
  0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
};

static const char *marv_wday_name[7] = {
  "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

static const char *marv_mon_name[12] = {
  "Jan", "Feb", "Mar", "Apr", "May", "Jun",
  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

static marv_tm_t marv_gmtime_buf;
static char marv_asctime_buf[32];

static int marv_secs_to_tm(int64_t t, marv_tm_t *ptm) {
  int64_t secs = t - MARV_LEAPOCH;
  int64_t days = secs / 86400;
  int remsecs = (int)(secs % 86400);
  if (remsecs < 0) {
    remsecs += 86400;
    --days;
  }
  int wday = (int)((3 + days) % 7);
  if (wday < 0) wday += 7;

  int qc_cycles = (int)(days / MARV_DAYS_PER_400Y);
  int remdays = (int)(days % MARV_DAYS_PER_400Y);
  if (remdays < 0) {
    remdays += MARV_DAYS_PER_400Y;
    --qc_cycles;
  }
  int c_cycles = remdays / MARV_DAYS_PER_100Y;
  if (c_cycles == 4) --c_cycles;
  remdays -= c_cycles * MARV_DAYS_PER_100Y;

  int q_cycles = remdays / MARV_DAYS_PER_4Y;
  if (q_cycles == 25) --q_cycles;
  remdays -= q_cycles * MARV_DAYS_PER_4Y;

  int remyears = remdays / 365;
  if (remyears == 4) --remyears;
  remdays -= remyears * 365;

  int leap = (!remyears && (q_cycles || !c_cycles)) ? 1 : 0;
  int yday = remdays + 31 + 28 + leap;
  if (yday >= 365 + leap) yday -= 365 + leap;

  int64_t years = remyears + 4 * q_cycles + 100 * c_cycles + 400LL * qc_cycles;
  int months = 0;
  for (; marv_days_in_month[months] <= remdays; months++) {
    remdays -= marv_days_in_month[months];
  }
  if (months >= 10) {
    months -= 12;
    ++years;
  }
  ptm->tm_year  = (marv_i32)(years + 100);
  ptm->tm_mon   = (marv_i32)(months + 2);
  ptm->tm_mday  = (marv_i32)(remdays + 1);
  ptm->tm_wday  = (marv_i32)wday;
  ptm->tm_yday  = (marv_i32)yday;
  ptm->tm_hour  = (marv_i32)(remsecs / 3600);
  ptm->tm_min   = (marv_i32)((remsecs / 60) % 60);
  ptm->tm_sec   = (marv_i32)(remsecs % 60);
  ptm->tm_isdst = 0;
  return 0;
}

marv_i64 time_clock(void) {
  clock_t c = clock();
  if (c == (clock_t)-1) return -1;
  return (marv_i64)((double)c * (1000000000.0 / (double)CLOCKS_PER_SEC));
}

marv_f64 time_difftime(marv_i64 t1, marv_i64 t0) {
  return (marv_f64)(t1 - t0);
}

marv_i64 time_time(marv_ptr pt) {
  marv_i64 t = (marv_i64)time(NULL);
  if (pt) marv_i64_store(pt, 0, t);
  return t;
}

marv_ptr time_gmtime_r(marv_ptr pt, marv_ptr ptm) {
  if (!pt || !ptm) return MARV_NULL;
  marv_i64 t = marv_i64_load(pt, 0);
  marv_tm_t tm;
  if (marv_secs_to_tm(t, &tm) < 0) return MARV_NULL;
  memcpy(ptm, &tm, sizeof(tm));
  return ptm;
}

marv_ptr time_gmtime(marv_ptr pt) {
  return time_gmtime_r(pt, (marv_ptr)&marv_gmtime_buf);
}

marv_ptr time_localtime_r(marv_ptr pt, marv_ptr ptm) {
  return time_gmtime_r(pt, ptm);
}

marv_ptr time_localtime(marv_ptr pt) {
  return time_gmtime(pt);
}

marv_i64 time_mktime(marv_ptr ptm) {
  if (!ptm) return -1;
  marv_tm_t tm;
  memcpy(&tm, ptm, sizeof(tm));
  int64_t year = (int64_t)tm.tm_year + 1900;
  int64_t mon = tm.tm_mon;
  if (mon < 0 || mon >= 12) {
    int64_t adj = mon / 12;
    mon %= 12;
    if (mon < 0) { mon += 12; adj--; }
    year += adj;
  }
  int is_leap = ((year % 4 == 0) && ((year % 100 != 0) || (year % 400 == 0))) ? 1 : 0;
  int64_t y = year - 1970;
  int64_t leaps = ((year - 1) / 4 - 1969 / 4)
                - ((year - 1) / 100 - 1969 / 100)
                + ((year - 1) / 400 - 1969 / 400);
  int64_t days = y * 365 + leaps + marv_mon_days[mon] + ((mon >= 2 && is_leap) ? 1 : 0) + (tm.tm_mday - 1);
  int64_t secs = days * 86400LL + tm.tm_hour * 3600LL + tm.tm_min * 60LL + tm.tm_sec;
  marv_secs_to_tm(secs, &tm);
  memcpy(ptm, &tm, sizeof(tm));
  return (marv_i64)secs;
}

marv_ptr time_asctime_r(marv_ptr ptm, marv_ptr buf) {
  if (!ptm || !buf) return MARV_NULL;
  marv_tm_t tm;
  memcpy(&tm, ptm, sizeof(tm));
  const char *wd = (tm.tm_wday >= 0 && tm.tm_wday < 7) ? marv_wday_name[tm.tm_wday] : "???";
  const char *mn = (tm.tm_mon >= 0 && tm.tm_mon < 12) ? marv_mon_name[tm.tm_mon] : "???";
  char tmp[64];
  snprintf(tmp, sizeof(tmp), "%.3s %.3s%3d %.2d:%.2d:%.2d %d\n",
           wd, mn, (int)tm.tm_mday, (int)tm.tm_hour, (int)tm.tm_min,
           (int)tm.tm_sec, (int)(1900 + tm.tm_year));
  memcpy(buf, tmp, 25);
  ((char *)buf)[25] = '\0';
  return buf;
}

marv_ptr time_asctime(marv_ptr ptm) {
  return time_asctime_r(ptm, (marv_ptr)marv_asctime_buf);
}

marv_ptr time_ctime_r(marv_ptr pt, marv_ptr buf) {
  marv_tm_t tm;
  if (!time_gmtime_r(pt, (marv_ptr)&tm)) return MARV_NULL;
  return time_asctime_r((marv_ptr)&tm, buf);
}

marv_ptr time_ctime(marv_ptr pt) {
  return time_ctime_r(pt, (marv_ptr)marv_asctime_buf);
}

marv_i32 time_strftime(marv_ptr s, marv_i32 maxsz, marv_ptr fmt, marv_ptr ptm) {
  if (!s || maxsz <= 0 || !fmt || !ptm) return 0;
  marv_tm_t mtm;
  memcpy(&mtm, ptm, sizeof(mtm));
  struct tm htm;
  memset(&htm, 0, sizeof(htm));
  htm.tm_sec   = mtm.tm_sec;
  htm.tm_min   = mtm.tm_min;
  htm.tm_hour  = mtm.tm_hour;
  htm.tm_mday  = mtm.tm_mday;
  htm.tm_mon   = mtm.tm_mon;
  htm.tm_year  = mtm.tm_year;
  htm.tm_wday  = mtm.tm_wday;
  htm.tm_yday  = mtm.tm_yday;
  htm.tm_isdst = mtm.tm_isdst;
  return (marv_i32)strftime((char *)s, (size_t)(uint32_t)maxsz, (const char *)fmt, &htm);
}

void time_tzset(void) {
}

#endif
