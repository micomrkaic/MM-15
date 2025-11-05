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

#ifndef MATH_HELPERS_H
#define MATH_HELPERS_H

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include "stack.h"

gsl_complex my_complex_asin(gsl_complex z);
gsl_complex my_complex_acos(gsl_complex z);
gsl_complex my_complex_atan(gsl_complex z);
gsl_complex my_complex_asinh(gsl_complex z);
gsl_complex my_complex_acosh(gsl_complex z);
gsl_complex my_complex_atanh(gsl_complex z);

double safe_frac(double a);
double safe_int(double a);
gsl_complex safe_frac_complex(gsl_complex z);
gsl_complex safe_int_complex(gsl_complex z);
double negate_real(double x);
gsl_complex negate_complex(gsl_complex x);
double log10_real(double x);
gsl_complex log10_complex(gsl_complex z);
double one_over_real(double x);
gsl_complex one_over_complex(gsl_complex x);
gsl_complex log10_complex_not_gsl(gsl_complex z);
gsl_complex to_gsl_complex(double complex z);
double complex to_double_complex(gsl_complex z);
bool is_zero_complex(gsl_complex z);

// ****************************************************************
// Wrappers
// ****************************************************************

void sin_wrapper(Stack* stack);
void cos_wrapper(Stack* stack);
void tan_wrapper(Stack* stack);
void asin_wrapper(Stack* stack);
void acos_wrapper(Stack* stack);
void atan_wrapper(Stack* stack);
void sinh_wrapper(Stack* stack);
void cosh_wrapper(Stack* stack);
void tanh_wrapper(Stack* stack);
void asinh_wrapper(Stack* stack);
void acosh_wrapper(Stack* stack);
void atanh_wrapper(Stack* stack);
void exp_wrapper(Stack* stack);
void chs_wrapper(Stack* stack);
void inv_wrapper(Stack* stack);
void frac_wrapper(Stack* stack);
void intg_wrapper(Stack* stack);

void logical_not_wrapper(Stack* stack);

void im_wrapper(Stack* stack);
void re_wrapper(Stack* stack);
void abs_wrapper(Stack* stack);
void arg_wrapper(Stack* stack);
void ln_wrapper(Stack* stack);
void log_wrapper(Stack* stack);
void sqrt_wrapper(Stack* stack);
void npdf_wrapper(Stack *stack);
void ncdf_wrapper(Stack *stack);
void nquant_wrapper(Stack *stack);

void gamma_wrapper(Stack *stack);
void ln_gamma_wrapper(Stack *stack);
void beta_wrapper(Stack *stack);
void ln_beta_wrapper(Stack *stack);

#endif // MATH_HELPERS_H
