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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <complex.h>
#include <ctype.h>
#include <stdbool.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_complex_math.h>
#include <gsl/gsl_blas.h>         // For gsl_blas_dgemm, gsl_blas_zgemm
#include <gsl/gsl_linalg.h>       // For LU decomposition/inversion
#include <gsl/gsl_permutation.h>  // For gsl_permutation and related
#include <gsl/gsl_vector_complex.h>      // for gsl_vector_complex
#include <gsl/gsl_eigen.h>        // for eigen decomposition functions
#include "stack.h"
#include "math_parsers.h"
#include "math_helpers.h"
#include "compare_fun.h"

static int cmp_real(double a, double b, comparison_op op) {
  switch (op) {
    case CMP_EQ: return a == b;
    case CMP_NE: return a != b;
    case CMP_LT: return a <  b;
    case CMP_LE: return a <= b;
    case CMP_GT: return a >  b;
    case CMP_GE: return a >= b;
    case CMP_OR: return a ||  b;
    case CMP_AND: return a &&  b;
    default: return 0;
  }
}

static int cmp_complex(gsl_complex a, gsl_complex b, comparison_op op) {
  double abs_a = gsl_complex_abs(a);
  double abs_b = gsl_complex_abs(b);
  return cmp_real(abs_a, abs_b, op);  // compare magnitudes
}

void dot_cmp_top_two(Stack* stack, comparison_op op) {
  if (stack->top < 1) {
    fprintf(stderr, "Stack underflow in dot_cmp_top_two.\n");
    return;
  }

  stack_element* a = &stack->items[stack->top - 1];
  stack_element* b = &stack->items[stack->top];
  stack_element result = {0};

  // Scalar vs Scalar
  if (a->type == TYPE_REAL && b->type == TYPE_REAL) {
    result.type = TYPE_REAL;
    result.real = cmp_real(a->real, b->real, op);
  }
  else if ((a->type == TYPE_REAL || a->type == TYPE_COMPLEX) &&
           (b->type == TYPE_REAL || b->type == TYPE_COMPLEX)) {
    gsl_complex za = (a->type == TYPE_REAL) ? gsl_complex_rect(a->real, 0.0) : a->complex_val;
    gsl_complex zb = (b->type == TYPE_REAL) ? gsl_complex_rect(b->real, 0.0) : b->complex_val;
    result.type = TYPE_REAL;
    result.real = cmp_complex(za, zb, op);
  }

  // Scalar vs Matrix (Real)
  else if ((a->type == TYPE_REAL && b->type == TYPE_MATRIX_REAL) ||
           (a->type == TYPE_MATRIX_REAL && b->type == TYPE_REAL)) {
    gsl_matrix* mat = (a->type == TYPE_MATRIX_REAL) ? a->matrix_real : b->matrix_real;
    double val = (a->type == TYPE_REAL) ? a->real : b->real;
    int scalar_first = (a->type == TYPE_REAL);
    size_t rows = mat->size1, cols = mat->size2;
    result.type = TYPE_MATRIX_REAL;
    result.matrix_real = gsl_matrix_alloc(rows, cols);
    for (size_t i = 0; i < rows; ++i)
      for (size_t j = 0; j < cols; ++j) {
        double m = gsl_matrix_get(mat, i, j);
        double left = scalar_first ? val : m;
        double right = scalar_first ? m : val;
        gsl_matrix_set(result.matrix_real, i, j, cmp_real(left, right, op));
      }
  }

  // Scalar vs Matrix (Complex)
  else if ((a->type == TYPE_COMPLEX && b->type == TYPE_MATRIX_COMPLEX) ||
           (a->type == TYPE_MATRIX_COMPLEX && b->type == TYPE_COMPLEX)) {
    gsl_matrix_complex* mat =
      (a->type == TYPE_MATRIX_COMPLEX) ? a->matrix_complex : b->matrix_complex;
    gsl_complex z = (a->type == TYPE_COMPLEX) ? a->complex_val : b->complex_val;
    int scalar_first = (a->type == TYPE_COMPLEX);
    size_t rows = mat->size1, cols = mat->size2;
    result.type = TYPE_MATRIX_REAL;
    result.matrix_real = gsl_matrix_alloc(rows, cols);  // comparisons return real (0/1)
    for (size_t i = 0; i < rows; ++i)
      for (size_t j = 0; j < cols; ++j) {
        gsl_complex w = gsl_matrix_complex_get(mat, i, j);
        gsl_complex lhs = scalar_first ? z : w;
        gsl_complex rhs = scalar_first ? w : z;
        gsl_matrix_set(result.matrix_real, i, j, cmp_complex(lhs, rhs, op));
      }
  }

  // Matrix vs Matrix
  else if (a->type == TYPE_MATRIX_REAL && b->type == TYPE_MATRIX_REAL) {
    if (a->matrix_real->size1 != b->matrix_real->size1 ||
        a->matrix_real->size2 != b->matrix_real->size2) {
      fprintf(stderr, "Matrix size mismatch in dot_cmp_top_two (real).\n");
      return;
    }
    size_t rows = a->matrix_real->size1, cols = a->matrix_real->size2;
    result.type = TYPE_MATRIX_REAL;
    result.matrix_real = gsl_matrix_alloc(rows, cols);
    for (size_t i = 0; i < rows; ++i)
      for (size_t j = 0; j < cols; ++j) {
        double x = gsl_matrix_get(a->matrix_real, i, j);
        double y = gsl_matrix_get(b->matrix_real, i, j);
        gsl_matrix_set(result.matrix_real, i, j, cmp_real(x, y, op));
      }
  }
  else if (a->type == TYPE_MATRIX_COMPLEX && b->type == TYPE_MATRIX_COMPLEX) {
    if (a->matrix_complex->size1 != b->matrix_complex->size1 ||
        a->matrix_complex->size2 != b->matrix_complex->size2) {
      fprintf(stderr, "Matrix size mismatch in dot_cmp_top_two (complex).\n");
      return;
    }
    size_t rows = a->matrix_complex->size1, cols = a->matrix_complex->size2;
    result.type = TYPE_MATRIX_REAL;
    result.matrix_real = gsl_matrix_alloc(rows, cols);  // comparison result: 0.0 or 1.0
    for (size_t i = 0; i < rows; ++i)
      for (size_t j = 0; j < cols; ++j) {
        gsl_complex x = gsl_matrix_complex_get(a->matrix_complex, i, j);
        gsl_complex y = gsl_matrix_complex_get(b->matrix_complex, i, j);
        gsl_matrix_set(result.matrix_real, i, j, cmp_complex(x, y, op));
      }
  }
  else {
    fprintf(stderr, "Unsupported types in dot_cmp_top_two.\n");
    return;
  }

  // Free old memory
  if (a->type == TYPE_MATRIX_REAL && a->matrix_real)
    gsl_matrix_free(a->matrix_real);
  if (a->type == TYPE_MATRIX_COMPLEX && a->matrix_complex)
    gsl_matrix_complex_free(a->matrix_complex);

  *a = result;
  stack->top--;
}
