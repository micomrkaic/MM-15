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

// An implementation of a heterogenous stack and some operations for an RPN calculator
// Mico Mrkaic, mrkaic.mico@gmail.com

#define _POSIX_C_SOURCE 200809L
#include <math.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_complex_math.h>
#include "stack.h"
#include "globals.h"
#include "print_fun.h"

void print_top_scalar(const Stack* stack) {
  if (stack->top == -1) {
    printf("{}\n");
    return;
  }
  int i = stack->top;
  switch (stack->items[i].type) {
  case TYPE_REAL:
    if (fixed_point)
      printf("[%d] ℝ : %.*f\n", i, print_precision, stack->items[i].real);
    else
      printf("[%d] ℝ : %.*g\n", i, print_precision, stack->items[i].real);
    break;
  case TYPE_COMPLEX:
    if (fixed_point)
      printf("[%d] ℂ : (%.*f, %.*fi)\n", i,
	     print_precision, GSL_REAL(stack->items[i].complex_val),
	     print_precision, GSL_IMAG(stack->items[i].complex_val));
    else
      printf("[%d] ℂ : (%.*g, %.*gi)\n", i,
	     print_precision, GSL_REAL(stack->items[i].complex_val),
	     print_precision, GSL_IMAG(stack->items[i].complex_val));
    break;
  case TYPE_STRING:
    printf("[%d] 𝒮 : \"%s\"\n", i, stack->items[i].string);
    break;
  default:
    break;
  }
}

void print_stack(const Stack* stack, char *title) {
  if (title) printf("%s\n",title);
  if (stack->top == -1) {
    printf("{}\n");
    return;
  }
  printf("\n");
  for (int i = 0; i <= stack->top; i++) {
    switch (stack->items[i].type) {
    case TYPE_REAL:
      if (fixed_point)
	printf("[%d] ℝ : %.*f\n", i, print_precision, stack->items[i].real);
      else
	printf("[%d] ℝ : %.*g\n", i, print_precision, stack->items[i].real);
      break;
    case TYPE_COMPLEX:
      if (fixed_point)
	printf("[%d] ℂ : (%.*f, %.*fi)\n", i,
	       print_precision, GSL_REAL(stack->items[i].complex_val),
	       print_precision, GSL_IMAG(stack->items[i].complex_val));
      else
	printf("[%d] ℂ : (%.*g, %.*gi)\n", i,
	       print_precision, GSL_REAL(stack->items[i].complex_val),
	       print_precision, GSL_IMAG(stack->items[i].complex_val));
      break;
    case TYPE_STRING:
      printf("[%d] 𝒮 : \"%s\"\n", i, stack->items[i].string);
      break;
    case TYPE_MATRIX_REAL:
      printf("[%d] Mℝ: %zu x %zu matrix\n", i,
	     stack->items[i].matrix_real->size1,
	     stack->items[i].matrix_real->size2);
      break;
    case TYPE_MATRIX_COMPLEX:
      printf("[%d] Mℂ: %zu x %zu matrix\n", i,
	     stack->items[i].matrix_complex->size1,
	     stack->items[i].matrix_complex->size2);
      break;
    }
  }
}

void print_matrix(Stack *stack) {
  stack_element a = check_top(stack);
  if (a.type == TYPE_MATRIX_REAL) print_real_matrix(a.matrix_real);
  if (a.type == TYPE_MATRIX_COMPLEX) print_complex_matrix(a.matrix_complex);
  return;
}

void print_real_matrix(const gsl_matrix* m) {
  size_t rows = m->size1;
  size_t cols = m->size2;
  int col_width[cols];

  // Step 1: find max width per column
  for (size_t j = 0; j < cols; ++j) {
    col_width[j] = 0;
    for (size_t i = 0; i < rows; ++i) {
      char buf[64];
      double val = gsl_matrix_get(m, i, j);
      if (fabs(val) > 1e5 || (fabs(val) > 0 && fabs(val) < 1e-4)) {
	snprintf(buf, sizeof(buf), "%.4e", val);
      } else {
	snprintf(buf, sizeof(buf), "%.4f", val);
      }
      int len = (int)strlen(buf);
      if (len > col_width[j]) col_width[j] = len;
    }
  }

  // Step 2: print each row
  for (size_t i = 0; i < rows; ++i) {
    printf("| ");
    for (size_t j = 0; j < cols; ++j) {
      double val = gsl_matrix_get(m, i, j);
      if (fabs(val) > 1e5 || (fabs(val) > 0 && fabs(val) < 1e-4)) {
	printf("%*.*e ", col_width[j], 4, val);
      } else {
	printf("%*.*f ", col_width[j], 4, val);
      }
    }
    printf("|\n");
  }
}

void print_complex_matrix(const gsl_matrix_complex* m) {
  size_t rows = m->size1;
  size_t cols = m->size2;
  int col_width[cols];

  // Step 1: find max width per column
  for (size_t j = 0; j < cols; ++j) {
    col_width[j] = 0;
    for (size_t i = 0; i < rows; ++i) {
      char buf[128];
      gsl_complex z = gsl_matrix_complex_get(m, i, j);
      double re = GSL_REAL(z);
      double im = GSL_IMAG(z);

      if ((fabs(re) > 1e5 || (fabs(re) > 0 && fabs(re) < 1e-4)) ||
	  (fabs(im) > 1e5 || (fabs(im) > 0 && fabs(im) < 1e-4))) {
	snprintf(buf, sizeof(buf), "(%.4e,%.4e)", re, im);
      } else {
	snprintf(buf, sizeof(buf), "(%.4f,%.4f)", re, im);
      }
      int len = (int)strlen(buf);
      if (len > col_width[j]) col_width[j] = len;
    }
  }

  // Step 2: print each row
  for (size_t i = 0; i < rows; ++i) {
    printf("| ");
    for (size_t j = 0; j < cols; ++j) {
      gsl_complex z = gsl_matrix_complex_get(m, i, j);
      double re = GSL_REAL(z);
      double im = GSL_IMAG(z);

      if ((fabs(re) > 1e5 || (fabs(re) > 0 && fabs(re) < 1e-4)) ||
	  (fabs(im) > 1e5 || (fabs(im) > 0 && fabs(im) < 1e-4))) {
	printf("%*s ", col_width[j], "");
	printf("(%.4e,%.4e)", re, im);
      } else {
	printf("%*s ", col_width[j], "");
	printf("(%.4f,%.4f)", re, im);
      }
    }
    printf("|\n");
  }
}


