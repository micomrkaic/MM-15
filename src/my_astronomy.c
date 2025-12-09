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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "stack.h"
#include "my_astronomy.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* --- basic helpers --- */

static double deg2rad(double deg) { return deg * M_PI / 180.0; }
static double rad2deg(double rad) { return rad * 180.0 / M_PI; }

static double normalize_deg(double x) {
  while (x < 0.0)   x += 360.0;
  while (x >= 360.0) x -= 360.0;
  return x;
}

static double normalize_hours(double h) {
  while (h < 0.0)   h += 24.0;
  while (h >= 24.0) h -= 24.0;
  return h;
}

/*
 * Parse date of the form "d.m.y" or "dd.mm.yyyy", e.g.:
 *   "3.4.2025", "03.04.2025", "31.12.2025"
 * Returns day-of-year (1..365/366) on success, -1 on error.
 */
static int day_of_year(const char *date_str, int *year_out) {
  int y, m, d;
  if (sscanf(date_str, "%d.%d.%d", &d, &m, &y) != 3)
    return -1;

  static const int mdays_norm[12] = {
    31,28,31,30,31,30,31,31,30,31,30,31
  };
  int mdays[12];
  memcpy(mdays, mdays_norm, sizeof(mdays_norm));

  int leap = ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0));
  if (leap) mdays[1] = 29;

  if (m < 1 || m > 12) return -1;
  if (d < 1 || d > mdays[m-1]) return -1;

  int n = d;
  for (int i = 0; i < m - 1; ++i)
    n += mdays[i];

  if (year_out) *year_out = y;
  return n;
}

/*
 * General solar event:
 *   kind = 0 → "rising" event (sunrise, dawn)
 *   kind = 1 → "setting" event (sunset, dusk)
 *   zenith_deg: geometric zenith angle (e.g. 90.833 for sunrise/sunset,
 *               96.0 for civil twilight).
 *
 * Returns:
 *   0  = success
 *  -1  = invalid date
 *  -2  = event never occurs (sun never gets high enough)
 *  -3  = event never occurs (sun never gets low enough)
 */
static int compute_solar_event(const char *date_str,
                               double latitude_deg,
                               double longitude_deg,
                               double utc_offset_h,
                               int kind,
                               double zenith_deg,
                               double *local_time_out)
{
  int year;
  int N = day_of_year(date_str, &year);
  if (N < 0) return -1;  /* invalid date */

  double lat_rad = deg2rad(latitude_deg);
  double sinLat  = sin(lat_rad);
  double cosLat  = cos(lat_rad);

  double lngHour = longitude_deg / 15.0;

  /* initial guess: 6h for rise-like events, 18h for set-like events */
  double base_hour = (kind == 0) ? 6.0 : 18.0;
  double t = N + ((base_hour - lngHour) / 24.0);

  /* Sun's mean anomaly */
  double M = (0.9856 * t) - 3.289;

  /* Sun's true longitude */
  double L = M
    + (1.916 * sin(deg2rad(M)))
    + (0.020 * sin(deg2rad(2.0 * M)))
    + 282.634;
  L = normalize_deg(L);

  /* Sun's right ascension */
  double RA = rad2deg(atan(0.91764 * tan(deg2rad(L))));
  RA = normalize_deg(RA);

  /* Adjust RA to be in same quadrant as L */
  double Lquadrant  = floor(L / 90.0) * 90.0;
  double RAquadrant = floor(RA / 90.0) * 90.0;
  RA = RA + (Lquadrant - RAquadrant);
  RA /= 15.0;  /* hours */

  /* Sun declination */
  double sinDec = 0.39782 * sin(deg2rad(L));
  double cosDec = cos(asin(sinDec));

  /* local hour angle */
  double cosH =
    (cos(deg2rad(zenith_deg)) - sinDec * sinLat) /
    (cosDec * cosLat);

  /* numeric safety: clamp tiny overshoots into [-1, 1] */
  if (cosH >  1.0 && cosH < 1.0 + 1e-12) cosH = 1.0;
  if (cosH < -1.0 && cosH > -1.0 - 1e-12) cosH = -1.0;

  if (cosH >  1.0) return -2; /* event never occurs (sun never that high) */
  if (cosH < -1.0) return -3; /* event never occurs (sun never that low) */

  double H;
  if (kind == 0) {
    /* rise-like event */
    H = 360.0 - rad2deg(acos(cosH));
  } else {
    /* set-like event */
    H = rad2deg(acos(cosH));
  }
  H /= 15.0; /* hours */

  double T  = H + RA - (0.06571 * t) - 6.622;
  double UT = T - lngHour;
  UT = normalize_hours(UT);

  double local_time = UT + utc_offset_h;
  local_time = normalize_hours(local_time);

  *local_time_out = local_time;
  return 0;
}

/* "Official" sunrise/sunset – Sun center at -0.833° altitude → zenith 90.833° */
static int compute_one_sun_time(const char *date_str,
                                double latitude_deg,
                                double longitude_deg,
                                double utc_offset_h,
                                int kind,
                                double *local_time_out)
{
  const double zenith_deg = 90.833; /* official sunrise/sunset */
  return compute_solar_event(date_str, latitude_deg, longitude_deg,
                             utc_offset_h, kind, zenith_deg,
                             local_time_out);
}

/* Civil twilight: Sun center at -6° altitude → zenith 96° */
static int compute_one_civil_twilight(const char *date_str,
                                      double latitude_deg,
                                      double longitude_deg,
                                      double utc_offset_h,
                                      int kind,
                                      double *local_time_out)
{
  const double civil_zenith_deg = 96.0; /* 90° + 6° */
  return compute_solar_event(date_str, latitude_deg, longitude_deg,
                             utc_offset_h, kind, civil_zenith_deg,
                             local_time_out);
}

static void format_time_hhmm(double h, char *buf, size_t len) {
  int hour   = (int)floor(h);
  int minute = (int)floor((h - hour) * 60.0 + 0.5);
  if (minute >= 60) {
    minute -= 60;
    hour = (hour + 1) % 24;
  }
  snprintf(buf, len, "%02d:%02d", hour, minute);
}

/* --- stack entry points --- */

/*
 * Common helper: check & decode top 4 stack items, without popping.
 *
 * Expects on stack (from bottom to top):
 *   ... "d.m.y" lat lon utc_offset
 *
 * where the date string is like "3.4.2025" or "03.04.2025".
 */
static int fetch_astro_args(const Stack *stack,
                            const char **date_str_out,
                            double *lat_out,
                            double *lon_out,
                            double *utc_offset_out)
{
  GUARANTEE_STACK(stack, 4);

  int idx = stack->top; /* top index */

  const stack_element *e_utc  = &stack->items[idx];
  const stack_element *e_lon  = &stack->items[idx - 1];
  const stack_element *e_lat  = &stack->items[idx - 2];
  const stack_element *e_date = &stack->items[idx - 3];

  if (e_date->type != TYPE_STRING) {
    fprintf(stderr,
            "Error: date must be a string \"d.m.y\" (e.g. \"3.4.2025\" or \"03.04.2025\").\n");
    return 0;
  }
  if (e_lat->type != TYPE_REAL ||
      e_lon->type != TYPE_REAL ||
      e_utc->type != TYPE_REAL) {
    fprintf(stderr,
            "Error: latitude, longitude, and UTC offset must be real numbers.\n");
    return 0;
  }

  *date_str_out   = e_date->string;
  *lat_out        = e_lat->real;
  *lon_out        = e_lon->real;
  *utc_offset_out = e_utc->real;

  return 1;
}

/*
 * SUNRISE word:
 *   ... "d.m.y" lat lon utc_offset  SUNRISE
 *   -> ... "HH:MM"
 */
int sunrise(Stack *stack)
{
  const char *date_str;
  double lat, lon, utc_offset;

  if (!fetch_astro_args(stack, &date_str, &lat, &lon, &utc_offset))
    return 0; /* error already printed */

  /* Now safely pop the 4 arguments */
  (void)pop(stack); /* utc_offset */
  (void)pop(stack); /* lon */
  (void)pop(stack); /* lat */
  (void)pop(stack); /* date string */

  double sun_h;
  int rc = compute_one_sun_time(date_str, lat, lon, utc_offset, 0, &sun_h);
  if (rc == -1) {
    fprintf(stderr,
            "Error: invalid date \"%s\" (expected \"d.m.y\", e.g. \"3.4.2025\").\n",
            date_str);
    return 0;
  } else if (rc == -2) {
    fprintf(stderr,
            "Error: sun never rises at this location on this date (polar night).\n");
    return 0;
  } else if (rc == -3) {
    fprintf(stderr,
            "Error: sun never sets at this location on this date (midnight sun).\n");
    return 0;
  }

  char buf[8];
  format_time_hhmm(sun_h, buf, sizeof(buf));
  push_string(stack, buf);

  return 1;
}

/*
 * SUNSET word:
 *   ... "d.m.y" lat lon utc_offset  SUNSET
 *   -> ... "HH:MM"
 */
int sunset(Stack *stack)
{
  const char *date_str;
  double lat, lon, utc_offset;

  if (!fetch_astro_args(stack, &date_str, &lat, &lon, &utc_offset))
    return 0; /* error already printed */

  /* Now safely pop the 4 arguments */
  (void)pop(stack); /* utc_offset */
  (void)pop(stack); /* lon */
  (void)pop(stack); /* lat */
  (void)pop(stack); /* date string */

  double sun_h;
  int rc = compute_one_sun_time(date_str, lat, lon, utc_offset, 1, &sun_h);
  if (rc == -1) {
    fprintf(stderr,
            "Error: invalid date \"%s\" (expected \"d.m.y\", e.g. \"3.4.2025\").\n",
            date_str);
    return 0;
  } else if (rc == -2) {
    fprintf(stderr,
            "Error: sun never rises at this location on this date (polar night).\n");
    return 0;
  } else if (rc == -3) {
    fprintf(stderr,
            "Error: sun never sets at this location on this date (midnight sun).\n");
    return 0;
  }

  char buf[8];
  format_time_hhmm(sun_h, buf, sizeof(buf));
  push_string(stack, buf);

  return 1;
}

/*
 * DAWN word (civil dawn, sun at -6°):
 *   ... "d.m.y" lat lon utc_offset  DAWN
 *   -> ... "HH:MM"
 */
int dawn(Stack *stack)
{
  const char *date_str;
  double lat, lon, utc_offset;

  if (!fetch_astro_args(stack, &date_str, &lat, &lon, &utc_offset))
    return 0; /* error already printed */

  /* Pop the 4 arguments */
  (void)pop(stack); /* utc_offset */
  (void)pop(stack); /* lon */
  (void)pop(stack); /* lat */
  (void)pop(stack); /* date string */

  double t_h;
  int rc = compute_one_civil_twilight(date_str, lat, lon, utc_offset, 0, &t_h);
  if (rc == -1) {
    fprintf(stderr,
            "Error: invalid date \"%s\" (expected \"d.m.y\", e.g. \"3.4.2025\").\n",
            date_str);
    return 0;
  } else if (rc == -2 || rc == -3) {
    fprintf(stderr,
            "Error: civil dawn does not occur at this location on this date.\n");
    return 0;
  }

  char buf[8];
  format_time_hhmm(t_h, buf, sizeof(buf));
  push_string(stack, buf);
  return 1;
}

/*
 * DUSK word (civil dusk, sun at -6°):
 *   ... "d.m.y" lat lon utc_offset  DUSK
 *   -> ... "HH:MM"
 */
int dusk(Stack *stack)
{
  const char *date_str;
  double lat, lon, utc_offset;

  if (!fetch_astro_args(stack, &date_str, &lat, &lon, &utc_offset))
    return 0; /* error already printed */

  /* Pop the 4 arguments */
  (void)pop(stack); /* utc_offset */
  (void)pop(stack); /* lon */
  (void)pop(stack); /* lat */
  (void)pop(stack); /* date string */

  double t_h;
  int rc = compute_one_civil_twilight(date_str, lat, lon, utc_offset, 1, &t_h);
  if (rc == -1) {
    fprintf(stderr,
            "Error: invalid date \"%s\" (expected \"d.m.y\", e.g. \"3.4.2025\").\n",
            date_str);
    return 0;
  } else if (rc == -2 || rc == -3) {
    fprintf(stderr,
            "Error: civil dusk does not occur at this location on this date.\n");
    return 0;
  }

  char buf[8];
  format_time_hhmm(t_h, buf, sizeof(buf));
  push_string(stack, buf);
  return 1;
}
