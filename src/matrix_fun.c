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
#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>
#include "stack.h"
#include "math_parsers.h"
#include "math_helpers.h"
#include "binary_fun.h"
#include "unary_fun.h"
#include "matrix_fun.h"

int split_matrix(Stack *s) {
  if (s->top < 0) {
    fprintf(stderr, "Error: stack underflow — no matrix on stack.\n");
    return -1;
  }

  stack_element *matrix_elem = &s->items[s->top];

  if (matrix_elem->type == TYPE_MATRIX_REAL) {
    gsl_matrix *matrix = matrix_elem->matrix_real;
    if (!matrix) {
      fprintf(stderr, "Error: null real matrix.\n");
      return -1;
    }

    // Remove matrix from the stack
    s->top--;

    // Push all elements in row-major order
    for (size_t i = 0; i < matrix->size1; ++i) {
      for (size_t j = 0; j < matrix->size2; ++j) {
        double val = gsl_matrix_get(matrix, i, j);

        if (s->top >= STACK_SIZE - 1) {
          fprintf(stderr, "Error: stack overflow.\n");
          return -1;
        }

        s->top++;
        s->items[s->top].type = TYPE_REAL;
        s->items[s->top].real = val;
      }
    }
  } else if (matrix_elem->type == TYPE_MATRIX_COMPLEX) {
    gsl_matrix_complex *matrix = matrix_elem->matrix_complex;
    if (!matrix) {
      fprintf(stderr, "Error: null complex matrix.\n");
      return -1;
    }

    // Remove matrix from the stack
    s->top--;

    // Push all elements in row-major order
    for (size_t i = 0; i < matrix->size1; ++i) {
      for (size_t j = 0; j < matrix->size2; ++j) {
        gsl_complex z = gsl_matrix_complex_get(matrix, i, j);

        if (s->top >= STACK_SIZE - 1) {
          fprintf(stderr, "Error: stack overflow.\n");
          return -1;
        }

        s->top++;
        s->items[s->top].type = TYPE_COMPLEX;
        s->items[s->top].complex_val = z;
      }
    }
  } else {
    fprintf(stderr, "Error: top of stack is not a matrix.\n");
    return -1;
  }

  return 0;
}

int select_matrix_element(Stack *s) {
  if (s->top < 1) {
    fprintf(stderr, "Error: insufficient elements on stack (need at least two: row, col).\n");
    return -1;  // Indicate error
  }

  // Pop row and column indices
  stack_element *col_elem = &s->items[s->top];
  stack_element *row_elem = &s->items[s->top - 1];
  s->top -= 2;

  if (row_elem->type != TYPE_REAL || col_elem->type != TYPE_REAL) {
    fprintf(stderr, "Error: row and column must be real scalars.\n");
    return -1;  // Indicate error
  }

  // Extract row and column numbers
  size_t row = (size_t)row_elem->real;
  size_t col = (size_t)col_elem->real;

  // Now look at the element on top of the stack (matrix)
  stack_element *matrix_elem = &s->items[s->top];
  if (matrix_elem->type == TYPE_MATRIX_REAL) {
    gsl_matrix *matrix = matrix_elem->matrix_real;
    if (!matrix || row >= matrix->size1 || col >= matrix->size2) {
      fprintf(stderr, "Error: invalid row or column index.\n");
      return -1;  // Indicate error
    }

    // Access the element in the real matrix
    double value = gsl_matrix_get(matrix, row, col);

    // Push the scalar onto the stack
    s->top++;
    stack_element *result_elem = &s->items[s->top];
    result_elem->type = TYPE_REAL;
    result_elem->real = value;
  } else if (matrix_elem->type == TYPE_MATRIX_COMPLEX) {
    gsl_matrix_complex *matrix = matrix_elem->matrix_complex;
    if (!matrix || row >= matrix->size1 || col >= matrix->size2) {
      fprintf(stderr, "Error: invalid row or column index.\n");
      return -1;  // Indicate error
    }

    // Access the element in the complex matrix
    gsl_complex value = gsl_matrix_complex_get(matrix, row, col);

    // Push the complex scalar onto the stack
    s->top++;
    stack_element *result_elem = &s->items[s->top];
    result_elem->type = TYPE_COMPLEX;
    result_elem->complex_val = value;
  } else {
    fprintf(stderr, "Error: top element is not a matrix.\n");
    return -1;  // Indicate error
  }

  return 0;  // Success
}

int set_matrix_element(Stack *s) {
  if (s->top < 3) {
    fprintf(stderr, "Error: insufficient elements on stack (need value, row, col, and matrix).\n");
    return -1;
  }

  // Top stack layout (high to low):
  stack_element *val_elem = &s->items[s->top - 3];   // [top - 3] -> value (scalar)
  stack_element *col_elem = &s->items[s->top - 2];   // [top - 2] -> col (real scalar)
  stack_element *row_elem = &s->items[s->top - 1];   // [top - 1] -> row (real scalar)
  stack_element *matrix_elem = &s->items[s->top];    // [top - 0] -> matrix (real or complex)

  if (row_elem->type != TYPE_REAL || col_elem->type != TYPE_REAL) {
    fprintf(stderr, "Error: row and column must be real scalars.\n");
    return -1;
  }

  size_t row = (size_t)row_elem->real;
  size_t col = (size_t)col_elem->real;

  if (matrix_elem->type == TYPE_MATRIX_REAL) {
    gsl_matrix *matrix = matrix_elem->matrix_real;
    if (!matrix || row >= matrix->size1 || col >= matrix->size2) {
      fprintf(stderr, "Error: invalid row or column index for real matrix.\n");
      return -1;
    }

    if (val_elem->type != TYPE_REAL) {
      fprintf(stderr, "Error: cannot set real matrix with non-real value.\n");
      return -1;
    }

    printf("rows %zu, cols %zu, element %g\n",row, col, val_elem->real);   
    gsl_matrix_set(matrix, row, col, val_elem->real);

  } else if (matrix_elem->type == TYPE_MATRIX_COMPLEX) {
    gsl_matrix_complex *matrix = matrix_elem->matrix_complex;
    if (!matrix || row >= matrix->size1 || col >= matrix->size2) {
      fprintf(stderr, "Error: invalid row or column index for complex matrix.\n");
      return -1;
    }

    gsl_complex value;
    if (val_elem->type == TYPE_COMPLEX) {
      value = val_elem->complex_val;
    } else if (val_elem->type == TYPE_REAL) {
      GSL_SET_COMPLEX(&value, val_elem->real, 0.0);
    } else {
      fprintf(stderr, "Error: invalid value type for complex matrix.\n");
      return -1;
    }

    gsl_matrix_complex_set(matrix, row, col, value);

  } else {
    fprintf(stderr, "Error: top-3 element is not a matrix.\n");
    return -1;
  }
  return 0;
}

int matrix_extract_diagonal(Stack* stack) {
  if (stack->top < 0) {
    printf("No matrix on stack to extract diagonal from\n");
    return 1;
  }

  stack_element m = pop(stack);

  if (m.type == TYPE_MATRIX_REAL) {
    size_t n =
      (m.matrix_real->size1 < m.matrix_real->size2) ? m.matrix_real->size1 : m.matrix_real->size2;

    gsl_matrix* diag = gsl_matrix_calloc(1, n);
    for (size_t i = 0; i < n; ++i) {
      double val = gsl_matrix_get(m.matrix_real, i, i);
      gsl_matrix_set(diag, 0, i, val);
    }

    gsl_matrix_free(m.matrix_real);
    push_matrix_real(stack, diag);

  } else if (m.type == TYPE_MATRIX_COMPLEX) {
    size_t n =
      (m.matrix_complex->size1 < m.matrix_complex->size2) ? m.matrix_complex->size1 : m.matrix_complex->size2;

    gsl_matrix_complex* diag = gsl_matrix_complex_calloc(1, n);
    for (size_t i = 0; i < n; ++i) {
      gsl_complex z = gsl_matrix_complex_get(m.matrix_complex, i, i);
      gsl_matrix_complex_set(diag, 0, i, z);
    }

    gsl_matrix_complex_free(m.matrix_complex);
    push_matrix_complex(stack, diag);

  } else {
    printf("Only real or complex matrices are supported for diagonal extraction\n");
    return 1;
  }
  return 0;
}

int make_unit_matrix(Stack* stack) {
  if (stack->top < 0) {
    fprintf(stderr, "Stack underflow: no dimension to create unit matrix.\n");
    return 1;
  }

  value_type top_type=stack_top_type(stack);

  if (top_type != TYPE_REAL) {
    fprintf(stderr, "Type error: top stack item must be a real number (dimension).\n");
    return 1;
  }

  stack_element dim_item = pop(stack);

  int n = (int)(dim_item.real);
  if (n <= 0) {
    fprintf(stderr, "Dimension must be positive, got %d.\n", n);
    return 1;
  }

  gsl_matrix* m = gsl_matrix_calloc(n, n); // allocates and zeroes the matrix
  if (!m) {
    fprintf(stderr, "Failed to allocate matrix.\n");
    return 1;
  }

  for (int i = 0; i < n; ++i) {
    gsl_matrix_set(m, i, i, 1.0);  // Set diagonal to 1
  }

  push_matrix_real(stack, m);
  return 0;
}

int make_row_range(Stack* stack) {
  if (stack->top < 0) {
    fprintf(stderr, "Stack underflow: you need one dimension to create the row matrix.\n");
    return 1;
  }

  value_type cols_top_type=stack_top_type(stack);
    
  if (cols_top_type != TYPE_REAL) {
    fprintf(stderr, "Type error: top stack item must be a real number (dimension).\n");
    return 1;
  }

  stack_element cols = pop(stack);
  int num_cols = (int)(cols.real);
  if (num_cols <= 0) {
    fprintf(stderr, "Dimension must be positive, got %d.\n", num_cols);
    return 1;
  }

  gsl_matrix* mat = gsl_matrix_calloc(1, num_cols); // allocates and zeroes the matrix
  if (!mat) {
    fprintf(stderr, "Failed to allocate matrix.\n");
    return 1;
  }
  for(int i=0; i < num_cols; i++)
    gsl_matrix_set(mat, 0, i, (double)i);
    
  push_matrix_real(stack, mat);
  return 0;
}


int make_matrix_of_ones(Stack* stack) {
  if (stack->top < 1) {
    fprintf(stderr, "Stack underflow: need two dimensions to create the matrix.\n");
    return 1;
  }

  value_type rows_top_type=stack_top_type(stack);
  value_type cols_top_type=stack_top_type(stack);
    
  if ((rows_top_type != TYPE_REAL) || (cols_top_type != TYPE_REAL)) {
    fprintf(stderr, "Type error: top stack item must be a real number (dimension).\n");
    return 1;
  }

  stack_element cols = pop(stack);
  stack_element rows = pop(stack);
    
  int n = (int)(rows.real);
  if (n <= 0) {
    fprintf(stderr, "Dimension must be positive, got %d.\n", n);
    return 1;
  }

  int m = (int)(cols.real);
  if (n <= 0) {
    fprintf(stderr, "Dimension must be positive, got %d.\n", m);
    return 1;
  }

  gsl_matrix* mat = gsl_matrix_calloc(n, m); // allocates and zeroes the matrix
  if (!mat) {
    fprintf(stderr, "Failed to allocate matrix.\n");
    return 1;
  }

  gsl_matrix_set_all(mat, 1.0);
    
  push_matrix_real(stack, mat);
  return 0;
}


int make_matrix_of_zeroes(Stack* stack) {
  if (stack->top < 1) {
    fprintf(stderr, "Stack underflow: need two dimensions to create the matrix.\n");
    return 1;
  }

  value_type rows_top_type=stack_top_type(stack);
  value_type cols_top_type=stack_top_type(stack);
    
  if ((rows_top_type != TYPE_REAL) || (cols_top_type != TYPE_REAL)) {
    fprintf(stderr, "Type error: top stack item must be a real number (dimension).\n");
    return 1;
  }

  stack_element cols = pop(stack);
  stack_element rows = pop(stack);
    
  int n = (int)(rows.real);
  if (n <= 0) {
    fprintf(stderr, "Dimension must be positive, got %d.\n", n);
    return 1;
  }

  int m = (int)(cols.real);
  if (n <= 0) {
    fprintf(stderr, "Dimension must be positive, got %d.\n", m);
    return 1;
  }

  gsl_matrix* mat = gsl_matrix_calloc(n, m); // allocates and zeroes the matrix
  if (!mat) {
    fprintf(stderr, "Failed to allocate matrix.\n");
    return 1;
  }
  push_matrix_real(stack, mat);
  return 0;
}

extern gsl_rng* global_rng;  // Assume you initialize this elsewhere

int make_random_matrix(Stack* stack) {
  if (stack->top < 1) {
    fprintf(stderr, "Stack underflow: need two dimensions to create the matrix.\n");
    return 1;
  }

  value_type cols_top_type = stack_top_type(stack);
  value_type rows_top_type = stack_top_type(stack);

  if ((rows_top_type != TYPE_REAL) || (cols_top_type != TYPE_REAL)) {
    fprintf(stderr, "Type error: top stack items must be real numbers (dimensions).\n");
    return 1;
  }

  stack_element cols = pop(stack);
  stack_element rows = pop(stack);

  int n = (int)(rows.real);
  int m = (int)(cols.real);

  if (n <= 0 || m <= 0) {
    fprintf(stderr, "Dimensions must be positive, got %d x %d.\n", n, m);
    return 1;
  }

  //    gsl_rng * rng = gsl_rng_alloc(gsl_rng_mt19937);
    
  gsl_matrix* mat = gsl_matrix_alloc(n, m);  // Allocate uninitialized matrix
  if (!mat) {
    fprintf(stderr, "Failed to allocate matrix.\n");
    return 1;
  }

  for (int i = 0; i < n; ++i) {
    for (int j = 0; j < m; ++j) {
      double u = gsl_rng_uniform(global_rng); // uniform [0,1)
      gsl_matrix_set(mat, i, j, u);
    }
  }
  push_matrix_real(stack, mat);
  return 0;
}

int make_gaussian_random_matrix(Stack* stack) {
  if (stack->top < 1) {
    fprintf(stderr, "Stack underflow: need two dimensions to create the matrix.\n");
    return 1;
  }

  value_type cols_top_type = stack_top_type(stack);
  value_type rows_top_type = stack_top_type(stack);

  if ((rows_top_type != TYPE_REAL) || (cols_top_type != TYPE_REAL)) {
    fprintf(stderr, "Type error: top stack items must be real numbers (dimensions).\n");
    return 1;
  }

  stack_element cols = pop(stack);
  stack_element rows = pop(stack);

  int n = (int)(rows.real);
  int m = (int)(cols.real);

  if (n <= 0 || m <= 0) {
    fprintf(stderr, "Dimensions must be positive, got %d x %d.\n", n, m);
    return 1;
  }
  gsl_matrix* mat = gsl_matrix_alloc(n, m);  // Allocate uninitialized matrix
  if (!mat) {
    fprintf(stderr, "Failed to allocate matrix.\n");
    return 1;
  }

  for (int i = 0; i < n; ++i) {
    for (int j = 0; j < m; ++j) {
      double u = gsl_ran_gaussian(global_rng,1.0); // uniform [0,1)
      gsl_matrix_set(mat, i, j, u);
    }
  }
  push_matrix_real(stack, mat);
  return 0;
}

int matrix_dimensions(Stack* stack) {
    if (stack->top < 0) {
        fprintf(stderr, "Stack underflow: need a matrix to get dimensions.\n");
        return 1;
    }

    stack_element* top_elem = &stack->items[stack->top];

    size_t rows = 0;
    size_t cols = 0;

    if (top_elem->type == TYPE_MATRIX_REAL) {
        if (!top_elem->matrix_real) {
            fprintf(stderr, "Real matrix pointer is NULL.\n");
            return 1;
        }
        rows = top_elem->matrix_real->size1;
        cols = top_elem->matrix_real->size2;
    } else if (top_elem->type == TYPE_MATRIX_COMPLEX) {
        if (!top_elem->matrix_complex) {
            fprintf(stderr, "Complex matrix pointer is NULL.\n");
            return 1;
        }
        rows = top_elem->matrix_complex->size1;
        cols = top_elem->matrix_complex->size2;
    } else {
        fprintf(stderr, "Type error: top stack item is not a matrix.\n");
        return 1;
    }

    push_real(stack, (double)rows);
    push_real(stack, (double)cols);
    return 0;
}

int reshape_matrix(Stack* stack) {
    if (stack->top < 2) {
        fprintf(stderr, "Stack underflow: need matrix and two dimensions.\n");
        return 1;
    }

    // Get new cols and rows (top two elements)
    stack_element cols_elem = stack->items[stack->top--];
    stack_element rows_elem = stack->items[stack->top--];

    if (cols_elem.type != TYPE_REAL || rows_elem.type != TYPE_REAL) {
        fprintf(stderr, "Type error: expected real numbers for new dimensions.\n");
        stack->top += 2; // Restore stack
        return 1;
    }

    int new_rows = (int)rows_elem.real;
    int new_cols = (int)cols_elem.real;

    if (new_rows <= 0 || new_cols <= 0) {
        fprintf(stderr, "Invalid reshape dimensions: must be positive integers.\n");
        return 1;
    }

    stack_element* mat_elem = &stack->items[stack->top];

    if (mat_elem->type == TYPE_MATRIX_REAL) {
        gsl_matrix* original = mat_elem->matrix_real;
        if (!original) {
            fprintf(stderr, "Null real matrix.\n");
            return 1;
        }

        size_t original_elements = original->size1 * original->size2;
        if ((size_t)(new_rows * new_cols) != original_elements) {
            fprintf(stderr, "Reshape error: size mismatch (%zu != %d).\n",
                    original_elements, new_rows * new_cols);
            return 1;
        }

        gsl_matrix* reshaped = gsl_matrix_alloc(new_rows, new_cols);
        if (!reshaped) {
            fprintf(stderr, "Allocation failed for reshaped matrix.\n");
            return 1;
        }

        // Copy data in row-major order
        size_t k = 0;
        for (size_t i = 0; i < original->size1; ++i) {
            for (size_t j = 0; j < original->size2; ++j) {
                size_t ri = k / new_cols;
                size_t rj = k % new_cols;
                gsl_matrix_set(reshaped, ri, rj, gsl_matrix_get(original, i, j));
                k++;
            }
        }

        // Replace original matrix
        gsl_matrix_free(original);
        mat_elem->matrix_real = reshaped;

    } else if (mat_elem->type == TYPE_MATRIX_COMPLEX) {
        gsl_matrix_complex* original = mat_elem->matrix_complex;
        if (!original) {
            fprintf(stderr, "Null complex matrix.\n");
            return 1;
        }

        size_t original_elements = original->size1 * original->size2;
        if ((size_t)(new_rows * new_cols) != original_elements) {
            fprintf(stderr, "Reshape error: size mismatch (%zu != %d).\n",
                    original_elements, new_rows * new_cols);
            return 1;
        }

        gsl_matrix_complex* reshaped = gsl_matrix_complex_alloc(new_rows, new_cols);
        if (!reshaped) {
            fprintf(stderr, "Allocation failed for reshaped complex matrix.\n");
            return 1;
        }

        size_t k = 0;
        for (size_t i = 0; i < original->size1; ++i) {
            for (size_t j = 0; j < original->size2; ++j) {
                size_t ri = k / new_cols;
                size_t rj = k % new_cols;
                gsl_matrix_complex_set(reshaped, ri, rj, gsl_matrix_complex_get(original, i, j));
                k++;
            }
        }

        gsl_matrix_complex_free(original);
        mat_elem->matrix_complex = reshaped;

    } else {
        fprintf(stderr, "Type error: expected a real or complex matrix below dimensions.\n");
        stack->top += 2; // Restore top
        return 1;
    }
    return 0;
    // Top of stack now holds reshaped matrix; two items (dims) are popped
}

int make_diag_matrix(Stack *stack) {
    if (stack->top < 0) {
        fprintf(stderr, "Error: stack underflow.\n");
        return -1;
    }

    stack_element *top = &stack->items[stack->top];

    if (top->type == TYPE_MATRIX_REAL) {
        gsl_matrix *vec = top->matrix_real;

        // Check if row or column vector
        size_t len = 0;
        if (vec->size1 == 1) {
            len = vec->size2;
        } else if (vec->size2 == 1) {
            len = vec->size1;
        } else {
            fprintf(stderr, "Error: not a row or column vector.\n");
            return -1;
        }

        // Remove original vector from stack
        stack->top--;

        gsl_matrix *diag = gsl_matrix_calloc(len, len);
        for (size_t i = 0; i < len; ++i) {
            double val = (vec->size1 == 1)
                         ? gsl_matrix_get(vec, 0, i)
                         : gsl_matrix_get(vec, i, 0);
            gsl_matrix_set(diag, i, i, val);
        }

        push_matrix_real(stack, diag);
        gsl_matrix_free(vec);
    }
    else if (top->type == TYPE_MATRIX_COMPLEX) {
        gsl_matrix_complex *vec = top->matrix_complex;

        size_t len = 0;
        if (vec->size1 == 1) {
            len = vec->size2;
        } else if (vec->size2 == 1) {
            len = vec->size1;
        } else {
            fprintf(stderr, "Error: not a row or column vector.\n");
            return -1;
        }

        stack->top--;

        gsl_matrix_complex *diag = gsl_matrix_complex_calloc(len, len);
        for (size_t i = 0; i < len; ++i) {
            gsl_complex val = (vec->size1 == 1)
                              ? gsl_matrix_complex_get(vec, 0, i)
                              : gsl_matrix_complex_get(vec, i, 0);
            gsl_matrix_complex_set(diag, i, i, val);
        }

        push_matrix_complex(stack, diag);
        gsl_matrix_complex_free(vec);
    }
    else {
        fprintf(stderr, "Error: top of stack is not a matrix.\n");
        return -1;
    }

    return 0;
}

int stack_join_matrix_vertical(Stack* stack) {
    if (stack->top < 1) {
        fprintf(stderr, "Need at least two matrices on the stack.\n");
        return 1;
    }

    stack_element* top = &stack->items[stack->top];
    stack_element* second = &stack->items[stack->top - 1];

    // Determine types
    bool top_complex = (top->type == TYPE_MATRIX_COMPLEX);
    bool second_complex = (second->type == TYPE_MATRIX_COMPLEX);
    bool mixed = top_complex || second_complex;

    size_t cols1;
    size_t rows1, rows2;

    if (mixed) {
        // Promote both to complex
        gsl_matrix_complex* mc1 = gsl_matrix_complex_alloc(
            (second->type == TYPE_MATRIX_REAL) ? second->matrix_real->size1 : second->matrix_complex->size1,
            (second->type == TYPE_MATRIX_REAL) ? second->matrix_real->size2 : second->matrix_complex->size2);
        gsl_matrix_complex* mc2 = gsl_matrix_complex_alloc(
            (top->type == TYPE_MATRIX_REAL) ? top->matrix_real->size1 : top->matrix_complex->size1,
            (top->type == TYPE_MATRIX_REAL) ? top->matrix_real->size2 : top->matrix_complex->size2);

        // Fill mc1
        if (second->type == TYPE_MATRIX_REAL) {
            for (size_t i = 0; i < mc1->size1; i++)
                for (size_t j = 0; j < mc1->size2; j++)
                    gsl_matrix_complex_set(mc1, i, j,
                        gsl_complex_rect(gsl_matrix_get(second->matrix_real, i, j), 0.0));
        } else {
            gsl_matrix_complex_memcpy(mc1, second->matrix_complex);
        }

        // Fill mc2
        if (top->type == TYPE_MATRIX_REAL) {
            for (size_t i = 0; i < mc2->size1; i++)
                for (size_t j = 0; j < mc2->size2; j++)
                    gsl_matrix_complex_set(mc2, i, j,
                        gsl_complex_rect(gsl_matrix_get(top->matrix_real, i, j), 0.0));
        } else {
            gsl_matrix_complex_memcpy(mc2, top->matrix_complex);
        }

        // Check column compatibility
        if (mc1->size2 != mc2->size2) {
            fprintf(stderr, "Column sizes must match to join matrices.\n");
            gsl_matrix_complex_free(mc1);
            gsl_matrix_complex_free(mc2);
            return 1;
        }

        // Allocate joined matrix
        gsl_matrix_complex* joined = gsl_matrix_complex_alloc(mc1->size1 + mc2->size1, mc1->size2);
        gsl_matrix_complex_view top_block =
	  gsl_matrix_complex_submatrix(joined, 0, 0, mc1->size1, mc1->size2);
        gsl_matrix_complex_view bot_block =
	  gsl_matrix_complex_submatrix(joined, mc1->size1, 0, mc2->size1, mc2->size2);

        gsl_matrix_complex_memcpy(&top_block.matrix, mc1);
        gsl_matrix_complex_memcpy(&bot_block.matrix, mc2);

        // Clean up
        gsl_matrix_complex_free(mc1);
        gsl_matrix_complex_free(mc2);

        // Pop both, push result
        pop(stack);
        pop(stack);
        push_matrix_complex(stack, joined);
    } else {
        // Both real
        if (second->matrix_real->size2 != top->matrix_real->size2) {
            fprintf(stderr, "Column sizes must match to join matrices.\n");
            return 1;
        }

        rows1 = second->matrix_real->size1;
        rows2 = top->matrix_real->size1;
        cols1 = second->matrix_real->size2;

        gsl_matrix* joined = gsl_matrix_alloc(rows1 + rows2, cols1);
        gsl_matrix_view top_block = gsl_matrix_submatrix(joined, 0, 0, rows1, cols1);
        gsl_matrix_view bot_block = gsl_matrix_submatrix(joined, rows1, 0, rows2, cols1);

        gsl_matrix_memcpy(&top_block.matrix, second->matrix_real);
        gsl_matrix_memcpy(&bot_block.matrix, top->matrix_real);

        // Pop both, push result
        pop(stack);
        pop(stack);
        push_matrix_real(stack, joined);
    }
    return 0;
}

int stack_join_matrix_horizontal(Stack* stack) {
    if (stack->top < 1) {
        fprintf(stderr, "Need at least two matrices on the stack.\n");
        return 1;
    }

    stack_element* top = &stack->items[stack->top];
    stack_element* second = &stack->items[stack->top - 1];

    bool top_complex = (top->type == TYPE_MATRIX_COMPLEX);
    bool second_complex = (second->type == TYPE_MATRIX_COMPLEX);
    bool mixed = top_complex || second_complex;

    if (mixed) {
        gsl_matrix_complex* mc1 = gsl_matrix_complex_alloc(
            (second->type == TYPE_MATRIX_REAL) ? second->matrix_real->size1 : second->matrix_complex->size1,
            (second->type == TYPE_MATRIX_REAL) ? second->matrix_real->size2 : second->matrix_complex->size2);
        gsl_matrix_complex* mc2 = gsl_matrix_complex_alloc(
            (top->type == TYPE_MATRIX_REAL) ? top->matrix_real->size1 : top->matrix_complex->size1,
            (top->type == TYPE_MATRIX_REAL) ? top->matrix_real->size2 : top->matrix_complex->size2);

        // Promote real to complex if necessary
        if (second->type == TYPE_MATRIX_REAL) {
            for (size_t i = 0; i < mc1->size1; i++)
                for (size_t j = 0; j < mc1->size2; j++)
                    gsl_matrix_complex_set(mc1, i, j,
                        gsl_complex_rect(gsl_matrix_get(second->matrix_real, i, j), 0.0));
        } else {
            gsl_matrix_complex_memcpy(mc1, second->matrix_complex);
        }

        if (top->type == TYPE_MATRIX_REAL) {
            for (size_t i = 0; i < mc2->size1; i++)
                for (size_t j = 0; j < mc2->size2; j++)
                    gsl_matrix_complex_set(mc2, i, j,
                        gsl_complex_rect(gsl_matrix_get(top->matrix_real, i, j), 0.0));
        } else {
            gsl_matrix_complex_memcpy(mc2, top->matrix_complex);
        }

        // Check row compatibility
        if (mc1->size1 != mc2->size1) {
            fprintf(stderr, "Row sizes must match to join matrices horizontally.\n");
            gsl_matrix_complex_free(mc1);
            gsl_matrix_complex_free(mc2);
            return 1;
        }

        gsl_matrix_complex* joined = gsl_matrix_complex_alloc(mc1->size1, mc1->size2 + mc2->size2);
        gsl_matrix_complex_view left =
	  gsl_matrix_complex_submatrix(joined, 0, 0, mc1->size1, mc1->size2);
        gsl_matrix_complex_view right =
	  gsl_matrix_complex_submatrix(joined, 0, mc1->size2, mc2->size1, mc2->size2);

        gsl_matrix_complex_memcpy(&left.matrix, mc1);
        gsl_matrix_complex_memcpy(&right.matrix, mc2);

        gsl_matrix_complex_free(mc1);
        gsl_matrix_complex_free(mc2);

        pop(stack);
        pop(stack);
        push_matrix_complex(stack, joined);
    } else {
        // Both real
        if (second->matrix_real->size1 != top->matrix_real->size1) {
            fprintf(stderr, "Row sizes must match to join matrices horizontally.\n");
            return 1;
        }

        size_t rows = second->matrix_real->size1;
        size_t cols1 = second->matrix_real->size2;
        size_t cols2 = top->matrix_real->size2;

        gsl_matrix* joined = gsl_matrix_alloc(rows, cols1 + cols2);
        gsl_matrix_view left = gsl_matrix_submatrix(joined, 0, 0, rows, cols1);
        gsl_matrix_view right = gsl_matrix_submatrix(joined, 0, cols1, rows, cols2);

        gsl_matrix_memcpy(&left.matrix, second->matrix_real);
        gsl_matrix_memcpy(&right.matrix, top->matrix_real);

        pop(stack);
        pop(stack);
        push_matrix_real(stack, joined);
    }
    return 0;
}

int matrix_cumsum_rows(Stack* stack) {
    if (stack->top < 0) {
        fprintf(stderr, "Stack underflow: expected a matrix.\n");
        return 1;
    }

    stack_element* top = &stack->items[stack->top];

    if (top->type == TYPE_MATRIX_REAL) {
        gsl_matrix* m = top->matrix_real;
        gsl_matrix* result = gsl_matrix_alloc(m->size1, m->size2);

        for (size_t i = 0; i < m->size1; i++) {
            double sum = 0.0;
            for (size_t j = 0; j < m->size2; j++) {
                sum += gsl_matrix_get(m, i, j);
                gsl_matrix_set(result, i, j, sum);
            }
        }

        pop(stack);
        push_matrix_real(stack, result);
    }
    else if (top->type == TYPE_MATRIX_COMPLEX) {
        gsl_matrix_complex* m = top->matrix_complex;
        gsl_matrix_complex* result = gsl_matrix_complex_alloc(m->size1, m->size2);

        for (size_t i = 0; i < m->size1; i++) {
            gsl_complex sum = gsl_complex_rect(0.0, 0.0);
            for (size_t j = 0; j < m->size2; j++) {
                gsl_complex z = gsl_matrix_complex_get(m, i, j);
                sum = gsl_complex_add(sum, z);
                gsl_matrix_complex_set(result, i, j, sum);
            }
        }

        pop(stack);
        push_matrix_complex(stack, result);
    }
    else {
        fprintf(stderr, "Top of stack is not a real or complex matrix.\n");
	return 1;
    }
    return 0;
}

int matrix_cumsum_cols(Stack* stack) {
    if (stack->top < 0) {
        fprintf(stderr, "Stack underflow: expected a matrix.\n");
        return 1;
    }

    stack_element* top = &stack->items[stack->top];

    if (top->type == TYPE_MATRIX_REAL) {
        gsl_matrix* m = top->matrix_real;
        gsl_matrix* result = gsl_matrix_alloc(m->size1, m->size2);

        for (size_t j = 0; j < m->size2; j++) {
            double sum = 0.0;
            for (size_t i = 0; i < m->size1; i++) {
                sum += gsl_matrix_get(m, i, j);
                gsl_matrix_set(result, i, j, sum);
            }
        }

        pop(stack);
        push_matrix_real(stack, result);
    }
    else if (top->type == TYPE_MATRIX_COMPLEX) {
        gsl_matrix_complex* m = top->matrix_complex;
        gsl_matrix_complex* result = gsl_matrix_complex_alloc(m->size1, m->size2);

        for (size_t j = 0; j < m->size2; j++) {
            gsl_complex sum = gsl_complex_rect(0.0, 0.0);
            for (size_t i = 0; i < m->size1; i++) {
                gsl_complex z = gsl_matrix_complex_get(m, i, j);
                sum = gsl_complex_add(sum, z);
                gsl_matrix_complex_set(result, i, j, sum);
            }
        }

        pop(stack);
        push_matrix_complex(stack, result);
    }
    else {
        fprintf(stderr, "Top of stack is not a real or complex matrix.\n");
	return 1;
    }
    return 0;
}
