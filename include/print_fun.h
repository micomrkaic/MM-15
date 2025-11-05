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

#ifndef PRINT_FUN_H
#define PRINT_FUN_H

#include <unistd.h>
#include <stdarg.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_complex_math.h>
#include "stack.h"
#include "globals.h"

static inline void cerror(const char* fmt, ...) {
    if (isatty(fileno(stderr))) fprintf(stderr, "\033[1;31m");
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    if (isatty(fileno(stderr))) fprintf(stderr, "\033[0m");
}

void print_top_scalar(const Stack* stack);
void print_matrix(Stack *stack);
void print_stack(const Stack* stack, char *title);
void print_real_matrix(const gsl_matrix* m);
void print_complex_matrix(const gsl_matrix_complex* m);

#endif // PRINT_FUN_H
