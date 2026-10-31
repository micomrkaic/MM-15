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

#include "errors.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

long error_count = 0;

int mm_error(const char *fmt, ...) {
  va_list ap;
  if (!fmt) fmt = "";          /* newer gcc otherwise warns "null format string" */
  fflush(stdout);
  if (strncmp(fmt, "Error", 5) != 0 && strncmp(fmt, "GSL", 3) != 0) fputs("Error: ", stderr);
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  size_t n = strlen(fmt);
  if (n == 0 || fmt[n - 1] != '\n') fputc('\n', stderr);
  error_count++;
  return -1;
}

void mm_warn(const char *fmt, ...) {
  va_list ap;
  if (!fmt) fmt = "";
  fflush(stdout);
  if (strncmp(fmt, "Warning", 7) != 0) fputs("Warning: ", stderr);
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  size_t n = strlen(fmt);
  if (n == 0 || fmt[n - 1] != '\n') fputc('\n', stderr);
}
