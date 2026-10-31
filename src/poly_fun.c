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
#include <gsl/gsl_complex.h>                // for gsl_complex, GSL_IMAG
#include <gsl/gsl_complex_math.h>           // for gsl_complex_rect, gsl_com...
#include <gsl/gsl_errno.h>                  // for GSL_SUCCESS
#include <gsl/gsl_matrix_complex_double.h>  // for gsl_matrix_complex_get
#include <gsl/gsl_matrix_double.h>          // for gsl_matrix_get, gsl_matrix
#include <gsl/gsl_poly.h>                   // for gsl_poly_complex_solve
#include <stdbool.h>                        // for bool
#include <stdio.h>                          // for fprintf, stderr, size_t
#include <stdlib.h>                         // for free, malloc
#include "stack.h"                          // for (anonymous struct)::(anon...
#include "poly_fun.h"                       // for poly_eval, poly_roots

void poly_eval(Stack* stack) {
  if (stack->top < 1) {
    mm_error("Stack underflow: need polynomial and x.\n");
    return;
  }

  const stack_element *x = &stack->items[stack->top];
  const stack_element *coeffs = &stack->items[stack->top - 1];

  if (x->type != TYPE_REAL && x->type != TYPE_COMPLEX) {
    mm_error("pval: x must be a real or complex scalar");
    return;
  }

  bool complex_coeffs = (coeffs->type == TYPE_MATRIX_COMPLEX);
  size_t len = 0, cols = 1;
  if (coeffs->type == TYPE_MATRIX_REAL && coeffs->matrix_real) {
    len = coeffs->matrix_real->size1 * coeffs->matrix_real->size2;
    cols = coeffs->matrix_real->size2;
  } else if (coeffs->type == TYPE_MATRIX_COMPLEX && coeffs->matrix_complex) {
    len = coeffs->matrix_complex->size1 * coeffs->matrix_complex->size2;
    cols = coeffs->matrix_complex->size2;
  } else {
    mm_error("pval: coefficients must be a matrix (highest degree first)");
    return;
  }
  if (len == 0) {
    mm_error("pval: empty coefficient matrix");
    return;
  }

  /* Horner's rule over the elements in row-major order */
  gsl_complex acc = complex_coeffs
    ? gsl_matrix_complex_get(coeffs->matrix_complex, 0, 0)
    : gsl_complex_rect(gsl_matrix_get(coeffs->matrix_real, 0, 0), 0.0);
  gsl_complex z = (x->type == TYPE_COMPLEX) ? x->complex_val : gsl_complex_rect(x->real, 0.0);

  for (size_t i = 1; i < len; ++i) {
    size_t r = i / cols, c = i % cols;
    gsl_complex term = complex_coeffs
      ? gsl_matrix_complex_get(coeffs->matrix_complex, r, c)
      : gsl_complex_rect(gsl_matrix_get(coeffs->matrix_real, r, c), 0.0);
    acc = gsl_complex_add(gsl_complex_mul(acc, z), term);
  }

  pop_and_free(stack);
  pop_and_free(stack);
  if (GSL_IMAG(acc) == 0.0) push_real(stack, GSL_REAL(acc));
  else push_complex(stack, acc);
}

void poly_roots(Stack* stack) {
  if (stack->top < 0) {
    mm_error("Stack underflow: no polynomial.\n");
    return;
  }

  const stack_element *coeffs = &stack->items[stack->top];

  if (coeffs->type != TYPE_MATRIX_REAL || !coeffs->matrix_real) {
    mm_error("roots: only real coefficient polynomials are supported");
    return;
  }

  gsl_matrix* mat = coeffs->matrix_real;
  size_t n = mat->size1 * mat->size2;
  if (n < 2) {
    mm_error("roots: degree must be at least 1");
    return;
  }
  if (mat->size1 != 1 && mat->size2 != 1) {
    mm_error("roots: coefficients must be a row or column vector");
    return;
  }

  /* Coefficients are given highest degree first (as for pval); GSL wants
     them in ascending order, so reverse into a contiguous buffer. */
  double* a = malloc(n * sizeof(double));
  double* z = malloc(2 * (n - 1) * sizeof(double));
  if (!a || !z) {
    mm_error("roots: memory allocation failed");
    free(a); free(z);
    return;
  }
  for (size_t i = 0; i < n; ++i) {
    size_t src = n - 1 - i;
    a[i] = (mat->size1 == 1) ? gsl_matrix_get(mat, 0, src) : gsl_matrix_get(mat, src, 0);
  }
  if (a[n - 1] == 0.0) {
    mm_error("roots: leading coefficient must be nonzero");
    free(a); free(z);
    return;
  }
  gsl_poly_complex_workspace* w = gsl_poly_complex_workspace_alloc(n);
  int status = gsl_poly_complex_solve(a, n, w, z);
  gsl_poly_complex_workspace_free(w);
  free(a);

  if (status != GSL_SUCCESS) {
    mm_error("roots: solver failed");
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

  pop_and_free(stack);
  push_matrix_complex(stack, result);
}
