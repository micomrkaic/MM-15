/*
 * This file is part of Mico's toy RPN Calculator
 *
 * Mico's toy RPN Calculator is free software: 
 * you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Mico's toy RPN Calculator is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Mico's toy RPN Calculator. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef POLY_FUN_H
#define POLY_FUN_H

#include <stdio.h>                // for fprintf, perror
#include <stdlib.h>               // for malloc, free
#include <math.h>                 // for general math if needed
#include <complex.h>              // for C complex numbers

#include <gsl/gsl_math.h>         // GSL math types and macros
#include <gsl/gsl_complex.h>      // gsl_complex types
#include <gsl/gsl_complex_math.h> // gsl_complex arithmetic
#include <gsl/gsl_matrix.h>       // gsl_matrix for real matrices
#include <gsl/gsl_poly.h>         // for gsl_poly_complex_solve


void poly_eval(Stack* stack);
void poly_roots(Stack* stack);

#endif // POLY_FUN_H
