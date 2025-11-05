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

void add_top_two_scalars(Stack* stack) {
  if (stack->top < 1) {
    fprintf(stderr,"Not enough elements to add\n");
    return;
  }
  stack_element a = pop(stack);
  stack_element b = pop(stack);

  if (a.type == TYPE_REAL && b.type == TYPE_REAL) {
    push_real(stack, a.real + b.real);
  } else if ((a.type == TYPE_REAL || a.type == TYPE_COMPLEX)
	     && (b.type == TYPE_REAL || b.type == TYPE_COMPLEX)) {
    gsl_complex ca = (a.type == TYPE_COMPLEX) ? a.complex_val : gsl_complex_rect(a.real, 0.0);
    gsl_complex cb = (b.type == TYPE_COMPLEX) ? b.complex_val : gsl_complex_rect(b.real, 0.0);
    push_complex(stack, gsl_complex_add(ca, cb));
  } else {
    fprintf(stderr,"Unsupported types for addition\n");
  }
}

void add_top_two_matrices(Stack* stack) {
  if (stack->top < 1) {
    fprintf(stderr,"Not enough matrices to add\n");
    return;
  }
  stack_element a = pop(stack);
  stack_element b = pop(stack);

  if (a.type == TYPE_MATRIX_REAL && b.type == TYPE_MATRIX_REAL) {
    if (a.matrix_real->size1 != b.matrix_real->size1
	|| a.matrix_real->size2 != b.matrix_real->size2) {
      fprintf(stderr,"Matrix dimensions must match\n");
      gsl_matrix_free(a.matrix_real);
      gsl_matrix_free(b.matrix_real);
      return;
    }
    gsl_matrix* result = gsl_matrix_alloc(a.matrix_real->size1, a.matrix_real->size2);
    gsl_matrix_memcpy(result, b.matrix_real);
    gsl_matrix_add(result, a.matrix_real);
    gsl_matrix_free(a.matrix_real);
    gsl_matrix_free(b.matrix_real);
    push_matrix_real(stack, result);
  } else if (a.type == TYPE_MATRIX_COMPLEX && b.type == TYPE_MATRIX_COMPLEX) {
    if (a.matrix_complex->size1 != b.matrix_complex->size1
	|| a.matrix_complex->size2 != b.matrix_complex->size2) {
      fprintf(stderr,"Matrix dimensions must match\n");
      gsl_matrix_complex_free(a.matrix_complex);
      gsl_matrix_complex_free(b.matrix_complex);
      return;
    }
    gsl_matrix_complex* result =
      gsl_matrix_complex_alloc(a.matrix_complex->size1, a.matrix_complex->size2);
    gsl_matrix_complex_memcpy(result, b.matrix_complex);
    gsl_matrix_complex_add(result, a.matrix_complex);
    gsl_matrix_complex_free(a.matrix_complex);
    gsl_matrix_complex_free(b.matrix_complex);
    push_matrix_complex(stack, result);
  } else {
    fprintf(stderr,"Unsupported matrix types for addition\n");
  }
}

void multiply_top_two_scalars(Stack* stack) {
  if (stack->top < 1) {
    fprintf(stderr,"Not enough elements to add\n");
    return;
  }
  stack_element a = pop(stack);
  stack_element b = pop(stack);

  if (a.type == TYPE_REAL && b.type == TYPE_REAL) {
    push_real(stack, a.real * b.real);
  } else if ((a.type == TYPE_REAL || a.type == TYPE_COMPLEX)
	     && (b.type == TYPE_REAL || b.type == TYPE_COMPLEX)) {
    gsl_complex ca = (a.type == TYPE_COMPLEX) ? a.complex_val : gsl_complex_rect(a.real, 0.0);
    gsl_complex cb = (b.type == TYPE_COMPLEX) ? b.complex_val : gsl_complex_rect(b.real, 0.0);
    push_complex(stack, gsl_complex_mul(ca, cb));
  } else {
    fprintf(stderr,"Unsupported types for multiplication\n");
  }
}

void subtract_top_two_scalars(Stack* stack) {
  if (stack->top < 1) {
    fprintf(stderr,"Not enough elements to add\n");
    return;
  }
  stack_element b = pop(stack);
  stack_element a = pop(stack);

  if (a.type == TYPE_REAL && b.type == TYPE_REAL) {
    push_real(stack, a.real - b.real);
  } else if ((a.type == TYPE_REAL || a.type == TYPE_COMPLEX)
	     && (b.type == TYPE_REAL || b.type == TYPE_COMPLEX)) {
    gsl_complex ca = (a.type == TYPE_COMPLEX) ? a.complex_val : gsl_complex_rect(a.real, 0.0);
    gsl_complex cb = (b.type == TYPE_COMPLEX) ? b.complex_val : gsl_complex_rect(b.real, 0.0);
    push_complex(stack, gsl_complex_sub(ca, cb));
  } else {
    fprintf(stderr,"Unsupported types for subtraction\n");
  }
}

void divide_top_two_scalars(Stack* stack) {
  if (stack->top < 1) {
    fprintf(stderr,"Not enough elements to divide\n");
    return;
  }
  stack_element b = pop(stack);
  if ((b.type == TYPE_REAL) && (b.real == 0.0)) {
    push_real(stack,b.real);
    return;
  }
  if ((b.type == TYPE_COMPLEX) && is_zero_complex(b.complex_val)) {
    push_complex(stack,b.complex_val);
    return;
  }
  // Division allowed, proceed
  stack_element a = pop(stack);
  if (a.type == TYPE_REAL && b.type == TYPE_REAL) {
    push_real(stack, a.real / b.real);
  } else if ((a.type == TYPE_REAL || a.type == TYPE_COMPLEX)
	     && (b.type == TYPE_REAL || b.type == TYPE_COMPLEX)) {
    gsl_complex ca = (a.type == TYPE_COMPLEX) ? a.complex_val : gsl_complex_rect(a.real, 0.0);
    gsl_complex cb = (b.type == TYPE_COMPLEX) ? b.complex_val : gsl_complex_rect(b.real, 0.0);
    push_complex(stack, gsl_complex_div(ca,cb));
  } else {
    fprintf(stderr,"Unsupported types for division\n");
  }
}

void subtract_top_two_matrices(Stack* stack) {
  if (stack->top < 1) {
    fprintf(stderr,"Not enough elements for matrix subtraction\n");
    return;
  }
  stack_element b = pop(stack);
  stack_element a = pop(stack);

  if (a.type == TYPE_MATRIX_REAL && b.type == TYPE_MATRIX_REAL) {
    if (a.matrix_real->size1 != b.matrix_real->size1
	|| a.matrix_real->size2 != b.matrix_real->size2) {
      fprintf(stderr,"Matrix size mismatch\n");
      return;
    }
    gsl_matrix* result = gsl_matrix_alloc(a.matrix_real->size1, a.matrix_real->size2);
    gsl_matrix_memcpy(result, a.matrix_real);
    gsl_matrix_sub(result, b.matrix_real);
    push_matrix_real(stack, result);
  } else if (a.type == TYPE_MATRIX_COMPLEX && b.type == TYPE_MATRIX_COMPLEX) {
    if (a.matrix_complex->size1 != b.matrix_complex->size1
	|| a.matrix_complex->size2 != b.matrix_complex->size2) {
      fprintf(stderr,"Matrix size mismatch\n");
      return;
    }
    gsl_matrix_complex* result =
      gsl_matrix_complex_alloc(a.matrix_complex->size1, a.matrix_complex->size2);
    gsl_matrix_complex_memcpy(result, a.matrix_complex);
    gsl_matrix_complex_sub(result, b.matrix_complex);
    push_matrix_complex(stack, result);
  } else {
    fprintf(stderr,"Unsupported matrix types for subtraction\n");
  }
}

void multiply_top_two_matrices(Stack* stack) {
  if (stack->top < 1) {
    fprintf(stderr,"Not enough elements for matrix multiplication\n");
    return;
  }
  stack_element b = pop(stack);
  stack_element a = pop(stack);

  if (a.type == TYPE_MATRIX_REAL && b.type == TYPE_MATRIX_REAL) {
    if (a.matrix_real->size2 != b.matrix_real->size1) {
      fprintf(stderr,"Matrix dimensions do not allow multiplication\n");
      return;
    }
    gsl_matrix* result =
      gsl_matrix_alloc(a.matrix_real->size1, b.matrix_real->size2);
    gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, a.matrix_real, b.matrix_real, 0.0, result);
    push_matrix_real(stack, result);
  } else if (a.type == TYPE_MATRIX_COMPLEX && b.type == TYPE_MATRIX_COMPLEX) {
    if (a.matrix_complex->size2 != b.matrix_complex->size1) {
      fprintf(stderr,"Matrix dimensions do not allow multiplication\n");
      return;
    }
    gsl_matrix_complex* result =
      gsl_matrix_complex_alloc(a.matrix_complex->size1, b.matrix_complex->size2);
    gsl_blas_zgemm(CblasNoTrans, CblasNoTrans,
		   GSL_COMPLEX_ONE, a.matrix_complex, b.matrix_complex,
		   GSL_COMPLEX_ZERO, result);
    push_matrix_complex(stack, result);
  } else {
    fprintf(stderr,"Unsupported matrix types for multiplication\n");
  }
}

void add_top_two(Stack* stack) {
  if (stack->top < 1) {
    fprintf(stderr, "Stack underflow in add_top_two.\n");
    return;
  }

  stack_element* a = &stack->items[stack->top - 1]; // second from top
  stack_element* b = &stack->items[stack->top];     // top
  stack_element result = {0};                       // temp result

  // Dispatch begins here
  if (a->type == TYPE_REAL && b->type == TYPE_REAL) {
    result.type = TYPE_REAL;
    result.real = a->real + b->real;
  }
  else if ((a->type == TYPE_REAL && b->type == TYPE_COMPLEX) ||
	   (a->type == TYPE_COMPLEX && b->type == TYPE_REAL)) {
    result.type = TYPE_COMPLEX;
    result.complex_val =
      gsl_complex_add((a->type == TYPE_REAL ? gsl_complex_rect(a->real, 0.0) : a->complex_val),
		      (b->type == TYPE_REAL ? gsl_complex_rect(b->real,0.0) : b->complex_val));
  }
  else if (a->type == TYPE_COMPLEX && b->type == TYPE_COMPLEX) {
    result.type = TYPE_COMPLEX;
    result.complex_val = gsl_complex_add(a->complex_val, b->complex_val);
  }
  else if ((a->type == TYPE_REAL && b->type == TYPE_MATRIX_REAL) ||
	   (a->type == TYPE_MATRIX_REAL && b->type == TYPE_REAL)) {
    result.type = TYPE_MATRIX_REAL;
    gsl_matrix* mat = (a->type == TYPE_MATRIX_REAL) ? a->matrix_real : b->matrix_real;
    double val = (a->type == TYPE_REAL) ? a->real : b->real;

    size_t rows = mat->size1, cols = mat->size2;
    result.matrix_real = gsl_matrix_alloc(rows, cols);
    gsl_matrix_memcpy(result.matrix_real, mat);
    gsl_matrix_add_constant(result.matrix_real, val);
  }
  else if ((a->type == TYPE_REAL && b->type == TYPE_MATRIX_COMPLEX) ||
	   (a->type == TYPE_MATRIX_COMPLEX && b->type == TYPE_REAL)) {
    result.type = TYPE_MATRIX_COMPLEX;
    gsl_matrix_complex* mat =
      (a->type == TYPE_MATRIX_COMPLEX) ? a->matrix_complex : b->matrix_complex;
    double val = (a->type == TYPE_REAL) ? a->real : b->real;

    size_t rows = mat->size1, cols = mat->size2;
    result.matrix_complex = gsl_matrix_complex_alloc(rows, cols);
    gsl_matrix_complex_memcpy(result.matrix_complex, mat);

    gsl_complex z = gsl_complex_rect(val, 0.0);
    for (size_t i = 0; i < rows; ++i)
      for (size_t j = 0; j < cols; ++j) {
	gsl_complex old = gsl_matrix_complex_get(result.matrix_complex, i, j);
	gsl_matrix_complex_set(result.matrix_complex, i, j, gsl_complex_add(old, z));
      }
  }
  else if ((a->type == TYPE_COMPLEX && b->type == TYPE_MATRIX_REAL) ||
	   (a->type == TYPE_MATRIX_REAL && b->type == TYPE_COMPLEX)) {
    result.type = TYPE_MATRIX_COMPLEX;
    gsl_matrix* mat_real = (a->type == TYPE_MATRIX_REAL) ? a->matrix_real : b->matrix_real;
    gsl_complex z = (a->type == TYPE_COMPLEX) ? a->complex_val : b->complex_val;

    size_t rows = mat_real->size1, cols = mat_real->size2;
    result.matrix_complex = gsl_matrix_complex_alloc(rows, cols);

    for (size_t i = 0; i < rows; ++i)
      for (size_t j = 0; j < cols; ++j) {
	double val = gsl_matrix_get(mat_real, i, j);
	gsl_matrix_complex_set(result.matrix_complex, i, j,
			       gsl_complex_add(z, gsl_complex_rect(val, 0.0)));
      }
  }
  else if ((a->type == TYPE_COMPLEX && b->type == TYPE_MATRIX_COMPLEX) ||
	   (a->type == TYPE_MATRIX_COMPLEX && b->type == TYPE_COMPLEX)) {
    result.type = TYPE_MATRIX_COMPLEX;
    gsl_matrix_complex* mat =
      (a->type == TYPE_MATRIX_COMPLEX) ? a->matrix_complex : b->matrix_complex;
    gsl_complex z = (a->type == TYPE_COMPLEX) ? a->complex_val : b->complex_val;

    size_t rows = mat->size1, cols = mat->size2;
    result.matrix_complex = gsl_matrix_complex_alloc(rows, cols);
    gsl_matrix_complex_memcpy(result.matrix_complex, mat);

    for (size_t i = 0; i < rows; ++i)
      for (size_t j = 0; j < cols; ++j) {
	gsl_complex old = gsl_matrix_complex_get(result.matrix_complex, i, j);
	gsl_matrix_complex_set(result.matrix_complex, i, j, gsl_complex_add(old, z));
      }
  }
  else if (a->type == TYPE_MATRIX_REAL && b->type == TYPE_MATRIX_REAL) {
    if (a->matrix_real->size1 != b->matrix_real->size1 ||
	a->matrix_real->size2 != b->matrix_real->size2) {
      fprintf(stderr, "Matrix size mismatch.\n");
      return;
    }
    result.type = TYPE_MATRIX_REAL;
    result.matrix_real = gsl_matrix_alloc(a->matrix_real->size1, a->matrix_real->size2);
    gsl_matrix_memcpy(result.matrix_real, a->matrix_real);
    gsl_matrix_add(result.matrix_real, b->matrix_real);
  }
  else if (a->type == TYPE_MATRIX_COMPLEX && b->type == TYPE_MATRIX_COMPLEX) {
    if (a->matrix_complex->size1 != b->matrix_complex->size1 ||
	a->matrix_complex->size2 != b->matrix_complex->size2) {
      fprintf(stderr, "Matrix size mismatch.\n");
      return;
    }
    result.type = TYPE_MATRIX_COMPLEX;
    result.matrix_complex =
      gsl_matrix_complex_alloc(a->matrix_complex->size1, a->matrix_complex->size2);
    gsl_matrix_complex_memcpy(result.matrix_complex, a->matrix_complex);
    gsl_matrix_complex_add(result.matrix_complex, b->matrix_complex);
  }
  else {
    fprintf(stderr, "Unsupported operand types in add_top_two.\n");
    return;
  }

  // Free any heap-allocated matrix in a
  if (a->type == TYPE_MATRIX_REAL && a->matrix_real)
    gsl_matrix_free(a->matrix_real);
  if (a->type == TYPE_MATRIX_COMPLEX && a->matrix_complex)
    gsl_matrix_complex_free(a->matrix_complex);

  // Overwrite a with result and reduce stack
  *a = result;
  stack->top--; // pop one element
}

void sub_top_two(Stack* stack) {
  if (stack->top < 1) {
    fprintf(stderr, "Stack underflow in sub_top_two.\n");
    return;
  }

  stack_element* a = &stack->items[stack->top - 1]; // second from top
  stack_element* b = &stack->items[stack->top];     // top
  stack_element result = {0};

  if (a->type == TYPE_REAL && b->type == TYPE_REAL) {
    result.type = TYPE_REAL;
    result.real = a->real - b->real;
  }
  else if ((a->type == TYPE_REAL && b->type == TYPE_COMPLEX) ||
	   (a->type == TYPE_COMPLEX && b->type == TYPE_REAL)) {
    result.type = TYPE_COMPLEX;
    gsl_complex za = (a->type == TYPE_REAL) ? gsl_complex_rect(a->real, 0) : a->complex_val;
    gsl_complex zb = (b->type == TYPE_REAL) ? gsl_complex_rect(b->real, 0) : b->complex_val;
    result.complex_val = gsl_complex_sub(za, zb);
  }
  else if (a->type == TYPE_COMPLEX && b->type == TYPE_COMPLEX) {
    result.type = TYPE_COMPLEX;
    //    result.complex_val = gsl_complex_sub(a->complex_val, b->complex_val);
    result.complex_val = gsl_complex_sub(a->complex_val, b->complex_val);
  }
  else if ((a->type == TYPE_REAL && b->type == TYPE_MATRIX_REAL) ||
	   (a->type == TYPE_MATRIX_REAL && b->type == TYPE_REAL)) {
    result.type = TYPE_MATRIX_REAL;
    gsl_matrix* mat = (a->type == TYPE_MATRIX_REAL) ? a->matrix_real : b->matrix_real;
    double val = (a->type == TYPE_REAL) ? a->real : b->real;
    int scalar_first = (a->type == TYPE_REAL);

    size_t rows = mat->size1, cols = mat->size2;
    result.matrix_real = gsl_matrix_alloc(rows, cols);
    gsl_matrix_memcpy(result.matrix_real, mat);

    if (scalar_first) {
      // scalar - matrix
      gsl_matrix_scale(result.matrix_real, -1.0);
      gsl_matrix_add_constant(result.matrix_real, val);
    } else {
      // matrix - scalar
      gsl_matrix_add_constant(result.matrix_real, -val);
    }
  }
  else if ((a->type == TYPE_REAL && b->type == TYPE_MATRIX_COMPLEX) ||
	   (a->type == TYPE_MATRIX_COMPLEX && b->type == TYPE_REAL)) {
    result.type = TYPE_MATRIX_COMPLEX;
    gsl_matrix_complex* mat =
      (a->type == TYPE_MATRIX_COMPLEX) ? a->matrix_complex : b->matrix_complex;
    gsl_complex z = gsl_complex_rect((a->type == TYPE_REAL) ? a->real : b->real, 0.0);
    int scalar_first = (a->type == TYPE_REAL);

    size_t rows = mat->size1, cols = mat->size2;
    result.matrix_complex = gsl_matrix_complex_alloc(rows, cols);
    gsl_matrix_complex_memcpy(result.matrix_complex, mat);

    if (scalar_first) {
      // scalar - matrix
      for (size_t i = 0; i < rows; ++i)
	for (size_t j = 0; j < cols; ++j) {
	  gsl_complex v = gsl_matrix_complex_get(mat, i, j);
	  gsl_matrix_complex_set(result.matrix_complex, i, j, gsl_complex_sub(z, v));
	}
    } else {
      // matrix - scalar
      for (size_t i = 0; i < rows; ++i)
	for (size_t j = 0; j < cols; ++j) {
	  gsl_complex v = gsl_matrix_complex_get(mat, i, j);
	  gsl_matrix_complex_set(result.matrix_complex, i, j, gsl_complex_sub(v, z));
	}
    }
  }
  else if ((a->type == TYPE_COMPLEX && b->type == TYPE_MATRIX_REAL) ||
	   (a->type == TYPE_MATRIX_REAL && b->type == TYPE_COMPLEX)) {
    result.type = TYPE_MATRIX_COMPLEX;
    gsl_matrix* mat_real = (a->type == TYPE_MATRIX_REAL) ? a->matrix_real : b->matrix_real;
    gsl_complex z = (a->type == TYPE_COMPLEX) ? a->complex_val : b->complex_val;
    int scalar_first = (a->type == TYPE_COMPLEX);

    size_t rows = mat_real->size1, cols = mat_real->size2;
    result.matrix_complex = gsl_matrix_complex_alloc(rows, cols);

    if (scalar_first) {
      // complex scalar - real matrix
      for (size_t i = 0; i < rows; ++i)
	for (size_t j = 0; j < cols; ++j) {
	  gsl_complex v = gsl_complex_rect(gsl_matrix_get(mat_real, i, j), 0);
	  gsl_matrix_complex_set(result.matrix_complex, i, j, gsl_complex_sub(z, v));
	}
    } else {
      // real matrix - complex scalar
      for (size_t i = 0; i < rows; ++i)
	for (size_t j = 0; j < cols; ++j) {
	  gsl_complex v = gsl_complex_rect(gsl_matrix_get(mat_real, i, j), 0);
	  gsl_matrix_complex_set(result.matrix_complex, i, j, gsl_complex_sub(v, z));
	}
    }
  }
  else if ((a->type == TYPE_COMPLEX && b->type == TYPE_MATRIX_COMPLEX) ||
	   (a->type == TYPE_MATRIX_COMPLEX && b->type == TYPE_COMPLEX)) {
    result.type = TYPE_MATRIX_COMPLEX;
    gsl_matrix_complex* mat =
      (a->type == TYPE_MATRIX_COMPLEX) ? a->matrix_complex : b->matrix_complex;
    gsl_complex z = (a->type == TYPE_COMPLEX) ? a->complex_val : b->complex_val;
    int scalar_first = (a->type == TYPE_COMPLEX);

    size_t rows = mat->size1, cols = mat->size2;
    result.matrix_complex = gsl_matrix_complex_alloc(rows, cols);

    if (scalar_first) {
      // complex scalar - complex matrix
      for (size_t i = 0; i < rows; ++i)
	for (size_t j = 0; j < cols; ++j) {
	  gsl_complex v = gsl_matrix_complex_get(mat, i, j);
	  gsl_matrix_complex_set(result.matrix_complex, i, j, gsl_complex_sub(z, v));
	}
    } else {
      // complex matrix - complex scalar
      for (size_t i = 0; i < rows; ++i)
	for (size_t j = 0; j < cols; ++j) {
	  gsl_complex v = gsl_matrix_complex_get(mat, i, j);
	  gsl_matrix_complex_set(result.matrix_complex, i, j, gsl_complex_sub(v, z));
	}
    }
  }
  else if (a->type == TYPE_MATRIX_REAL && b->type == TYPE_MATRIX_REAL) {
    if (a->matrix_real->size1 != b->matrix_real->size1 ||
	a->matrix_real->size2 != b->matrix_real->size2) {
      fprintf(stderr, "Matrix size mismatch in sub_top_two (real matrices).\n");
      return;
    }
    result.type = TYPE_MATRIX_REAL;
    result.matrix_real = gsl_matrix_alloc(a->matrix_real->size1, a->matrix_real->size2);
    gsl_matrix_memcpy(result.matrix_real, a->matrix_real);
    gsl_matrix_sub(result.matrix_real, b->matrix_real);
  }
  else if (a->type == TYPE_MATRIX_COMPLEX && b->type == TYPE_MATRIX_COMPLEX) {
    if (a->matrix_complex->size1 != b->matrix_complex->size1 ||
	a->matrix_complex->size2 != b->matrix_complex->size2) {
      fprintf(stderr, "Matrix size mismatch in sub_top_two (complex matrices).\n");
      return;
    }
    result.type = TYPE_MATRIX_COMPLEX;
    result.matrix_complex =
      gsl_matrix_complex_alloc(a->matrix_complex->size1, a->matrix_complex->size2);
    gsl_matrix_complex_memcpy(result.matrix_complex, a->matrix_complex);
    gsl_matrix_complex_sub(result.matrix_complex, b->matrix_complex);
  }
  else {
    fprintf(stderr, "Unsupported operand types in sub_top_two.\n");
    return;
  }

  // Free memory in a
  if (a->type == TYPE_MATRIX_REAL && a->matrix_real)
    gsl_matrix_free(a->matrix_real);
  if (a->type == TYPE_MATRIX_COMPLEX && a->matrix_complex)
    gsl_matrix_complex_free(a->matrix_complex);

  // Store result and pop
  *a = result;
  stack->top--;
}


void mul_top_two(Stack* stack) {
  if (stack->top < 1) {
    fprintf(stderr, "Stack underflow in mul_top_two.\n");
    return;
  }

  stack_element* a = &stack->items[stack->top - 1]; // second from top
  stack_element* b = &stack->items[stack->top];     // top
  stack_element result = {0};

  // Scalar * Scalar
  if (a->type == TYPE_REAL && b->type == TYPE_REAL) {
    result.type = TYPE_REAL;
    result.real = a->real * b->real;
  }
  else if ((a->type == TYPE_REAL && b->type == TYPE_COMPLEX) ||
	   (a->type == TYPE_COMPLEX && b->type == TYPE_REAL)) {
    result.type = TYPE_COMPLEX;
    gsl_complex za = (a->type == TYPE_REAL) ? gsl_complex_rect(a->real, 0) : a->complex_val;
    gsl_complex zb = (b->type == TYPE_REAL) ? gsl_complex_rect(b->real, 0) : b->complex_val;
    result.complex_val = gsl_complex_mul(za, zb);
  }
  else if (a->type == TYPE_COMPLEX && b->type == TYPE_COMPLEX) {
    result.type = TYPE_COMPLEX;
    result.complex_val = gsl_complex_mul(a->complex_val, b->complex_val);
  }

  // Real scalar * Real matrix
  else if ((a->type == TYPE_REAL && b->type == TYPE_MATRIX_REAL) ||
	   (a->type == TYPE_MATRIX_REAL && b->type == TYPE_REAL)) {
    result.type = TYPE_MATRIX_REAL;
    double scalar = (a->type == TYPE_REAL) ? a->real : b->real;
    gsl_matrix* mat = (a->type == TYPE_MATRIX_REAL) ? a->matrix_real : b->matrix_real;

    size_t rows = mat->size1, cols = mat->size2;
    result.matrix_real = gsl_matrix_alloc(rows, cols);
    gsl_matrix_memcpy(result.matrix_real, mat);
    gsl_matrix_scale(result.matrix_real, scalar);
  }

  // Real scalar * Complex matrix
  else if ((a->type == TYPE_REAL && b->type == TYPE_MATRIX_COMPLEX) ||
	   (a->type == TYPE_MATRIX_COMPLEX && b->type == TYPE_REAL)) {
    result.type = TYPE_MATRIX_COMPLEX;
    double scalar = (a->type == TYPE_REAL) ? a->real : b->real;
    gsl_matrix_complex* mat =
      (a->type == TYPE_MATRIX_COMPLEX) ? a->matrix_complex : b->matrix_complex;

    size_t rows = mat->size1, cols = mat->size2;
    result.matrix_complex = gsl_matrix_complex_alloc(rows, cols);
    gsl_matrix_complex_memcpy(result.matrix_complex, mat);
    gsl_matrix_complex_scale(result.matrix_complex, gsl_complex_rect(scalar, 0.0));
  }

  // Complex scalar * Real matrix -> Complex matrix
  else if ((a->type == TYPE_COMPLEX && b->type == TYPE_MATRIX_REAL) ||
	   (a->type == TYPE_MATRIX_REAL && b->type == TYPE_COMPLEX)) {
    result.type = TYPE_MATRIX_COMPLEX;
    gsl_complex z = (a->type == TYPE_COMPLEX) ? a->complex_val : b->complex_val;
    gsl_matrix* mat_real = (a->type == TYPE_MATRIX_REAL) ? a->matrix_real : b->matrix_real;

    size_t rows = mat_real->size1, cols = mat_real->size2;
    result.matrix_complex = gsl_matrix_complex_alloc(rows, cols);

    for (size_t i = 0; i < rows; ++i) {
      for (size_t j = 0; j < cols; ++j) {
	double val = gsl_matrix_get(mat_real, i, j);
	gsl_complex val_c = gsl_complex_rect(val, 0.0);
	gsl_matrix_complex_set(result.matrix_complex, i, j, gsl_complex_mul(z, val_c));
      }
    }
  }

  // Complex scalar * Complex matrix
  else if ((a->type == TYPE_COMPLEX && b->type == TYPE_MATRIX_COMPLEX) ||
	   (a->type == TYPE_MATRIX_COMPLEX && b->type == TYPE_COMPLEX)) {
    result.type = TYPE_MATRIX_COMPLEX;
    gsl_complex z = (a->type == TYPE_COMPLEX) ? a->complex_val : b->complex_val;
    gsl_matrix_complex* mat =
      (a->type == TYPE_MATRIX_COMPLEX) ? a->matrix_complex : b->matrix_complex;

    size_t rows = mat->size1, cols = mat->size2;
    result.matrix_complex = gsl_matrix_complex_alloc(rows, cols);

    for (size_t i = 0; i < rows; ++i) {
      for (size_t j = 0; j < cols; ++j) {
	gsl_complex v = gsl_matrix_complex_get(mat, i, j);
	gsl_matrix_complex_set(result.matrix_complex, i, j, gsl_complex_mul(z, v));
      }
    }
  }

  // Real matrix * Real matrix
  else if (a->type == TYPE_MATRIX_REAL && b->type == TYPE_MATRIX_REAL) {
    if (a->matrix_real->size2 != b->matrix_real->size1) {
      fprintf(stderr, "Dimension mismatch for real matrix multiplication.\n");
      return;
    }

    result.type = TYPE_MATRIX_REAL;
    result.matrix_real = gsl_matrix_alloc(a->matrix_real->size1, b->matrix_real->size2);
    gsl_blas_dgemm(CblasNoTrans, CblasNoTrans,
		   1.0, a->matrix_real, b->matrix_real,
		   0.0, result.matrix_real);
  }

  // Complex matrix * Complex matrix
  else if (a->type == TYPE_MATRIX_COMPLEX && b->type == TYPE_MATRIX_COMPLEX) {
    if (a->matrix_complex->size2 != b->matrix_complex->size1) {
      fprintf(stderr, "Dimension mismatch for complex matrix multiplication.\n");
      return;
    }

    result.type = TYPE_MATRIX_COMPLEX;
    result.matrix_complex =
      gsl_matrix_complex_alloc(a->matrix_complex->size1, b->matrix_complex->size2);
    gsl_blas_zgemm(CblasNoTrans, CblasNoTrans,
		   GSL_COMPLEX_ONE, a->matrix_complex, b->matrix_complex,
		   GSL_COMPLEX_ZERO, result.matrix_complex);
  }

  else {
    fprintf(stderr, "Unsupported operand types in mul_top_two.\n");
    return;
  }

  // Free memory in a
  if (a->type == TYPE_MATRIX_REAL && a->matrix_real)
    gsl_matrix_free(a->matrix_real);
  if (a->type == TYPE_MATRIX_COMPLEX && a->matrix_complex)
    gsl_matrix_complex_free(a->matrix_complex);

  // Store result in-place and pop b
  *a = result;
  stack->top--;
}


void div_top_two(Stack* stack) {
  if (stack->top < 1) {
    fprintf(stderr, "Stack underflow in div_top_two.\n");
    return;
  }

  stack_element* a = &stack->items[stack->top - 1]; // numerator
  stack_element* b = &stack->items[stack->top];     // denominator
  stack_element result = {0};

  // ---- Scalar ÷ Scalar ----
  if (a->type == TYPE_REAL && b->type == TYPE_REAL) {
    result.type = TYPE_REAL;
    result.real = a->real / b->real;
  }
  else if ((a->type == TYPE_REAL && b->type == TYPE_COMPLEX) ||
	   (a->type == TYPE_COMPLEX && b->type == TYPE_REAL)) {
    gsl_complex za = (a->type == TYPE_REAL) ? gsl_complex_rect(a->real, 0) : a->complex_val;
    gsl_complex zb = (b->type == TYPE_REAL) ? gsl_complex_rect(b->real, 0) : b->complex_val;
    result.type = TYPE_COMPLEX;
    result.complex_val = gsl_complex_div(za, zb);
  }
  else if (a->type == TYPE_COMPLEX && b->type == TYPE_COMPLEX) {
    result.type = TYPE_COMPLEX;
    //    result.complex_val = gsl_complex_div(a->complex_val, b->complex_val);
    result.complex_val = gsl_complex_div(a->complex_val, b->complex_val);
  }

  // ---- Matrix ÷ Scalar (real or complex) ----
  else if ((a->type == TYPE_MATRIX_REAL && b->type == TYPE_REAL) ||
	   (a->type == TYPE_REAL && b->type == TYPE_MATRIX_REAL)) {
    result.type = TYPE_MATRIX_REAL;
    gsl_matrix* mat = (a->type == TYPE_MATRIX_REAL) ? a->matrix_real : b->matrix_real;
    double scalar = (a->type == TYPE_REAL) ? a->real : b->real;

    size_t rows = mat->size1, cols = mat->size2;
    result.matrix_real = gsl_matrix_alloc(rows, cols);

    if (a->type == TYPE_MATRIX_REAL) {
      // Matrix ÷ scalar
      gsl_matrix_memcpy(result.matrix_real, mat);
      gsl_matrix_scale(result.matrix_real, 1.0 / scalar);
    } else {
      // Scalar ÷ Matrix: scalar divided by each element
      for (size_t i = 0; i < rows; ++i)
	for (size_t j = 0; j < cols; ++j) {
	  double val = gsl_matrix_get(mat, i, j);
	  gsl_matrix_set(result.matrix_real, i, j, scalar / val);
	}
    }
  }
  else if ((a->type == TYPE_MATRIX_REAL && b->type == TYPE_COMPLEX) ||
	   (a->type == TYPE_COMPLEX && b->type == TYPE_MATRIX_REAL)) {
    result.type = TYPE_MATRIX_COMPLEX;
    gsl_matrix* mat_real = (a->type == TYPE_MATRIX_REAL) ? a->matrix_real : b->matrix_real;
    gsl_complex scalar = (a->type == TYPE_COMPLEX)
      ? a->complex_val
      : gsl_complex_div(gsl_complex_rect(1.0, 0.0), b->complex_val);

    size_t rows = mat_real->size1, cols = mat_real->size2;
    result.matrix_complex = gsl_matrix_complex_alloc(rows, cols);

    if (a->type == TYPE_MATRIX_REAL) {
      // Real matrix ÷ complex scalar
      for (size_t i = 0; i < rows; ++i)
	for (size_t j = 0; j < cols; ++j) {
	  double val = gsl_matrix_get(mat_real, i, j);
	  gsl_matrix_complex_set(result.matrix_complex, i, j,
				 gsl_complex_div(gsl_complex_rect(val, 0.0), scalar));
	}
    } else {
      // Complex scalar ÷ real matrix
      for (size_t i = 0; i < rows; ++i)
	for (size_t j = 0; j < cols; ++j) {
	  double val = gsl_matrix_get(mat_real, i, j);
	  gsl_matrix_complex_set(result.matrix_complex, i, j,
				 gsl_complex_div(scalar, gsl_complex_rect(val, 0.0)));
	}
    }
  }
  else if ((a->type == TYPE_MATRIX_COMPLEX && b->type == TYPE_REAL) ||
	   (a->type == TYPE_REAL && b->type == TYPE_MATRIX_COMPLEX)) {
    result.type = TYPE_MATRIX_COMPLEX;
    gsl_matrix_complex* mat_complex =
      (a->type == TYPE_MATRIX_COMPLEX) ? a->matrix_complex : b->matrix_complex;
    gsl_complex scalar =
      (a->type == TYPE_REAL) ? gsl_complex_rect(a->real, 0.0) : gsl_complex_rect(1.0, 0.0);

    size_t rows = mat_complex->size1, cols = mat_complex->size2;
    result.matrix_complex = gsl_matrix_complex_alloc(rows, cols);

    if (a->type == TYPE_MATRIX_COMPLEX) {
      // Complex matrix ÷ real scalar
      gsl_matrix_complex_memcpy(result.matrix_complex, mat_complex);
      gsl_matrix_complex_scale(result.matrix_complex, gsl_complex_rect(1.0 / b->real, 0.0));
    } else {
      // Real scalar ÷ complex matrix
      for (size_t i = 0; i < rows; ++i)
	for (size_t j = 0; j < cols; ++j) {
	  gsl_complex val = gsl_matrix_complex_get(mat_complex, i, j);
	  gsl_matrix_complex_set(result.matrix_complex, i, j, gsl_complex_div(scalar, val));
	}
    }
  }
  else if (a->type == TYPE_MATRIX_COMPLEX && b->type == TYPE_COMPLEX) {
    // ---- Complex matrix ÷ complex scalar ----
    result.type = TYPE_MATRIX_COMPLEX;
    size_t rows = a->matrix_complex->size1, cols = a->matrix_complex->size2;
    result.matrix_complex = gsl_matrix_complex_alloc(rows, cols);

    for (size_t i = 0; i < rows; ++i)
      for (size_t j = 0; j < cols; ++j) {
	gsl_complex val = gsl_matrix_complex_get(a->matrix_complex, i, j);
	gsl_matrix_complex_set(result.matrix_complex, i, j, gsl_complex_div(val, b->complex_val));
      }
  }

  // ---- Matrix ÷ Matrix (A * inv(B)) ----
  else if ((a->type == TYPE_MATRIX_REAL && b->type == TYPE_MATRIX_REAL) ||
	   (a->type == TYPE_MATRIX_COMPLEX && b->type == TYPE_MATRIX_COMPLEX)) {

    if ((b->type == TYPE_MATRIX_REAL && b->matrix_real->size1 != b->matrix_real->size2) ||
	(b->type == TYPE_MATRIX_COMPLEX && b->matrix_complex->size1 != b->matrix_complex->size2)) {
      fprintf(stderr, "Matrix divisor must be square for inversion.\n");
      return;
    }

    if (a->type == TYPE_MATRIX_REAL && b->type == TYPE_MATRIX_REAL) {
      gsl_matrix* binv = gsl_matrix_alloc(b->matrix_real->size1, b->matrix_real->size2);
      gsl_permutation* p = gsl_permutation_alloc(b->matrix_real->size1);
      int signum;
      gsl_matrix* bcopy = gsl_matrix_alloc(b->matrix_real->size1, b->matrix_real->size2);
      gsl_matrix_memcpy(bcopy, b->matrix_real);
      gsl_linalg_LU_decomp(bcopy, p, &signum);
      gsl_linalg_LU_invert(bcopy, p, binv);

      result.type = TYPE_MATRIX_REAL;
      result.matrix_real = gsl_matrix_alloc(a->matrix_real->size1, binv->size2);
      gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, a->matrix_real, binv, 0.0, result.matrix_real);

      gsl_matrix_free(binv);
      gsl_matrix_free(bcopy);
      gsl_permutation_free(p);
    }
    else if (a->type == TYPE_MATRIX_COMPLEX && b->type == TYPE_MATRIX_COMPLEX) {
      gsl_matrix_complex* binv =
	gsl_matrix_complex_alloc(b->matrix_complex->size1, b->matrix_complex->size2);
      gsl_permutation* p = gsl_permutation_alloc(b->matrix_complex->size1);
      int signum;
      gsl_matrix_complex* bcopy =
	gsl_matrix_complex_alloc(b->matrix_complex->size1, b->matrix_complex->size2);
      gsl_matrix_complex_memcpy(bcopy, b->matrix_complex);
      gsl_linalg_complex_LU_decomp(bcopy, p, &signum);
      gsl_linalg_complex_LU_invert(bcopy, p, binv);

      result.type = TYPE_MATRIX_COMPLEX;
      result.matrix_complex = gsl_matrix_complex_alloc(a->matrix_complex->size1, binv->size2);
      gsl_blas_zgemm(CblasNoTrans, CblasNoTrans,
		     GSL_COMPLEX_ONE, a->matrix_complex, binv,
		     GSL_COMPLEX_ZERO, result.matrix_complex);

      gsl_matrix_complex_free(binv);
      gsl_matrix_complex_free(bcopy);
      gsl_permutation_free(p);
    }
  }

  else {
    fprintf(stderr, "Unsupported operand types in div_top_two.\n");
    return;
  }

  // ---- Cleanup and finalize ----
  if (a->type == TYPE_MATRIX_REAL && a->matrix_real)
    gsl_matrix_free(a->matrix_real);
  if (a->type == TYPE_MATRIX_COMPLEX && a->matrix_complex)
    gsl_matrix_complex_free(a->matrix_complex);

  *a = result;
  stack->top--;
}

void pow_top_two(Stack* stack) {
  if (stack->top < 1) {
    fprintf(stderr, "Stack underflow in pow_top_two.\n");
    return;
  }

  stack_element* a = &stack->items[stack->top - 1]; // base
  stack_element* b = &stack->items[stack->top];     // exponent
  stack_element result = {0};

  // ---- Scalar power ----
  if (a->type == TYPE_REAL && b->type == TYPE_REAL) {
    result.type = TYPE_REAL;
    result.real = pow(a->real, b->real);
  }
  else if ((a->type == TYPE_REAL && b->type == TYPE_COMPLEX) ||
	   (a->type == TYPE_COMPLEX && b->type == TYPE_REAL) ||
	   (a->type == TYPE_COMPLEX && b->type == TYPE_COMPLEX)) {
    gsl_complex base = (a->type == TYPE_REAL) ? gsl_complex_rect(a->real, 0) : a->complex_val;
    gsl_complex exp = (b->type == TYPE_REAL) ? gsl_complex_rect(b->real, 0) : b->complex_val;
    gsl_complex log_base = gsl_complex_log(base);
    gsl_complex scale = gsl_complex_mul(log_base, exp);
    result.type = TYPE_COMPLEX;
    result.complex_val = gsl_complex_exp(scale);
  }

  // ---- Matrix ^ integer ----
  else if (a->type == TYPE_MATRIX_REAL && b->type == TYPE_REAL) {
    int n = (int)b->real;
    if (n < 0 || a->matrix_real->size1 != a->matrix_real->size2) {
      fprintf(stderr, "Matrix exponent must be non-negative and square.\n");
      return;
    }
    gsl_matrix* res = gsl_matrix_alloc(a->matrix_real->size1, a->matrix_real->size2);
    gsl_matrix_set_identity(res);

    gsl_matrix* temp = gsl_matrix_alloc(a->matrix_real->size1, a->matrix_real->size2);
    gsl_matrix_memcpy(temp, a->matrix_real);

    for (int i = 0; i < n; i++) {
      gsl_matrix* temp_res = gsl_matrix_alloc(res->size1, temp->size2);
      gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, res, temp, 0.0, temp_res);
      gsl_matrix_free(res);
      res = temp_res;
    }

    result.type = TYPE_MATRIX_REAL;
    result.matrix_real = res;
    gsl_matrix_free(temp);
  }

  else if (a->type == TYPE_MATRIX_COMPLEX && b->type == TYPE_REAL) {
    int n = (int)b->real;
    if (n < 0 || a->matrix_complex->size1 != a->matrix_complex->size2) {
      fprintf(stderr, "Matrix exponent must be non-negative and square.\n");
      return;
    }
    gsl_matrix_complex* res =
      gsl_matrix_complex_alloc(a->matrix_complex->size1, a->matrix_complex->size2);
    gsl_matrix_complex_set_identity(res);

    gsl_matrix_complex* temp =
      gsl_matrix_complex_alloc(a->matrix_complex->size1, a->matrix_complex->size2);
    gsl_matrix_complex_memcpy(temp, a->matrix_complex);

    for (int i = 0; i < n; i++) {
      gsl_matrix_complex* temp_res = gsl_matrix_complex_alloc(res->size1, temp->size2);
      gsl_blas_zgemm(CblasNoTrans, CblasNoTrans, GSL_COMPLEX_ONE,
		     res, temp, GSL_COMPLEX_ZERO, temp_res);
      gsl_matrix_complex_free(res);
      res = temp_res;
    }

    result.type = TYPE_MATRIX_COMPLEX;
    result.matrix_complex = res;
    gsl_matrix_complex_free(temp);
  }

  // ---- Unsupported case ----
  else {
    fprintf(stderr, "Unsupported types in pow_top_two.\n");
    return;
  }

  // Free a’s previous data
  if (a->type == TYPE_MATRIX_REAL && a->matrix_real)
    gsl_matrix_free(a->matrix_real);
  if (a->type == TYPE_MATRIX_COMPLEX && a->matrix_complex)
    gsl_matrix_complex_free(a->matrix_complex);

  // Store result in a and pop b
  *a = result;
  stack->top--;
}

void join_2_reals(Stack *s) {
  if (s->top < 1) {
    fprintf(stderr, "Error: need at least two elements to join.\n");
    return;
  }

  stack_element *imag_elem = &s->items[s->top];
  stack_element *real_elem = &s->items[s->top - 1];

  // Handle real scalars
  if (real_elem->type == TYPE_REAL && imag_elem->type == TYPE_REAL) {
    double real_part = real_elem->real;
    double imag_part = imag_elem->real;

    // Pop imag and real scalars
    s->top -= 2;

    // Push complex scalar
    if (s->top + 1 >= STACK_SIZE) {
      fprintf(stderr, "Error: stack overflow when pushing complex scalar.\n");
      return;
    }

    s->top++;
    stack_element *dest = &s->items[s->top];
    dest->type = TYPE_COMPLEX;
    dest->complex_val = gsl_complex_rect(real_part, imag_part);
    return;
  }

  // Handle real matrices
  if (real_elem->type == TYPE_MATRIX_REAL && imag_elem->type == TYPE_MATRIX_REAL) {
    gsl_matrix *real_mat = real_elem->matrix_real;
    gsl_matrix *imag_mat = imag_elem->matrix_real;

    if (!real_mat || !imag_mat) {
      fprintf(stderr, "Error: one of the matrices is NULL.\n");
      return;
    }

    if (real_mat->size1 != imag_mat->size1 || real_mat->size2 != imag_mat->size2) {
      fprintf(stderr, "Error: matrices must have the same dimensions to join.\n");
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
	double real_part = gsl_matrix_get(real_mat, i, j);
	double imag_part = gsl_matrix_get(imag_mat, i, j);
	gsl_complex z = gsl_complex_rect(real_part, imag_part);
	gsl_matrix_complex_set(complex_mat, i, j, z);
      }
    }

    // Free the old real matrices
    gsl_matrix_free(real_mat);
    gsl_matrix_free(imag_mat);

    // Pop imag and real matrices
    s->top -= 2;

    // Push complex matrix
    if (s->top + 1 >= STACK_SIZE) {
      fprintf(stderr, "Error: stack overflow when pushing complex matrix.\n");
      gsl_matrix_complex_free(complex_mat);
      return;
    }

    s->top++;
    stack_element *dest = &s->items[s->top];
    dest->type = TYPE_MATRIX_COMPLEX;
    dest->matrix_complex = complex_mat;

    return;
  }

  // If types don't match
  fprintf(stderr, "Error: join_real expects two real scalars or two real matrices.\n");
}


int kronecker_top_two(Stack* stack) {
  if (stack->top < 1) {
    fprintf(stderr, "Stack underflow in kronecker_top_two.\n");
    return 1;
  }

  stack_element* a = &stack->items[stack->top - 1]; // first matrix
  stack_element* b = &stack->items[stack->top];     // second matrix
  stack_element result = {0};

  // Real × Real matrices
  if (a->type == TYPE_MATRIX_REAL && b->type == TYPE_MATRIX_REAL) {
    size_t a_rows = a->matrix_real->size1;
    size_t a_cols = a->matrix_real->size2;
    size_t b_rows = b->matrix_real->size1;
    size_t b_cols = b->matrix_real->size2;

    result.type = TYPE_MATRIX_REAL;
    result.matrix_real = gsl_matrix_alloc(a_rows * b_rows, a_cols * b_cols);

    for (size_t i = 0; i < a_rows; ++i) {
      for (size_t j = 0; j < a_cols; ++j) {
	double a_ij = gsl_matrix_get(a->matrix_real, i, j);
	for (size_t k = 0; k < b_rows; ++k) {
	  for (size_t l = 0; l < b_cols; ++l) {
	    double b_kl = gsl_matrix_get(b->matrix_real, k, l);
	    gsl_matrix_set(result.matrix_real, i * b_rows + k, j * b_cols + l, a_ij * b_kl);
	  }
	}
      }
    }
  }

  // Complex × Complex matrices
  else if (a->type == TYPE_MATRIX_COMPLEX && b->type == TYPE_MATRIX_COMPLEX) {
    size_t a_rows = a->matrix_complex->size1;
    size_t a_cols = a->matrix_complex->size2;
    size_t b_rows = b->matrix_complex->size1;
    size_t b_cols = b->matrix_complex->size2;

    result.type = TYPE_MATRIX_COMPLEX;
    result.matrix_complex = gsl_matrix_complex_alloc(a_rows * b_rows, a_cols * b_cols);

    for (size_t i = 0; i < a_rows; ++i) {
      for (size_t j = 0; j < a_cols; ++j) {
	gsl_complex a_ij = gsl_matrix_complex_get(a->matrix_complex, i, j);
	for (size_t k = 0; k < b_rows; ++k) {
	  for (size_t l = 0; l < b_cols; ++l) {
	    gsl_complex b_kl = gsl_matrix_complex_get(b->matrix_complex, k, l);
	    gsl_matrix_complex_set(result.matrix_complex,
				   i * b_rows + k,
				   j * b_cols + l,
				   gsl_complex_mul(a_ij, b_kl));
	  }
	}
      }
    }
  }

  // Real × Complex matrix → promote real to complex
  else if (a->type == TYPE_MATRIX_REAL && b->type == TYPE_MATRIX_COMPLEX) {
    size_t a_rows = a->matrix_real->size1;
    size_t a_cols = a->matrix_real->size2;
    size_t b_rows = b->matrix_complex->size1;
    size_t b_cols = b->matrix_complex->size2;

    result.type = TYPE_MATRIX_COMPLEX;
    result.matrix_complex = gsl_matrix_complex_alloc(a_rows * b_rows, a_cols * b_cols);

    for (size_t i = 0; i < a_rows; ++i) {
      for (size_t j = 0; j < a_cols; ++j) {
	double a_ij = gsl_matrix_get(a->matrix_real, i, j);
	gsl_complex a_complex = gsl_complex_rect(a_ij, 0.0);
	for (size_t k = 0; k < b_rows; ++k) {
	  for (size_t l = 0; l < b_cols; ++l) {
	    gsl_complex b_kl = gsl_matrix_complex_get(b->matrix_complex, k, l);
	    gsl_matrix_complex_set(result.matrix_complex,
				   i * b_rows + k,
				   j * b_cols + l,
				   gsl_complex_mul(a_complex, b_kl));
	  }
	}
      }
    }
  }

  // Complex × Real matrix → promote real to complex
  else if (a->type == TYPE_MATRIX_COMPLEX && b->type == TYPE_MATRIX_REAL) {
    size_t a_rows = a->matrix_complex->size1;
    size_t a_cols = a->matrix_complex->size2;
    size_t b_rows = b->matrix_real->size1;
    size_t b_cols = b->matrix_real->size2;

    result.type = TYPE_MATRIX_COMPLEX;
    result.matrix_complex = gsl_matrix_complex_alloc(a_rows * b_rows, a_cols * b_cols);

    for (size_t i = 0; i < a_rows; ++i) {
      for (size_t j = 0; j < a_cols; ++j) {
	gsl_complex a_ij = gsl_matrix_complex_get(a->matrix_complex, i, j);
	for (size_t k = 0; k < b_rows; ++k) {
	  for (size_t l = 0; l < b_cols; ++l) {
	    double b_kl = gsl_matrix_get(b->matrix_real, k, l);
	    gsl_complex b_complex = gsl_complex_rect(b_kl, 0.0);
	    gsl_matrix_complex_set(result.matrix_complex,
				   i * b_rows + k,
				   j * b_cols + l,
				   gsl_complex_mul(a_ij, b_complex));
	  }
	}
      }
    }
  }

  else {
    fprintf(stderr, "Unsupported operand types for kronecker product.\n");
    return 1;
  }

  // Cleanup: free first matrix if allocated
  if (a->type == TYPE_MATRIX_REAL && a->matrix_real)
    gsl_matrix_free(a->matrix_real);
  if (a->type == TYPE_MATRIX_COMPLEX && a->matrix_complex)
    gsl_matrix_complex_free(a->matrix_complex);

  *a = result;
  stack->top--;
  return 0;
}

void dot_div_top_two(Stack* stack) {
  if (stack->top < 1) {
    fprintf(stderr, "Stack underflow in dot_div_top_two.\n");
    return;
  }

  stack_element* a = &stack->items[stack->top - 1]; // second from top (numerator)
  stack_element* b = &stack->items[stack->top];     // top (denominator)
  stack_element result = {0};

  if (a->type == TYPE_REAL && b->type == TYPE_REAL) {
    result.type = TYPE_REAL;
    result.real = a->real / b->real;
  }
  else if ((a->type == TYPE_REAL && b->type == TYPE_COMPLEX) ||
           (a->type == TYPE_COMPLEX && b->type == TYPE_REAL)) {
    result.type = TYPE_COMPLEX;
    gsl_complex za = (a->type == TYPE_REAL) ? gsl_complex_rect(a->real, 0) : a->complex_val;
    gsl_complex zb = (b->type == TYPE_REAL) ? gsl_complex_rect(b->real, 0) : b->complex_val;
    result.complex_val = gsl_complex_div(za, zb);
  }
  else if (a->type == TYPE_COMPLEX && b->type == TYPE_COMPLEX) {
    result.type = TYPE_COMPLEX;
    result.complex_val = gsl_complex_div(a->complex_val, b->complex_val);
  }
  else if ((a->type == TYPE_REAL && b->type == TYPE_MATRIX_REAL) ||
           (a->type == TYPE_MATRIX_REAL && b->type == TYPE_REAL)) {
    result.type = TYPE_MATRIX_REAL;
    gsl_matrix* mat = (a->type == TYPE_MATRIX_REAL) ? a->matrix_real : b->matrix_real;
    double val = (a->type == TYPE_REAL) ? a->real : b->real;
    int scalar_first = (a->type == TYPE_REAL);
    size_t rows = mat->size1, cols = mat->size2;
    result.matrix_real = gsl_matrix_alloc(rows, cols);
    for (size_t i = 0; i < rows; ++i)
      for (size_t j = 0; j < cols; ++j) {
        double m = gsl_matrix_get(mat, i, j);
        double r = scalar_first ? val / m : m / val;
        gsl_matrix_set(result.matrix_real, i, j, r);
      }
  }
  else if ((a->type == TYPE_REAL && b->type == TYPE_MATRIX_COMPLEX) ||
           (a->type == TYPE_MATRIX_COMPLEX && b->type == TYPE_REAL)) {
    result.type = TYPE_MATRIX_COMPLEX;
    gsl_matrix_complex* mat =
      (a->type == TYPE_MATRIX_COMPLEX) ? a->matrix_complex : b->matrix_complex;
    gsl_complex z = gsl_complex_rect((a->type == TYPE_REAL) ? a->real : b->real, 0.0);
    int scalar_first = (a->type == TYPE_REAL);
    size_t rows = mat->size1, cols = mat->size2;
    result.matrix_complex = gsl_matrix_complex_alloc(rows, cols);
    for (size_t i = 0; i < rows; ++i)
      for (size_t j = 0; j < cols; ++j) {
        gsl_complex v = gsl_matrix_complex_get(mat, i, j);
        gsl_complex r = scalar_first ? gsl_complex_div(z, v) : gsl_complex_div(v, z);
        gsl_matrix_complex_set(result.matrix_complex, i, j, r);
      }
  }
  else if ((a->type == TYPE_COMPLEX && b->type == TYPE_MATRIX_REAL) ||
           (a->type == TYPE_MATRIX_REAL && b->type == TYPE_COMPLEX)) {
    result.type = TYPE_MATRIX_COMPLEX;
    gsl_matrix* mat_real = (a->type == TYPE_MATRIX_REAL) ? a->matrix_real : b->matrix_real;
    gsl_complex z = (a->type == TYPE_COMPLEX) ? a->complex_val : b->complex_val;
    int scalar_first = (a->type == TYPE_COMPLEX);
    size_t rows = mat_real->size1, cols = mat_real->size2;
    result.matrix_complex = gsl_matrix_complex_alloc(rows, cols);
    for (size_t i = 0; i < rows; ++i)
      for (size_t j = 0; j < cols; ++j) {
        gsl_complex v = gsl_complex_rect(gsl_matrix_get(mat_real, i, j), 0);
        gsl_complex r = scalar_first ? gsl_complex_div(z, v) : gsl_complex_div(v, z);
        gsl_matrix_complex_set(result.matrix_complex, i, j, r);
      }
  }
  else if ((a->type == TYPE_COMPLEX && b->type == TYPE_MATRIX_COMPLEX) ||
           (a->type == TYPE_MATRIX_COMPLEX && b->type == TYPE_COMPLEX)) {
    result.type = TYPE_MATRIX_COMPLEX;
    gsl_matrix_complex* mat =
      (a->type == TYPE_MATRIX_COMPLEX) ? a->matrix_complex : b->matrix_complex;
    gsl_complex z = (a->type == TYPE_COMPLEX) ? a->complex_val : b->complex_val;
    int scalar_first = (a->type == TYPE_COMPLEX);
    size_t rows = mat->size1, cols = mat->size2;
    result.matrix_complex = gsl_matrix_complex_alloc(rows, cols);
    for (size_t i = 0; i < rows; ++i)
      for (size_t j = 0; j < cols; ++j) {
        gsl_complex v = gsl_matrix_complex_get(mat, i, j);
        gsl_complex r = scalar_first ? gsl_complex_div(z, v) : gsl_complex_div(v, z);
        gsl_matrix_complex_set(result.matrix_complex, i, j, r);
      }
  }
  else if (a->type == TYPE_MATRIX_REAL && b->type == TYPE_MATRIX_REAL) {
    if (a->matrix_real->size1 != b->matrix_real->size1 ||
        a->matrix_real->size2 != b->matrix_real->size2) {
      fprintf(stderr, "Matrix size mismatch in dot_div_top_two (real).\n");
      return;
    }
    result.type = TYPE_MATRIX_REAL;
    size_t rows = a->matrix_real->size1, cols = a->matrix_real->size2;
    result.matrix_real = gsl_matrix_alloc(rows, cols);
    for (size_t i = 0; i < rows; ++i)
      for (size_t j = 0; j < cols; ++j)
        gsl_matrix_set(result.matrix_real, i, j,
                       gsl_matrix_get(a->matrix_real, i, j) /
                       gsl_matrix_get(b->matrix_real, i, j));
  }
  else if (a->type == TYPE_MATRIX_COMPLEX && b->type == TYPE_MATRIX_COMPLEX) {
    if (a->matrix_complex->size1 != b->matrix_complex->size1 ||
        a->matrix_complex->size2 != b->matrix_complex->size2) {
      fprintf(stderr, "Matrix size mismatch in dot_div_top_two (complex).\n");
      return;
    }
    result.type = TYPE_MATRIX_COMPLEX;
    size_t rows = a->matrix_complex->size1, cols = a->matrix_complex->size2;
    result.matrix_complex = gsl_matrix_complex_alloc(rows, cols);
    for (size_t i = 0; i < rows; ++i)
      for (size_t j = 0; j < cols; ++j) {
        gsl_complex z1 = gsl_matrix_complex_get(a->matrix_complex, i, j);
        gsl_complex z2 = gsl_matrix_complex_get(b->matrix_complex, i, j);
        gsl_matrix_complex_set(result.matrix_complex, i, j, gsl_complex_div(z1, z2));
      }
  }
  else {
    fprintf(stderr, "Unsupported operand types in dot_div_top_two.\n");
    return;
  }

  // Free memory in a
  if (a->type == TYPE_MATRIX_REAL && a->matrix_real)
    gsl_matrix_free(a->matrix_real);
  if (a->type == TYPE_MATRIX_COMPLEX && a->matrix_complex)
    gsl_matrix_complex_free(a->matrix_complex);

  // Store result and pop
  *a = result;
  stack->top--;
}

void dot_mult_top_two(Stack* stack) {
  if (stack->top < 1) {
    fprintf(stderr, "Stack underflow in dot_mult_top_two.\n");
    return;
  }

  stack_element* a = &stack->items[stack->top - 1]; // second from top
  stack_element* b = &stack->items[stack->top];     // top
  stack_element result = {0};

  if (a->type == TYPE_REAL && b->type == TYPE_REAL) {
    result.type = TYPE_REAL;
    result.real = a->real * b->real;
  }
  else if ((a->type == TYPE_REAL && b->type == TYPE_COMPLEX) ||
           (a->type == TYPE_COMPLEX && b->type == TYPE_REAL)) {
    result.type = TYPE_COMPLEX;
    gsl_complex za =
      (a->type == TYPE_REAL) ? gsl_complex_rect(a->real, 0.0) : a->complex_val;
    gsl_complex zb =
      (b->type == TYPE_REAL) ? gsl_complex_rect(b->real, 0.0) : b->complex_val;
    result.complex_val = gsl_complex_mul(za, zb);
  }
  else if (a->type == TYPE_COMPLEX && b->type == TYPE_COMPLEX) {
    result.type = TYPE_COMPLEX;
    result.complex_val = gsl_complex_mul(a->complex_val, b->complex_val);
  }
  else if ((a->type == TYPE_REAL && b->type == TYPE_MATRIX_REAL) ||
           (a->type == TYPE_MATRIX_REAL && b->type == TYPE_REAL)) {
    result.type = TYPE_MATRIX_REAL;
    gsl_matrix* mat =
      (a->type == TYPE_MATRIX_REAL) ? a->matrix_real : b->matrix_real;
    double val = (a->type == TYPE_REAL) ? a->real : b->real;
    size_t rows = mat->size1, cols = mat->size2;
    result.matrix_real = gsl_matrix_alloc(rows, cols);
    for (size_t i = 0; i < rows; ++i)
      for (size_t j = 0; j < cols; ++j)
        gsl_matrix_set(result.matrix_real, i, j, val * gsl_matrix_get(mat, i, j));
  }
  else if ((a->type == TYPE_REAL && b->type == TYPE_MATRIX_COMPLEX) ||
           (a->type == TYPE_MATRIX_COMPLEX && b->type == TYPE_REAL)) {
    result.type = TYPE_MATRIX_COMPLEX;
    gsl_matrix_complex* mat =
      (a->type == TYPE_MATRIX_COMPLEX) ? a->matrix_complex : b->matrix_complex;
    gsl_complex z =
      gsl_complex_rect((a->type == TYPE_REAL) ? a->real : b->real, 0.0);
    size_t rows = mat->size1, cols = mat->size2;
    result.matrix_complex = gsl_matrix_complex_alloc(rows, cols);
    for (size_t i = 0; i < rows; ++i)
      for (size_t j = 0; j < cols; ++j) {
        gsl_complex v = gsl_matrix_complex_get(mat, i, j);
        gsl_matrix_complex_set(result.matrix_complex, i, j, gsl_complex_mul(z, v));
      }
  }
  else if ((a->type == TYPE_COMPLEX && b->type == TYPE_MATRIX_REAL) ||
           (a->type == TYPE_MATRIX_REAL && b->type == TYPE_COMPLEX)) {
    result.type = TYPE_MATRIX_COMPLEX;
    gsl_matrix* mat_real =
      (a->type == TYPE_MATRIX_REAL) ? a->matrix_real : b->matrix_real;
    gsl_complex tmp = (a->type == TYPE_COMPLEX) ? a->complex_val : b->complex_val;
    gsl_complex z = gsl_complex_rect(GSL_REAL(tmp), GSL_IMAG(tmp));

    size_t rows = mat_real->size1, cols = mat_real->size2;
    result.matrix_complex = gsl_matrix_complex_alloc(rows, cols);
    for (size_t i = 0; i < rows; ++i)
      for (size_t j = 0; j < cols; ++j) {
        gsl_complex v = gsl_complex_rect(gsl_matrix_get(mat_real, i, j), 0);
        gsl_matrix_complex_set(result.matrix_complex, i, j, gsl_complex_mul(z, v));
      }
  }
  else if ((a->type == TYPE_COMPLEX && b->type == TYPE_MATRIX_COMPLEX) ||
           (a->type == TYPE_MATRIX_COMPLEX && b->type == TYPE_COMPLEX)) {
    result.type = TYPE_MATRIX_COMPLEX;
    gsl_matrix_complex* mat =
      (a->type == TYPE_MATRIX_COMPLEX) ? a->matrix_complex : b->matrix_complex;
    gsl_complex tmp = (a->type == TYPE_COMPLEX) ? a->complex_val : b->complex_val;
    gsl_complex z = gsl_complex_rect(GSL_REAL(tmp), GSL_IMAG(tmp));

    size_t rows = mat->size1, cols = mat->size2;
    result.matrix_complex = gsl_matrix_complex_alloc(rows, cols);
    for (size_t i = 0; i < rows; ++i)
      for (size_t j = 0; j < cols; ++j) {
        gsl_complex v = gsl_matrix_complex_get(mat, i, j);
        gsl_matrix_complex_set(result.matrix_complex, i, j, gsl_complex_mul(z, v));
      }
  }
  else if (a->type == TYPE_MATRIX_REAL && b->type == TYPE_MATRIX_REAL) {
    if (a->matrix_real->size1 != b->matrix_real->size1 ||
        a->matrix_real->size2 != b->matrix_real->size2) {
      fprintf(stderr, "Matrix size mismatch in dot_mult_top_two (real).\n");
      return;
    }
    result.type = TYPE_MATRIX_REAL;
    size_t rows = a->matrix_real->size1, cols = a->matrix_real->size2;
    result.matrix_real = gsl_matrix_alloc(rows, cols);
    for (size_t i = 0; i < rows; ++i)
      for (size_t j = 0; j < cols; ++j)
        gsl_matrix_set(result.matrix_real, i, j,
                       gsl_matrix_get(a->matrix_real, i, j) *
                       gsl_matrix_get(b->matrix_real, i, j));
  }
  else if (a->type == TYPE_MATRIX_COMPLEX && b->type == TYPE_MATRIX_COMPLEX) {
    if (a->matrix_complex->size1 != b->matrix_complex->size1 ||
        a->matrix_complex->size2 != b->matrix_complex->size2) {
      fprintf(stderr, "Matrix size mismatch in dot_mult_top_two (complex).\n");
      return;
    }
    result.type = TYPE_MATRIX_COMPLEX;
    size_t rows = a->matrix_complex->size1, cols = a->matrix_complex->size2;
    result.matrix_complex = gsl_matrix_complex_alloc(rows, cols);
    for (size_t i = 0; i < rows; ++i)
      for (size_t j = 0; j < cols; ++j) {
        gsl_complex z1 = gsl_matrix_complex_get(a->matrix_complex, i, j);
        gsl_complex z2 = gsl_matrix_complex_get(b->matrix_complex, i, j);
        gsl_matrix_complex_set(result.matrix_complex, i, j, gsl_complex_mul(z1, z2));
      }
  }
  else {
    fprintf(stderr, "Unsupported operand types in dot_mult_top_two.\n");
    return;
  }

  // Free memory in a
  if (a->type == TYPE_MATRIX_REAL && a->matrix_real)
    gsl_matrix_free(a->matrix_real);
  if (a->type == TYPE_MATRIX_COMPLEX && a->matrix_complex)
    gsl_matrix_complex_free(a->matrix_complex);

  // Store result and pop
  *a = result;
  stack->top--;
}

void dot_pow_top_two(Stack* stack) {
  if (stack->top < 1) {
    fprintf(stderr, "Stack underflow in dot_pow_top_two.\n");
    return;
  }

  stack_element* a = &stack->items[stack->top - 1]; // second from top (base)
  stack_element* b = &stack->items[stack->top];     // top (exponent)
  stack_element result = {0};

  if (a->type == TYPE_REAL && b->type == TYPE_REAL) {
    result.type = TYPE_REAL;
    result.real = pow(a->real, b->real);
  }
  else if ((a->type == TYPE_REAL || a->type == TYPE_COMPLEX) &&
           (b->type == TYPE_REAL || b->type == TYPE_COMPLEX)) {
    result.type = TYPE_COMPLEX;
    gsl_complex base = (a->type == TYPE_REAL) ? gsl_complex_rect(a->real, 0.0) : a->complex_val;
    gsl_complex exp  = (b->type == TYPE_REAL) ? gsl_complex_rect(b->real, 0.0) : b->complex_val;
    result.complex_val = gsl_complex_pow(base, exp);
  }
  else if ((a->type == TYPE_REAL && b->type == TYPE_MATRIX_REAL) ||
           (a->type == TYPE_MATRIX_REAL && b->type == TYPE_REAL)) {
    result.type = TYPE_MATRIX_REAL;
    gsl_matrix* mat = (a->type == TYPE_MATRIX_REAL) ? a->matrix_real : b->matrix_real;
    double val = (a->type == TYPE_REAL) ? a->real : b->real;
    int scalar_first = (a->type == TYPE_REAL);
    size_t rows = mat->size1, cols = mat->size2;
    result.matrix_real = gsl_matrix_alloc(rows, cols);
    for (size_t i = 0; i < rows; ++i)
      for (size_t j = 0; j < cols; ++j) {
        double base = scalar_first ? val : gsl_matrix_get(mat, i, j);
        double expo = scalar_first ? gsl_matrix_get(mat, i, j) : val;
        gsl_matrix_set(result.matrix_real, i, j, pow(base, expo));
      }
  }
  else if ((a->type == TYPE_COMPLEX && b->type == TYPE_MATRIX_REAL) ||
           (a->type == TYPE_MATRIX_REAL && b->type == TYPE_COMPLEX)) {
    result.type = TYPE_MATRIX_COMPLEX;
    gsl_matrix* mat_real = (a->type == TYPE_MATRIX_REAL) ? a->matrix_real : b->matrix_real;
    gsl_complex tmp = (a->type == TYPE_COMPLEX) ? a->complex_val : b->complex_val;
    gsl_complex z = gsl_complex_rect(GSL_REAL(tmp), GSL_IMAG(tmp));
    int scalar_first = (a->type == TYPE_COMPLEX);
    size_t rows = mat_real->size1, cols = mat_real->size2;
    result.matrix_complex = gsl_matrix_complex_alloc(rows, cols);
    for (size_t i = 0; i < rows; ++i)
      for (size_t j = 0; j < cols; ++j) {
        gsl_complex d = gsl_complex_rect(gsl_matrix_get(mat_real, i, j), 0.0);
        gsl_complex r = scalar_first ? gsl_complex_pow(z, d) : gsl_complex_pow(d, z);
        gsl_matrix_complex_set(result.matrix_complex, i, j, r);
      }
  }
  else if ((a->type == TYPE_COMPLEX && b->type == TYPE_MATRIX_COMPLEX) ||
           (a->type == TYPE_MATRIX_COMPLEX && b->type == TYPE_COMPLEX)) {
    result.type = TYPE_MATRIX_COMPLEX;
    gsl_matrix_complex* mat =
      (a->type == TYPE_MATRIX_COMPLEX) ? a->matrix_complex : b->matrix_complex;
    gsl_complex z = (a->type == TYPE_COMPLEX) ? a->complex_val : b->complex_val;
    int scalar_first = (a->type == TYPE_COMPLEX);
    size_t rows = mat->size1, cols = mat->size2;
    result.matrix_complex = gsl_matrix_complex_alloc(rows, cols);
    for (size_t i = 0; i < rows; ++i)
      for (size_t j = 0; j < cols; ++j) {
        gsl_complex w = gsl_matrix_complex_get(mat, i, j);
        gsl_complex r = scalar_first ? gsl_complex_pow(z, w)
	  : gsl_complex_pow(w, z);
        gsl_matrix_complex_set(result.matrix_complex, i, j, r);
      }
  }
  else if (a->type == TYPE_MATRIX_REAL && b->type == TYPE_MATRIX_REAL) {
    if (a->matrix_real->size1 != b->matrix_real->size1 ||
        a->matrix_real->size2 != b->matrix_real->size2) {
      fprintf(stderr, "Matrix size mismatch in dot_pow_top_two (real).\n");
      return;
    }
    result.type = TYPE_MATRIX_REAL;
    size_t rows = a->matrix_real->size1, cols = a->matrix_real->size2;
    result.matrix_real = gsl_matrix_alloc(rows, cols);
    for (size_t i = 0; i < rows; ++i)
      for (size_t j = 0; j < cols; ++j) {
        double base = gsl_matrix_get(a->matrix_real, i, j);
        double exp  = gsl_matrix_get(b->matrix_real, i, j);
        gsl_matrix_set(result.matrix_real, i, j, pow(base, exp));
      }
  }
  else if (a->type == TYPE_MATRIX_COMPLEX && b->type == TYPE_MATRIX_COMPLEX) {
    if (a->matrix_complex->size1 != b->matrix_complex->size1 ||
        a->matrix_complex->size2 != b->matrix_complex->size2) {
      fprintf(stderr, "Matrix size mismatch in dot_pow_top_two (complex).\n");
      return;
    }
    result.type = TYPE_MATRIX_COMPLEX;
    size_t rows = a->matrix_complex->size1, cols = a->matrix_complex->size2;
    result.matrix_complex = gsl_matrix_complex_alloc(rows, cols);
    for (size_t i = 0; i < rows; ++i)
      for (size_t j = 0; j < cols; ++j) {
        gsl_complex z1 = gsl_matrix_complex_get(a->matrix_complex, i, j);
        gsl_complex z2 = gsl_matrix_complex_get(b->matrix_complex, i, j);
        gsl_matrix_complex_set(result.matrix_complex, i, j, gsl_complex_pow(z1, z2));
      }
  }
  else {
    fprintf(stderr, "Unsupported operand types in dot_pow_top_two.\n");
    return;
  }

  // Free memory in a
  if (a->type == TYPE_MATRIX_REAL && a->matrix_real)
    gsl_matrix_free(a->matrix_real);
  if (a->type == TYPE_MATRIX_COMPLEX && a->matrix_complex)
    gsl_matrix_complex_free(a->matrix_complex);

  // Store result and pop
  *a = result;
  stack->top--;
}
