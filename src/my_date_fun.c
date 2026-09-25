/*
 * This file is part of Mico's MM-15 Calculator
 *
 * Mico's MM-15 Calculator is free software: 
 * you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Mico's MM-15 Calculator is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Mico's MM-15 Calculator. If not, see <https://www.gnu.org/licenses/>.
 */

#define _POSIX_C_SOURCE 200809L

#include <gsl/gsl_complex.h>  // for GSL_REAL
#include <limits.h>           // for INT_MAX, INT_MIN
#include <math.h>             // for floor, isfinite
#include <stdio.h>            // for fprintf, stderr, sscanf, snprintf, NULL
#include <stdlib.h>           // for free
#include <string.h>           // for strdup
#include <sys/stat.h>         // for stat, S_ISREG (zone name validation)
#include <time.h>             // for tm, time_t, mktime, localtime, time, tzset
#include "stack.h"            // for stack_element, (anonymous struct)::(ano...
#include "my_date_fun.h"      // for date_plus_days, days_to_end_of_year

int extract_day_month_year(Stack* stack) {
  if (stack->top < 0) {
    fprintf(stderr, "Error: Stack underflow\n");
    return 1;
  }

  stack_element date_elem = stack->items[stack->top--];

  if (date_elem.type != TYPE_STRING) {
    fprintf(stderr, "Error: Expected string date in DD.MM.YYYY format\n");
    return 1;
  }

  int day, month, year;
  if (sscanf(date_elem.string, "%d.%d.%d", &day, &month, &year) != 3) {
    fprintf(stderr, "Error: Invalid date format. Expected DD.MM.YYYY\n");
    return 1;
  }

  // Push year
  stack_element y = { .type = TYPE_REAL, .real = year };
  stack->items[++stack->top] = y;

  // Push month
  stack_element m = { .type = TYPE_REAL, .real = month };
  stack->items[++stack->top] = m;

  // Push day
  stack_element d = { .type = TYPE_REAL, .real = day };
  stack->items[++stack->top] = d;

  return 0;
}

int date_plus_days(Stack* stack) {
  if (stack->top < 1) {
    fprintf(stderr, "Error: Need a date string and number of days on stack\n");
    return 1;
  }

  stack_element days_elem = stack->items[stack->top--];
  stack_element date_elem = stack->items[stack->top--];

  if (date_elem.type != TYPE_STRING || (days_elem.type != TYPE_REAL && days_elem.type != TYPE_COMPLEX))
    {
      fprintf(stderr, "Error: Expected a string and a number\n");
      return 1;
    }

  int day, month, year;
  if (sscanf(date_elem.string, "%d.%d.%d", &day, &month, &year) != 3) {
    fprintf(stderr, "Error: Invalid date format. Expected DD.MM.YYYY\n");
    return 1;
  }

  struct tm date = {0};
  date.tm_mday = day;
  date.tm_mon = month - 1;
  date.tm_year = year - 1900;
  date.tm_hour = 12; // avoid DST issues

  time_t t = mktime(&date);
  if (t == (time_t)(-1)) {
    fprintf(stderr, "Error: mktime failed\n");
    return 1;
  }

  int delta = (int)(days_elem.type == TYPE_COMPLEX ? GSL_REAL(days_elem.complex_val) : days_elem.real);
  t += delta * 86400;  // seconds per day

  struct tm* new_date = localtime(&t);
  if (!new_date) {
    fprintf(stderr, "Error: localtime failed\n");
    return 1;
  }

  char buffer[64];
  snprintf(buffer, sizeof(buffer), "%02d.%02d.%04d",
	   new_date->tm_mday,
	   new_date->tm_mon + 1,
	   new_date->tm_year + 1900);

  char* result = strdup(buffer);
  if (!result) {
    fprintf(stderr, "Error: Memory allocation failed\n");
    return 1;
  }

  stack_element out;
  out.type = TYPE_STRING;
  out.string = result;

  if (stack->top >= STACK_SIZE - 1) {
    fprintf(stderr, "Error: Stack overflow\n");
    free(result);
    return 1;
  }

  stack->items[++stack->top] = out;
  return 0;
}


int push_weekday_name_from_date_string(Stack* stack) {
  if (stack->top < 0) {
    fprintf(stderr, "Error: Stack underflow\n");
    return 1;
  }

  stack_element elem = stack->items[stack->top--];

  if (elem.type != TYPE_STRING) {
    fprintf(stderr, "Error: Expected string date in DD.MM.YYYY format\n");
    return 1;
  }

  int day, month, year;
  if (sscanf(elem.string, "%d.%d.%d", &day, &month, &year) != 3) {
    fprintf(stderr, "Error: Invalid date format. Expected DD.MM.YYYY\n");
    return 1;
  }

  struct tm date = {0};
  date.tm_mday = day;
  date.tm_mon = month - 1;       // tm_mon is 0-based
  date.tm_year = year - 1900;    // tm_year is years since 1900
  date.tm_hour = 12;             // avoid DST ambiguity

  if (mktime(&date) == (time_t)(-1)) {
    fprintf(stderr, "Error: mktime failed\n");
    return 1;
  }

  static const char* weekdays[] = {
    "Sunday", "Monday", "Tuesday", "Wednesday",
    "Thursday", "Friday", "Saturday"
  };

  if (date.tm_wday < 0 || date.tm_wday > 6) {
    fprintf(stderr, "Error: Invalid weekday\n");
    return 1;
  }

  const char* weekday_name = weekdays[date.tm_wday];
  char* result = strdup(weekday_name);
  if (!result) {
    fprintf(stderr, "Error: Memory allocation failed\n");
    return 1;
  }

  stack_element out;
  out.type = TYPE_STRING;
  out.string = result;

  if (stack->top >= STACK_SIZE - 1) {
    fprintf(stderr, "Error: Stack overflow\n");
    free(result);
    return 1;
  }

  stack->items[++stack->top] = out;
  return 0;
}



int push_today_date(Stack* stack) {
  time_t now = time(NULL);
  if (now == (time_t)(-1)) {
    fprintf(stderr, "Error: could not get current time\n");
    return 1;
  }

  struct tm* tm_now = localtime(&now);
  if (!tm_now) {
    fprintf(stderr, "Error: could not convert time to local time\n");
    return 1;
  }

  char buffer[64];  // enough for "DD.MM.YYYY\0"
  snprintf(buffer, sizeof(buffer), "%02d.%02d.%04d",
	   tm_now->tm_mday,
	   tm_now->tm_mon + 1,
	   tm_now->tm_year + 1900);

  // Duplicate the string before pushing
  char* date_str = strdup(buffer);
  if (!date_str) {
    fprintf(stderr, "Error: memory allocation failed\n");
    return 1;
  }

  stack_element elem;
  elem.type = TYPE_STRING;
  elem.string = date_str;

  if (stack->top >= STACK_SIZE - 1) {
    fprintf(stderr, "Error: stack overflow\n");
    free(date_str);
    return 1;
  }

  stack->items[++stack->top] = elem;
  return 0;
}

/* ---- helpers ---- */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

/* Helper: Leap Year Check */
static int is_leap_year(int y) {
    return (y % 4 == 0) && ((y % 100 != 0) || (y % 400 == 0));
}

/* Helper: Days in Month */
static int days_in_month(int y, int m) {
    static const int dim[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
    if (m == 2) return 28 + is_leap_year(y);
    if (m >= 1 && m <= 12) return dim[m - 1];
    return 0;
}

/* * Relaxed parser with Trailing Check:
 * - Allows "2.3.2025", "02.03.2025"
 * - Rejects "2.3.2025abc" (trailing junk)
 * - Rejects "2.3.2025 " (trailing space)
 */
int parse_date_relaxed(const char *s, int *d, int *m, int *y) {
    if (!s) return 0;
    
    int consumed = 0;

    // %d.%d.%d reads the numbers
    // %n stores the count of characters read so far into 'consumed'
    if (sscanf(s, "%d.%d.%d%n", d, m, y, &consumed) != 3) {
        return 0;
    }

    // Check for trailing characters
    // If consumed length != string length, there is extra junk
    if (s[consumed] != '\0') {
        return 0;
    }

    // Validate Month
    if (*m < 1 || *m > 12) return 0;

    // Validate Day based on Month/Year
    int dim = days_in_month(*y, *m);
    if (*d < 1 || *d > dim) return 0;

    // Optional: Year sanity check
    if (*y < 1900) return 0;

    return 1;
}

int delta_days_strings(Stack* stack) {
    if (stack->top < 1) {
        fprintf(stderr, "Error: delta_days requires two values on the stack\n");
        return 1;
    }

    stack_element b = stack->items[stack->top--];
    stack_element a = stack->items[stack->top--];

    if (a.type != TYPE_STRING || b.type != TYPE_STRING) {
        fprintf(stderr, "Error: delta_days requires two strings\n");
        return 1;
    }

    int d1, m1, y1, d2, m2, y2;
    
    if (!parse_date_relaxed(a.string, &d1, &m1, &y1) ||
        !parse_date_relaxed(b.string, &d2, &m2, &y2)) {
        fprintf(stderr, "Error: invalid date format. Use D.M.YYYY (e.g., 2.3.2025) with no trailing text.\n");
        return 1;
    }

    struct tm tm1 = {0}, tm2 = {0};
    
    tm1.tm_mday = d1;
    tm1.tm_mon  = m1 - 1;
    tm1.tm_year = y1 - 1900;
    tm1.tm_hour = 12;
    tm1.tm_isdst = -1;

    tm2.tm_mday = d2;
    tm2.tm_mon  = m2 - 1;
    tm2.tm_year = y2 - 1900;
    tm2.tm_hour = 12;
    tm2.tm_isdst = -1;

    time_t t1 = mktime(&tm1);
    time_t t2 = mktime(&tm2);

    if (t1 == (time_t)-1 || t2 == (time_t)-1) {
        fprintf(stderr, "Error: failed to convert date to time_t\n");
        return 1;
    }

    int days = (int)(difftime(t2, t1) / (60 * 60 * 24));
    push_real(stack, (double)days);

    return 0;
}

/* static int is_leap_year(int y) { */
/*   return (y % 4 == 0) && ((y % 100 != 0) || (y % 400 == 0)); */
/* } */

/* static int days_in_month(int y, int m) { */
/*   static const int dim[12] = {31,28,31,30,31,30,31,31,30,31,30,31}; */
/*   if (m == 2) return 28 + is_leap_year(y); */
/*   if (m >= 1 && m <= 12) return dim[m - 1]; */
/*   return 0; */
/* } */

/* /\* strict "DD.MM.YYYY" parser: exactly 10 chars, digits except dots at 2 and 5 *\/ */
/* static int parse_date_strict(const char *s, int *d, int *m, int *y) { */
/*   if (!s || strlen(s) != 10) return 0; */
/*   for (int i = 0; i < 10; ++i) { */
/*     if (i == 2 || i == 5) { */
/*       if (s[i] != '.') return 0; */
/*     } else if (!isdigit((unsigned char)s[i])) { */
/*       return 0; */
/*     } */
/*   } */
/*   // two-digit day, two-digit month, four-digit year */
/*   *d = (s[0]-'0')*10 + (s[1]-'0'); */
/*   *m = (s[3]-'0')*10 + (s[4]-'0'); */
/*   *y = (s[6]-'0')*1000 + (s[7]-'0')*100 + (s[8]-'0')*10 + (s[9]-'0'); */

/*   if (*m < 1 || *m > 12) return 0; */
/*   int dim = days_in_month(*y, *m); */
/*   if (*d < 1 || *d > dim) return 0; */

/*   return 1; */
/* } */

/* int delta_days_strings(Stack* stack) { */
/*   if (stack->top < 1) { */
/*     fprintf(stderr, "Error: delta_days requires two values on the stack\n"); */
/*     return 1; */
/*   } */

/*   stack_element b = stack->items[stack->top--]; */
/*   stack_element a = stack->items[stack->top--]; */

/*   if (a.type != TYPE_STRING || b.type != TYPE_STRING) { */
/*     fprintf(stderr, "Error: delta_days requires two strings in DD.MM.YYYY format\n"); */
/*     return 1; */
/*   } */

/*   int d1, m1, y1, d2, m2, y2; */
/*   if (!parse_date_strict(a.string, &d1, &m1, &y1) || */
/*       !parse_date_strict(b.string, &d2, &m2, &y2)) { */
/*     fprintf(stderr, "Error: invalid date. Use DD.MM.YYYY and a real calendar date\n"); */
/*     return 1; */
/*   } */

/*   struct tm tm1 = {0}, tm2 = {0}; */
/*   tm1.tm_mday = d1; */
/*   tm1.tm_mon  = m1 - 1;       // 0..11 */
/*   tm1.tm_year = y1 - 1900;    // years since 1900 */
/*   tm1.tm_hour = 12;           // avoid DST edge cases */
/*   tm1.tm_isdst = -1;          // let libc determine */

/*   tm2.tm_mday = d2; */
/*   tm2.tm_mon  = m2 - 1; */
/*   tm2.tm_year = y2 - 1900; */
/*   tm2.tm_hour = 12; */
/*   tm2.tm_isdst = -1; */

/*   time_t t1 = mktime(&tm1); */
/*   time_t t2 = mktime(&tm2); */

/*   if (t1 == (time_t)-1 || t2 == (time_t)-1) { */
/*     fprintf(stderr, "Error: failed to convert date to time_t\n"); */
/*     return 1; */
/*   } */

/*   /\* Convert to whole days (truncates toward zero; adjust if you want rounding) *\/ */
/*   int days = (int)(difftime(t2, t1) / (60 * 60 * 24)); */
/*   push_real(stack, (double)days);  // or push_int if you have it */

/*   return 0; */
/* } */

/* ---- function: build string "DD.MM.YYYY" from three numbers ---- */
/* Accept TYPE_INT or TYPE_REAL that is integral. Adjust to your real type names. */
static int elem_to_int(const stack_element *e, int *out) {
  if (e->type == TYPE_REAL) {
    double v = e->real; 
    if (isfinite(v) && floor(v) == v &&
        v >= (double)INT_MIN && v <= (double)INT_MAX) {
      *out = (int)v;
      return 1;
    }
  }
  return 0;
}

/* int make_date_string(Stack* stack) { */
/*   if (stack->top < 2) { */
/*     fprintf(stderr, "Error: make_date_string requires three numeric values (day, month, year)\n"); */
/*     return 1; */
/*   } */

/*   /\* Pop in reverse: year (top), month, day *\/ */
/*   stack_element ed = stack->items[stack->top--]; */
/*   stack_element em = stack->items[stack->top--]; */
/*   stack_element ey = stack->items[stack->top--]; */

/*   int d, m, y; */
/*   if (!elem_to_int(&ed, &d) || !elem_to_int(&em, &m) || !elem_to_int(&ey, &y)) { */
/*     fprintf(stderr, "Error: make_date_string requires integer-like numbers (day, month, year)\n"); */
/*     return 1; */
/*   } */

/*   /\* Basic sanity checks (tweak year bounds as you like) *\/ */
/*   if (m < 1 || m > 12) { */
/*     fprintf(stderr, "Error: invalid month: %d (must be 1..12)\n", m); */
/*     return 1; */
/*   } */
/*   int dim = days_in_month(y, m); */
/*   if (d < 1 || d > dim) { */
/*     fprintf(stderr, "Error: invalid day %d for %02d.%04d (max %d)\n", d, m, y, dim); */
/*     return 1; */
/*   } */

/*   /\* Optional: normalize with mktime to catch exotic calendar issues/DST */
/*      Not strictly required since we already validated the calendar date. *\/ */
/*   struct tm tmv = {0}; */
/*   tmv.tm_mday = d; */
/*   tmv.tm_mon  = m - 1; */
/*   tmv.tm_year = y - 1900; */
/*   tmv.tm_hour = 12; */
/*   tmv.tm_isdst = -1; */

/*   if (mktime(&tmv) == (time_t)-1) { */
/*     fprintf(stderr, "Error: failed to normalize date\n"); */
/*     return 1; */
/*   } */

/*   /\* Format "DD.MM.YYYY" *\/ */
/*   char buf[11]; /\* "DD.MM.YYYY" + '\0' *\/ */
/*   snprintf(buf, sizeof(buf), "%02d.%02d.%04d", d, m, y); */

/*   /\* Push as string (assumes push_string copies) *\/ */
/*   push_string(stack, buf); */

/*   return 0; */
/* } */

int make_date_string(Stack *stack)
{
    if (!stack) {
        fprintf(stderr, "Error: make_date_string: null stack\n");
        return 0;
    }

    GUARANTEE_STACK(stack, 3);

    /* Stack order expected: ... day month year (year on top) */
    stack_element ey = pop(stack);
    stack_element em = pop(stack);
    stack_element ed = pop(stack);

    int d = 0, m = 0, y = 0;
    if (!elem_to_int(&ed, &d) || !elem_to_int(&em, &m) || !elem_to_int(&ey, &y)) {
        fprintf(stderr, "Error: make_date_string requires integer-like numbers (day, month, year)\n");
        return 0;
    }

    if (m < 1 || m > 12) {
        fprintf(stderr, "Error: invalid month: %d (must be 1..12)\n", m);
        return 0;
    }

    int dim = days_in_month(y, m);
    if (d < 1 || d > dim) {
        fprintf(stderr, "Error: invalid day %d for %02d.%04d (max %d)\n", d, m, y, dim);
        return 0;
    }

    /* Optional normalization check */
    struct tm tmv = {0};
    tmv.tm_mday  = d;
    tmv.tm_mon   = m - 1;
    tmv.tm_year  = y - 1900;
    tmv.tm_hour  = 12;
    tmv.tm_isdst = -1;

    if (mktime(&tmv) == (time_t)-1) {
        fprintf(stderr, "Error: failed to normalize date\n");
        return 0;
    }

    char buf[11]; /* "DD.MM.YYYY" + '\0' */
    int n = snprintf(buf, sizeof(buf), "%02d.%02d.%04d", d, m, y);
    if (n < 0 || (size_t)n >= sizeof(buf)) {
        fprintf(stderr, "Error: date formatting failed\n");
        return 0;
    }

    push_string(stack, buf);
    return 1;
}


/* Pushes the count of days left in the current year (excludes today). */
int days_to_end_of_year(Stack* stack) {
  time_t now = time(NULL);
  if (now == (time_t)-1) {
    fprintf(stderr, "Error: failed to get current time\n");
    return 1;
  }

  struct tm lt;
#if defined(_POSIX_THREAD_SAFE_FUNCTIONS)
  if (!localtime_r(&now, &lt)) {
    fprintf(stderr, "Error: localtime_r failed\n");
    return 1;
  }
#else
  struct tm *ptm = localtime(&now);
  if (!ptm) {
    fprintf(stderr, "Error: localtime failed\n");
    return 1;
  }
  lt = *ptm;
#endif

  int year = lt.tm_year + 1900;
  int year_len = is_leap_year(year) ? 366 : 365;

  /* tm_yday is 0-based (Jan 1 = 0). Remaining days AFTER today: */
  int remaining = year_len - 1 - lt.tm_yday;

  if (remaining < 0) remaining = 0; /* ultra-defensive */

  push_real(stack, (double)remaining); /* or push_int if available */
  return 0;
}

/* ---------------------------------------------------------------------
 * now  ( -- "HH:MM" )
 * Push the current local wall-clock time as a string, in the same HH:MM
 * form that sunrise/sunset/dawn/dusk produce, so the same string helpers
 * (e.g. the dec_hr macro) work on both.
 * ------------------------------------------------------------------- */
int push_now_time(Stack* stack) {
  time_t now = time(NULL);
  if (now == (time_t)(-1)) {
    fprintf(stderr, "Error: could not get current time\n");
    return 1;
  }
  struct tm tm_now;
  if (!localtime_r(&now, &tm_now)) {
    fprintf(stderr, "Error: could not convert time to local time\n");
    return 1;
  }
  char buffer[8]; /* "HH:MM" + NUL */
  snprintf(buffer, sizeof buffer, "%02d:%02d", tm_now.tm_hour, tm_now.tm_min);
  push_string(stack, buffer);
  return 0;
}

/* ---------------------------------------------------------------------
 * zone_offset_hours
 *
 * Compute the UTC offset, in hours, that IANA zone `zone` (e.g.
 * "America/New_York") is on at local noon on d.m.y.  Uses the system
 * tzdata through TZ/tzset/mktime, so the DST rule in force on that date
 * is honoured.  The process TZ is saved and restored.
 *
 * Returns 1 and sets *hours_out on success; 0 (with a message on stderr)
 * on failure.  Noon is used so the instant can never fall inside a
 * spring-forward gap.
 *
 * Portability: relies only on POSIX (setenv/unsetenv/tzset/localtime_r/
 * gmtime_r/stat).  The offset is derived by differencing the local and
 * UTC broken-down times rather than reading tm_gmtoff, which is a BSD/
 * glibc extension not visible under _POSIX_C_SOURCE.
 * ------------------------------------------------------------------- */
int zone_offset_hours(const char *zone, int d, int m, int y, double *hours_out) {
  if (!zone || !*zone || !hours_out) return 0;

  /* libc silently treats an unknown TZ as UTC, so validate against the
   * zoneinfo directory ourselves.  Also refuse anything that could walk
   * out of that directory. */
  if (zone[0] == '/' || strstr(zone, "..") != NULL || strchr(zone, ':') != NULL) {
    fprintf(stderr, "Error: invalid time zone name \"%s\"\n", zone);
    return 0;
  }
  char path[512];
  int n = snprintf(path, sizeof path, "/usr/share/zoneinfo/%s", zone);
  if (n < 0 || (size_t)n >= sizeof path) {
    fprintf(stderr, "Error: time zone name too long\n");
    return 0;
  }
  struct stat st;
  if (stat(path, &st) != 0 || !S_ISREG(st.st_mode)) {
    fprintf(stderr,
            "Error: unknown time zone \"%s\" (expected an IANA name such as \"Europe/Ljubljana\")\n",
            zone);
    return 0;
  }

  /* Save the caller's TZ so we can restore it exactly (set vs. unset). */
  const char *old_tz = getenv("TZ");
  char *saved_tz = NULL;
  if (old_tz) {
    saved_tz = strdup(old_tz);
    if (!saved_tz) {
      fprintf(stderr, "Error: memory allocation failed\n");
      return 0;
    }
  }

  int ok = 0;
  double hours = 0.0;

  if (setenv("TZ", zone, 1) == 0) {
    tzset();

    struct tm lt;
    memset(&lt, 0, sizeof lt);
    lt.tm_mday  = d;
    lt.tm_mon   = m - 1;
    lt.tm_year  = y - 1900;
    lt.tm_hour  = 12;
    lt.tm_isdst = -1;

    time_t t = mktime(&lt);
    if (t != (time_t)-1) {
      struct tm loc, utc;
      if (localtime_r(&t, &loc) && gmtime_r(&t, &utc)) {
        long days;
        if      (loc.tm_year > utc.tm_year) days =  1;
        else if (loc.tm_year < utc.tm_year) days = -1;
        else                                days = loc.tm_yday - utc.tm_yday;
        long secs = days * 86400L
                  + (long)(loc.tm_hour - utc.tm_hour) * 3600L
                  + (long)(loc.tm_min  - utc.tm_min ) * 60L
                  + (long)(loc.tm_sec  - utc.tm_sec );
        hours = (double)secs / 3600.0;
        ok = 1;
      }
    }
  }

  /* Restore TZ before doing anything else. */
  if (saved_tz) {
    setenv("TZ", saved_tz, 1);
    free(saved_tz);
  } else {
    unsetenv("TZ");
  }
  tzset();

  if (!ok) {
    fprintf(stderr, "Error: could not compute the offset for \"%s\" on %d.%d.%d\n",
            zone, d, m, y);
    return 0;
  }
  *hours_out = hours;
  return 1;
}

/* ---------------------------------------------------------------------
 * tz_offset  ( "d.m.y" "Area/City" -- hours )
 * Push the UTC offset that the zone is on at local noon on the given date.
 * ------------------------------------------------------------------- */
int tz_offset(Stack* stack) {
  if (stack->top < 1) {
    fprintf(stderr, "Error: tz_offset needs a date string and a zone name on the stack\n");
    return 1;
  }
  const stack_element *zone_e = &stack->items[stack->top];
  const stack_element *date_e = &stack->items[stack->top - 1];

  if (zone_e->type != TYPE_STRING || date_e->type != TYPE_STRING) {
    fprintf(stderr, "Error: tz_offset expects \"d.m.y\" \"Area/City\" (two strings)\n");
    return 1;
  }

  int d, m, y;
  if (!parse_date_relaxed(date_e->string, &d, &m, &y)) {
    fprintf(stderr, "Error: invalid date \"%s\" (expected \"d.m.y\", e.g. \"3.4.2025\")\n",
            date_e->string);
    return 1;
  }

  double hours;
  if (!zone_offset_hours(zone_e->string, d, m, y, &hours))
    return 1; /* message already printed; stack untouched */

  pop_and_free(stack); /* zone */
  pop_and_free(stack); /* date */
  push_real(stack, hours);
  return 0;
}
