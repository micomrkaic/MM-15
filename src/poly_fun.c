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
#include <gsl/gsl_poly.h>
#include <gsl/gsl_errno.h>
#include <stdio.h>                // for fprintf, perror
#include <stdlib.h>               // for malloc, free
#include <math.h>                 // for general math if needed
#include <complex.h>              // for C complex numbers
#include <stdbool.h>
#include <gsl/gsl_math.h>         // GSL math types and macros
#include <gsl/gsl_complex.h>      // gsl_complex types
#include <gsl/gsl_matrix.h>       // gsl_matrix for real matrices
#include <gsl/gsl_poly.h>         // for gsl_poly_complex_solve
#include "stack.h"

void poly_eval(Stack* stack) {
    if (stack->top < 1) {
        fprintf(stderr, "Stack underflow: need polynomial and x.\n");
        return;
    }

    stack_element x = stack->items[stack->top--];
    stack_element coeffs = stack->items[stack->top--];

    //    bool complex_x = (x.type == TYPE_COMPLEX);
    bool complex_coeffs = (coeffs.type == TYPE_MATRIX_COMPLEX);

    size_t len = 0;
    if (coeffs.type == TYPE_MATRIX_REAL && coeffs.matrix_real) {
        len = coeffs.matrix_real->size1 * coeffs.matrix_real->size2;
    } else if (coeffs.type == TYPE_MATRIX_COMPLEX && coeffs.matrix_complex) {
        len = coeffs.matrix_complex->size1 * coeffs.matrix_complex->size2;
    } else {
        fprintf(stderr, "poly_eval: coefficients must be a matrix.\n");
        return;
    }
    gsl_complex acc;

    // Get first coefficient
    if (complex_coeffs) {
      acc = gsl_matrix_complex_get(coeffs.matrix_complex, 0, 0);
    } else {
      double r = gsl_matrix_get(coeffs.matrix_real, 0, 0);
      acc = gsl_complex_rect(r, 0.0); // promote real to complex
    }

    gsl_complex z = (x.type == TYPE_COMPLEX) ? x.complex_val : gsl_complex_rect(x.real, 0.0);

    for (size_t i = 1; i < len; ++i) {
      acc = gsl_complex_mul(acc, z);      
      gsl_complex term;
      if (complex_coeffs) {
        term = gsl_matrix_complex_get(coeffs.matrix_complex, 0, i);
      } else {
        double r = gsl_matrix_get(coeffs.matrix_real, 0, i);
        term = gsl_complex_rect(r, 0.0);
      }      
      acc = gsl_complex_add(acc, term);
    }

    stack_element out;
    if (GSL_IMAG(acc) == 0.0) {
        out.type = TYPE_REAL;
        out.real = GSL_REAL(acc);
    } else {
        out.type = TYPE_COMPLEX;
        out.complex_val = acc;
    }

    stack->items[++stack->top] = out;
}

void poly_roots(Stack* stack) {
    if (stack->top < 0) {
        fprintf(stderr, "Stack underflow: no polynomial.\n");
        return;
    }

    stack_element coeffs = stack->items[stack->top--];

    if (coeffs.type != TYPE_MATRIX_REAL || !coeffs.matrix_real) {
        fprintf(stderr, "poly_roots: only real coefficient polynomials supported.\n");
        return;
    }

    gsl_matrix* mat = coeffs.matrix_real;
    size_t n = mat->size1 * mat->size2;
    if (n < 2) {
        fprintf(stderr, "poly_roots: degree must be at least 1.\n");
        return;
    }

    double* a = mat->data;
    double* z = malloc(2 * (n - 1) * sizeof(double));
    gsl_poly_complex_workspace* w = gsl_poly_complex_workspace_alloc(n);
    int status = gsl_poly_complex_solve(a, n, w, z);
    gsl_poly_complex_workspace_free(w);

    if (status != GSL_SUCCESS) {
        fprintf(stderr, "poly_roots: solver failed.\n");
        free(z);
        return;
    }

    gsl_matrix_complex* result = gsl_matrix_complex_alloc(1, n - 1);
    for (size_t i = 0; i < n - 1; ++i) {
        double re = z[2 * i];
        double im = z[2 * i + 1];
	gsl_complex root = gsl_complex_rect(re, im);
	gsl_matrix_complex_set(result, 0, i, root);
    }

    free(z);

    stack_element out;
    out.type = TYPE_MATRIX_COMPLEX;
    out.matrix_complex = result;
    stack->items[++stack->top] = out;
}
