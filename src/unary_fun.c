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
#include "stack.h"
#include "math_parsers.h"
#include "math_helpers.h"
#include "binary_fun.h"
#include "unary_fun.h"

// === Unary math functions for real and complex ===
void apply_real_unary(Stack* stack, double (*func)(double)) {
  if (stack->top < 0 || stack->items[stack->top].type != TYPE_REAL) {
    fprintf(stderr,"Expected real number\n");
    return;
  }
  stack->items[stack->top].real = func(stack->items[stack->top].real);
}

void apply_complex_unary(Stack* stack, gsl_complex (*func)(gsl_complex)) {
  if (stack->top < 0 || stack->items[stack->top].type != TYPE_COMPLEX) {
    fprintf(stderr,"Expected complex number\n");
    return;
  }
  stack->items[stack->top].complex_val = func(stack->items[stack->top].complex_val);
}

void apply_complex_matrix_unary_inplace(Stack* stack, gsl_complex (*func)(gsl_complex)) {
  if (stack->top < 0) {
    fprintf(stderr,"Stack is empty!\n");
    return;
  }

  stack_element* top = &stack->items[stack->top];
  if (top->type != TYPE_MATRIX_COMPLEX) {
    fprintf(stderr,"Top of stack is not a complex matrix!\n");
    return;
  }

  gsl_matrix_complex* mat = top->matrix_complex;
  size_t rows = mat->size1;
  size_t cols = mat->size2;

  for (size_t i = 0; i < rows; ++i) {
    for (size_t j = 0; j < cols; ++j) {
      gsl_complex z = gsl_matrix_complex_get(mat, i, j);
      gsl_matrix_complex_set(mat, i, j, func(z));
    }
  }
}

void apply_real_matrix_unary_inplace(Stack* stack, double (*func)(double)) {
  if (stack->top < 0) {
    fprintf(stderr,"Stack is empty!\n");
    return;
  }

  stack_element* top = &stack->items[stack->top];
  if (top->type != TYPE_MATRIX_REAL) {
    fprintf(stderr,"Top of stack is not a real matrix!\n");
    return;
  }

  gsl_matrix* mat = top->matrix_real;
  size_t rows = mat->size1;
  size_t cols = mat->size2;

  for (size_t i = 0; i < rows; ++i) {
    for (size_t j = 0; j < cols; ++j) {
      double val = gsl_matrix_get(mat, i, j);
      gsl_matrix_set(mat, i, j, func(val));
    }
  }
}

void complex_matrix_real_part(Stack *s) {
  if (s->top < 0 || s->items[s->top].type != TYPE_MATRIX_COMPLEX) {
    fprintf(stderr, "Error: top of stack must be a complex matrix.\n");
    return;
  }

  stack_element *src = &s->items[s->top];

  gsl_matrix_complex *matrix = src->matrix_complex;
  if (!matrix) {
    fprintf(stderr, "Error: complex matrix is NULL.\n");
    return;
  }

  size_t rows = matrix->size1;
  size_t cols = matrix->size2;

  gsl_matrix *result = gsl_matrix_alloc(rows, cols);
  if (!result) {
    fprintf(stderr, "Error: failed to allocate result matrix.\n");
    return;
  }

  for (size_t i = 0; i < rows; i++) {
    for (size_t j = 0; j < cols; j++) {
      gsl_complex z = gsl_matrix_complex_get(matrix, i, j);
      double real = GSL_REAL(z);
      gsl_matrix_set(result, i, j, real);
    }
  }

  // Inline stackl_pop:
  if (src->type == TYPE_MATRIX_COMPLEX && src->matrix_complex) {
    gsl_matrix_complex_free(src->matrix_complex);
  }

  //    src->type = TYPE_NONE; // Mark slot as empty
  s->top--;

  // Inline stackl_push_rmatrix:
  if (s->top + 1 >= STACK_SIZE) {
    fprintf(stderr, "Error: stack overflow when pushing result matrix.\n");
    gsl_matrix_free(result);
    return;
  }
  s->top++;
  stack_element *dest = &s->items[s->top];
  dest->type = TYPE_MATRIX_REAL;
  dest->matrix_real = result;
}

void complex_matrix_imag_part(Stack *s) {
  if (s->top < 0 || s->items[s->top].type != TYPE_MATRIX_COMPLEX) {
    fprintf(stderr, "Error: top of stack must be a complex matrix.\n");
    return;
  }

  stack_element *src = &s->items[s->top];

  gsl_matrix_complex *matrix = src->matrix_complex;
  if (!matrix) {
    fprintf(stderr, "Error: complex matrix is NULL.\n");
    return;
  }

  size_t rows = matrix->size1;
  size_t cols = matrix->size2;

  gsl_matrix *result = gsl_matrix_alloc(rows, cols);
  if (!result) {
    fprintf(stderr, "Error: failed to allocate result matrix.\n");
    return;
  }

  for (size_t i = 0; i < rows; i++) {
    for (size_t j = 0; j < cols; j++) {
      gsl_complex z = gsl_matrix_complex_get(matrix, i, j);
      double real = GSL_IMAG(z);
      gsl_matrix_set(result, i, j, real);
    }
  }

  // Inline stackl_pop:
  if (src->type == TYPE_MATRIX_COMPLEX && src->matrix_complex) {
    gsl_matrix_complex_free(src->matrix_complex);
  }
  //    src->type = TYPE_NONE; // Mark slot as empty
  s->top--;

  // Inline stackl_push_rmatrix:
  if (s->top + 1 >= STACK_SIZE) {
    fprintf(stderr, "Error: stack overflow when pushing result matrix.\n");
    gsl_matrix_free(result);
    return;
  }
  s->top++;
  stack_element *dest = &s->items[s->top];
  dest->type = TYPE_MATRIX_REAL;
  dest->matrix_real = result;
}

void complex_matrix_abs_by_element(Stack *s) {
  if (s->top < 0 || s->items[s->top].type != TYPE_MATRIX_COMPLEX) {
    fprintf(stderr, "Error: top of stack must be a complex matrix.\n");
    return;
  }

  stack_element *src = &s->items[s->top];

  gsl_matrix_complex *matrix = src->matrix_complex;
  if (!matrix) {
    fprintf(stderr, "Error: complex matrix is NULL.\n");
    return;
  }

  size_t rows = matrix->size1;
  size_t cols = matrix->size2;

  gsl_matrix *result = gsl_matrix_alloc(rows, cols);
  if (!result) {
    fprintf(stderr, "Error: failed to allocate result matrix.\n");
    return;
  }

  for (size_t i = 0; i < rows; i++) {
    for (size_t j = 0; j < cols; j++) {
      gsl_complex z = gsl_matrix_complex_get(matrix, i, j);
      double real = cabs(to_double_complex(z));
      gsl_matrix_set(result, i, j, real);
    }
  }

  // Inline stackl_pop:
  if (src->type == TYPE_MATRIX_COMPLEX && src->matrix_complex) {
    gsl_matrix_complex_free(src->matrix_complex);
  }
  //    src->type = TYPE_NONE; // Mark slot as empty
  s->top--;

  // Inline stackl_push_rmatrix:
  if (s->top + 1 >= STACK_SIZE) {
    fprintf(stderr, "Error: stack overflow when pushing result matrix.\n");
    gsl_matrix_free(result);
    return;
  }
  s->top++;
  stack_element *dest = &s->items[s->top];
  dest->type = TYPE_MATRIX_REAL;
  dest->matrix_real = result;
}

void real2complex(Stack *s) {
  if (s->top < 0) {
    fprintf(stderr, "Error: stack is empty.\n");
    return;
  }

  stack_element *src = &s->items[s->top];

  switch (src->type) {
  case TYPE_REAL: {
    double real = src->real;
    src->type = TYPE_COMPLEX;
    src->complex_val = gsl_complex_rect(real, 0.0);
    break;
  }

  case TYPE_MATRIX_REAL: {
    gsl_matrix *real_mat = src->matrix_real;
    if (!real_mat) {
      fprintf(stderr, "Error: real matrix is NULL.\n");
      return;
    }

    size_t rows = real_mat->size1;
    size_t cols = real_mat->size2;

    gsl_matrix_complex *complex_mat = gsl_matrix_complex_alloc(rows, cols);
    if (!complex_mat) {
      fprintf(stderr, "Error: failed to allocate complex matrix.\n");
      return;
    }

    for (size_t i = 0; i < rows; i++) {
      for (size_t j = 0; j < cols; j++) {
	double real = gsl_matrix_get(real_mat, i, j);
	gsl_complex z = gsl_complex_rect(real, 0.0);
	gsl_matrix_complex_set(complex_mat, i, j, z);
      }
    }

    // Free the old real matrix
    gsl_matrix_free(real_mat);

    // Update the stack element
    src->type = TYPE_MATRIX_COMPLEX;
    src->matrix_complex = complex_mat;
    break;
  }

  case TYPE_COMPLEX:
  case TYPE_MATRIX_COMPLEX:
    // Already complex; do nothing
    break;

  default:
    fprintf(stderr, "Error: r2c expects a real scalar, real matrix, or complex.\n");
    return;
  }
}

void split_complex(Stack *s) {
  if (s->top < 0) {
    fprintf(stderr, "Error: stack is empty.\n");
    return;
  }

  stack_element *src = &s->items[s->top];

  switch (src->type) {
  case TYPE_COMPLEX: {
    //            gsl_complex z = src->complex_val;
    double real_part = GSL_REAL(src->complex_val);
    double imag_part = GSL_IMAG(src->complex_val);

    // Check space for two pushes
    if (s->top + 2 >= STACK_SIZE) {
      fprintf(stderr, "Error: not enough space on stack to split scalar.\n");
      return;
    }

    // Pop the complex scalar
    s->top--;

    // Push real part
    s->top++;
    stack_element *real_elem = &s->items[s->top];
    real_elem->type = TYPE_REAL;
    real_elem->real = real_part;

    // Push imag part
    s->top++;
    stack_element *imag_elem = &s->items[s->top];
    imag_elem->type = TYPE_REAL;
    imag_elem->real = imag_part;

    break;
  }

  case TYPE_MATRIX_COMPLEX: {
    gsl_matrix_complex *matrix = src->matrix_complex;
    if (!matrix) {
      fprintf(stderr, "Error: complex matrix is NULL.\n");
      return;
    }

    size_t rows = matrix->size1;
    size_t cols = matrix->size2;

    gsl_matrix *real_mat = gsl_matrix_alloc(rows, cols);
    gsl_matrix *imag_mat = gsl_matrix_alloc(rows, cols);
    if (!real_mat || !imag_mat) {
      fprintf(stderr, "Error: failed to allocate real/imag matrices.\n");
      gsl_matrix_free(real_mat);
      gsl_matrix_free(imag_mat);
      return;
    }

    for (size_t i = 0; i < rows; i++) {
      for (size_t j = 0; j < cols; j++) {
	gsl_complex z = gsl_matrix_complex_get(matrix, i, j);
	gsl_matrix_set(real_mat, i, j, GSL_REAL(z));
	gsl_matrix_set(imag_mat, i, j, GSL_IMAG(z));
      }
    }

    // Check space for two pushes
    if (s->top + 2 >= STACK_SIZE) {
      fprintf(stderr, "Error: not enough space on stack to split matrix.\n");
      gsl_matrix_free(real_mat);
      gsl_matrix_free(imag_mat);
      return;
    }

    // Pop the complex matrix
    gsl_matrix_complex_free(matrix);
    s->top--;

    // Push real part
    s->top++;
    stack_element *real_elem = &s->items[s->top];
    real_elem->type = TYPE_MATRIX_REAL;
    real_elem->matrix_real = real_mat;

    // Push imag part
    s->top++;
    stack_element *imag_elem = &s->items[s->top];
    imag_elem->type = TYPE_MATRIX_REAL;
    imag_elem->matrix_real = imag_mat;

    break;
  }

  default:
    fprintf(stderr, "Error: split_complex expects a complex scalar or complex matrix.\n");
    return;
  }
}
