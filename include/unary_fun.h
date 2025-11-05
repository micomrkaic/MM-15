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

#ifndef UNARY_FUN_H
#define UNARY_FUN_H

#include <complex.h>
#include "stack.h"

void apply_real_unary(Stack* stack, double (*func)(double));
void apply_complex_unary(Stack* stack, gsl_complex (*func)(gsl_complex));
void apply_complex_matrix_unary_inplace(Stack* stack, gsl_complex (*func)(gsl_complex));
void apply_real_matrix_unary_inplace(Stack* stack, double (*func)(double));
void complex_matrix_real_part(Stack *s);
void complex_matrix_imag_part(Stack *s);
void complex_matrix_abs_by_element(Stack *s);
void real2complex(Stack *s);
void split_complex(Stack *s);
#endif // UNARY_FUN_H
