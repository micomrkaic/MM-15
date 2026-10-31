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

#ifndef ERRORS_H
#define ERRORS_H

/* The single error channel.
 *
 * mm_error() prints the message on stderr (prefixed with "Error: " unless
 * the message already starts with it), and bumps error_count. The
 * interpreter compares error_count before and after each token: if it
 * grew, the stack is rolled back to its state before that token and the
 * rest of the line is skipped. In batch mode a nonzero count at exit
 * gives exit status 1.
 *
 * mm_warn() prints "Warning: ..." and does not count. */
int  mm_error(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
void mm_warn(const char *fmt, ...)  __attribute__((format(printf, 1, 2)));
extern long error_count;

#include <errno.h>
#include <string.h>
#define mm_perror(what) mm_error("%s: %s", (what), strerror(errno))

#endif
