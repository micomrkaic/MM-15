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

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <complex.h>
#include <ctype.h>
#include <stdbool.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_vector.h>
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
#include "math_helpers.h"
#include "linear_algebra.h"

int matrix_inverse(Stack* stack) {
  if (stack->top < 0) {
    fprintf(stderr,"No matrix to invert\n");
    return 1;
  }

  stack_element m = pop(stack);

  if (m.type == TYPE_MATRIX_REAL) {
    size_t n = m.matrix_real->size1;
    if (n != m.matrix_real->size2) {
      fprintf(stderr,"Matrix is not square\n");
      return 1;
    }

    gsl_matrix* inv = gsl_matrix_alloc(n, n);
    gsl_matrix* tmp = gsl_matrix_alloc(n, n);
    gsl_matrix_memcpy(tmp, m.matrix_real);
    gsl_permutation* p = gsl_permutation_alloc(n);
    int signum;

    // LU decomposition
    int status = gsl_linalg_LU_decomp(tmp, p, &signum);
    if (status != GSL_SUCCESS) {
      fprintf(stderr, "LU decomposition failed\n");
      gsl_matrix_free(inv);
      gsl_matrix_free(tmp);
      gsl_permutation_free(p);
      gsl_matrix_free(m.matrix_real);
      return 1;
    }

    // Check determinant
    double det = gsl_linalg_LU_det(tmp, signum);
    if ((det == 0.0) || (isnan(det))) {
      fprintf(stderr,"Matrix is singular, cannot invert\n");
      gsl_matrix_free(inv);
      gsl_matrix_free(tmp);
      gsl_permutation_free(p);
      gsl_matrix_free(m.matrix_real); // free popped matrix
      return 1;
    }

    // Now safe to invert
    if (gsl_linalg_LU_invert(tmp, p, inv) != 0) {
      fprintf(stderr,"Matrix inversion failed\n");
      gsl_matrix_free(inv);
      gsl_matrix_free(tmp);
      gsl_permutation_free(p);
      gsl_matrix_free(m.matrix_real);
    }

    gsl_matrix_free(tmp);
    gsl_permutation_free(p);
    gsl_matrix_free(m.matrix_real);
    push_matrix_real(stack, inv);
    return 0;

  } else if (m.type == TYPE_MATRIX_COMPLEX) {
    size_t n = m.matrix_complex->size1;
    if (n != m.matrix_complex->size2) {
      fprintf(stderr,"Complex matrix is not square\n");
      return 1;
    }

    gsl_matrix_complex* inv = gsl_matrix_complex_alloc(n, n);
    gsl_matrix_complex* tmp = gsl_matrix_complex_alloc(n, n);
    gsl_matrix_complex_memcpy(tmp, m.matrix_complex);
    gsl_permutation* p = gsl_permutation_alloc(n);
    int signum;

    // LU decomposition
    gsl_linalg_complex_LU_decomp(tmp, p, &signum);

    // Check complex determinant
    gsl_complex det = gsl_linalg_complex_LU_det(tmp, signum);
    if (GSL_REAL(det) == 0.0 && GSL_IMAG(det) == 0.0) {
      fprintf(stderr,"Complex matrix is singular, cannot invert\n");
      gsl_matrix_complex_free(inv);
      gsl_matrix_complex_free(tmp);
      gsl_permutation_free(p);
      gsl_matrix_complex_free(m.matrix_complex);
      return 1;
    }

    // Safe to invert
    if (gsl_linalg_complex_LU_invert(tmp, p, inv) != 0) {
      fprintf(stderr,"Complex matrix inversion failed\n");
      gsl_matrix_complex_free(inv);
      gsl_matrix_complex_free(tmp);
      gsl_permutation_free(p);
      gsl_matrix_complex_free(m.matrix_complex);
    }

    gsl_matrix_complex_free(tmp);
    gsl_permutation_free(p);
    gsl_matrix_complex_free(m.matrix_complex);
    push_matrix_complex(stack, inv);
    return 0;

  } else {
    fprintf(stderr,"Only real or complex matrix inversion is supported\n");
    return 1;
  }
}

int matrix_determinant(Stack* stack) {

  if (stack->top < 0) {
    fprintf(stderr,"No matrix to compute determinant\n");
    return 1;
  }

  stack_element m = pop(stack);
  if (m.type == TYPE_MATRIX_REAL) {
    size_t n = m.matrix_real->size1;
    if (n != m.matrix_real->size2) {
      fprintf(stderr,"Matrix is not square\n");
      return 1;
    }

    gsl_matrix* tmp = gsl_matrix_alloc(n, n);
    gsl_matrix_memcpy(tmp, m.matrix_real);
    gsl_permutation* p = gsl_permutation_alloc(n);
    int signum;

    if (gsl_linalg_LU_decomp(tmp, p, &signum) != 0) {
      fprintf(stderr,"Matrix decomposition failed\n");
      gsl_matrix_free(tmp);
      gsl_permutation_free(p);
      return 1;
    }

    double det = gsl_linalg_LU_det(tmp, signum);
    gsl_matrix_free(tmp);
    gsl_permutation_free(p);
    push_real(stack, det);

  } else if (m.type == TYPE_MATRIX_COMPLEX) {
    size_t n = m.matrix_complex->size1;
    if (n != m.matrix_complex->size2) {
      fprintf(stderr,"Complex matrix is not square\n");
      return 1;
    }

    gsl_matrix_complex* tmp = gsl_matrix_complex_alloc(n, n);
    gsl_matrix_complex_memcpy(tmp, m.matrix_complex);
    gsl_permutation* p = gsl_permutation_alloc(n);
    int signum;

    if (gsl_linalg_complex_LU_decomp(tmp, p, &signum) != 0) {
      fprintf(stderr,"Complex matrix decomposition failed\n");
      gsl_matrix_complex_free(tmp);
      gsl_permutation_free(p);
      return 1;
    }
    gsl_complex det = gsl_linalg_complex_LU_det(tmp, signum);
    gsl_matrix_complex_free(tmp);
    gsl_permutation_free(p);
    push_complex(stack, det);
    return 0;
  } else {
    fprintf(stderr,"Only real or complex matrix determinant is supported\n");
    return 1;
  }
  return 0;
}

int solve_linear_system(Stack* stack) {
  if (stack->top < 1) {
    fprintf(stderr,"Need coefficient matrix and RHS vector\n");
    return 1;
  }

  stack_element b = pop(stack); // RHS vector (as matrix)
  stack_element a = pop(stack); // Coefficient matrix

  if (a.type == TYPE_MATRIX_REAL && b.type == TYPE_MATRIX_REAL) {
    if (a.matrix_real->size1 != a.matrix_real->size2 ||
	b.matrix_real->size2 != 1 || a.matrix_real->size1 != b.matrix_real->size1) {
      fprintf(stderr,"Dimension mismatch or matrix not square\n");
      return 1;
    }

    size_t n = a.matrix_real->size1;
    gsl_matrix* A = gsl_matrix_alloc(n, n);
    gsl_matrix_memcpy(A, a.matrix_real);
    gsl_vector* B = gsl_vector_alloc(n);
    for (size_t i = 0; i < n; ++i)
      gsl_vector_set(B, i, gsl_matrix_get(b.matrix_real, i, 0));

    gsl_vector* X = gsl_vector_alloc(n);
    gsl_permutation* p = gsl_permutation_alloc(n);
    int signum;

    gsl_linalg_LU_decomp(A, p, &signum);
    gsl_linalg_LU_solve(A, p, B, X);

    gsl_matrix* result = gsl_matrix_alloc(n, 1);
    for (size_t i = 0; i < n; ++i)
      gsl_matrix_set(result, i, 0, gsl_vector_get(X, i));

    push_matrix_real(stack, result);
    gsl_matrix_free(A);
    gsl_vector_free(B);
    gsl_vector_free(X);
    gsl_permutation_free(p);
    return 0;
  } else {
    fprintf(stderr,"Unsupported types for linear system solving\n");
    return 1;
  }
  return 0;
}

int matrix_eigen_decompose(Stack* stack) {
  if (stack->top < 0) {
    fprintf(stderr,"No matrix on stack for eigendecomposition\n");
    return 1;
  }

  stack_element m = pop(stack);
  if (m.type == TYPE_MATRIX_REAL) {
    size_t n = m.matrix_real->size1;
    if (n != m.matrix_real->size2) {
      fprintf(stderr,"Matrix is not square\n");
      return 1;
    }

    // Allocate workspace and result containers
    gsl_matrix* tmp = gsl_matrix_alloc(n, n);
    gsl_matrix_memcpy(tmp, m.matrix_real);
    gsl_vector_complex* eval = gsl_vector_complex_alloc(n);
    gsl_matrix_complex* evec = gsl_matrix_complex_alloc(n, n);
    gsl_eigen_nonsymmv_workspace* w = gsl_eigen_nonsymmv_alloc(n);

    if (gsl_eigen_nonsymmv(tmp, eval, evec, w) != 0) {
      fprintf(stderr,"Eigen decomposition failed\n");
      gsl_matrix_free(tmp);
      gsl_vector_complex_free(eval);
      gsl_matrix_complex_free(evec);
      gsl_eigen_nonsymmv_free(w);
      return 1;
    }

    gsl_eigen_nonsymmv_free(w);
    gsl_matrix_free(tmp);

    // Push results to the stack
    push_matrix_complex(stack, evec);

    // Convert eigenvalues (vector) to a diagonal matrix
    gsl_matrix_complex* eval_matrix = gsl_matrix_complex_calloc(n, n);
    for (size_t i = 0; i < n; ++i) {
      gsl_complex z = gsl_vector_complex_get(eval, i);
      gsl_matrix_complex_set(eval_matrix, i, i, z);
    }

    gsl_vector_complex_free(eval);
    push_matrix_complex(stack, eval_matrix);
    return 0;
  } else {
    fprintf(stderr,"Only real matrix eigendecomposition is supported\n");
    return 1;
  }
}

int matrix_transpose(Stack* stack) {
  if (stack->top < 0) {
    fprintf(stderr,"No matrix to transpose\n");
    return 1;
  }

  stack_element m = pop(stack);

  if (m.type == TYPE_MATRIX_REAL) {
    size_t rows = m.matrix_real->size1;
    size_t cols = m.matrix_real->size2;

    gsl_matrix* transposed = gsl_matrix_alloc(cols, rows);
    if (!transposed) {
      fprintf(stderr,"Memory allocation failed for transposed matrix\n");
      return 1;
    }

    for (size_t i = 0; i < rows; ++i) {
      for (size_t j = 0; j < cols; ++j) {
	double val = gsl_matrix_get(m.matrix_real, i, j);
	gsl_matrix_set(transposed, j, i, val);
      }
    }

    push_matrix_real(stack, transposed);
    gsl_matrix_free(m.matrix_real);
  }
  else if (m.type == TYPE_MATRIX_COMPLEX) {
    size_t rows = m.matrix_complex->size1;
    size_t cols = m.matrix_complex->size2;

    gsl_matrix_complex* transposed = gsl_matrix_complex_alloc(cols, rows);
    if (!transposed) {
      fprintf(stderr,"Memory allocation failed for transposed complex matrix\n");
      return 1;
    }

    for (size_t i = 0; i < rows; ++i) {
      for (size_t j = 0; j < cols; ++j) {
	gsl_complex val = gsl_matrix_complex_get(m.matrix_complex, i, j);
	gsl_matrix_complex_set(transposed, j, i, val);
      }
    }

    push_matrix_complex(stack, transposed);
    gsl_matrix_complex_free(m.matrix_complex);
    return 0;
  }
  else {
    fprintf(stderr,"Only real or complex matrices can be transposed\n");
    return 1;
  }
  return 0;
}

int matrix_cholesky(Stack* stack) {
  if (stack->top < 0) {
    fprintf(stderr,"No matrix on stack for Cholesky decomposition\n");
    return 1;
  }

  stack_element m = pop(stack);

  if (m.type == TYPE_MATRIX_COMPLEX) {
    fprintf(stderr,"Cholesky decomposition is only supported for real matrices\n");
    return 1;
  }

  if (m.type != TYPE_MATRIX_REAL) {
    fprintf(stderr,"Only real matrices are supported for Cholesky decomposition\n");
    return 1;
  }

  size_t n = m.matrix_real->size1;
  if (n != m.matrix_real->size2) {
    fprintf(stderr,"Matrix must be square for Cholesky decomposition\n");
    return 1;
  }

  // Check if the matrix is symmetric
  for (size_t i = 0; i < n; ++i) {
    for (size_t j = i + 1; j < n; ++j) { // Only need to check upper triangle
      double aij = gsl_matrix_get(m.matrix_real, i, j);
      double aji = gsl_matrix_get(m.matrix_real, j, i);
      if (fabs(aij - aji) > 1e-9) { // Tolerance for floating point errors
	fprintf(stderr,"Matrix is not symmetric; cannot perform Cholesky decomposition\n");
	return 1;
      }
    }
  }

  gsl_matrix* tmp = gsl_matrix_alloc(n, n);
  gsl_matrix_memcpy(tmp, m.matrix_real);

  int status = gsl_linalg_cholesky_decomp(tmp);
  if (status != 0) {
    fprintf(stderr,"Cholesky decomposition failed (matrix may not be positive definite)\n");
    gsl_matrix_free(tmp);
    return 1;
  }

  // Zero out the upper triangle to make it clean
  for (size_t i = 0; i < n; ++i) {
    for (size_t j = i + 1; j < n; ++j) {
      gsl_matrix_set(tmp, i, j, 0.0);
    }
  }

  gsl_matrix_free(m.matrix_real); // Free original
  push_matrix_real(stack, tmp);
  return 0;
}

int matrix_svd(Stack* stack) {
  if (stack->top < 0) {
    fprintf(stderr,"No matrix on stack for SVD\n");
    return 1;
  }

  stack_element m = pop(stack);

  if (m.type != TYPE_MATRIX_REAL) {
    fprintf(stderr,"SVD is only implemented for real matrices\n");
    return 1;
  }

  size_t m_rows = m.matrix_real->size1;
  size_t m_cols = m.matrix_real->size2;

  gsl_matrix* A = gsl_matrix_alloc(m_rows, m_cols);
  gsl_matrix_memcpy(A, m.matrix_real);

  size_t min_dim = (m_rows < m_cols) ? m_rows : m_cols;

  gsl_vector* S = gsl_vector_alloc(min_dim);            // Singular values
  gsl_matrix* V = gsl_matrix_alloc(m_cols, m_cols);     // Right singular vectors
  gsl_vector* work = gsl_vector_alloc(min_dim);         // Workspace

  int status = gsl_linalg_SV_decomp(A, V, S, work);

  if (status != 0) {
    fprintf(stderr,"SVD decomposition failed\n");
    gsl_matrix_free(A);
    gsl_matrix_free(V);
    gsl_vector_free(S);
    gsl_vector_free(work);
    return 1;
  }

  // Extract U from overwritten A
  gsl_matrix* U = gsl_matrix_alloc(m_rows, min_dim);
  for (size_t i = 0; i < m_rows; ++i) {
    for (size_t j = 0; j < min_dim; ++j) {
      gsl_matrix_set(U, i, j, gsl_matrix_get(A, i, j));
    }
  }

  // Create diagonal matrix for S
  gsl_matrix* S_mat = gsl_matrix_calloc(m_rows, m_cols);
  for (size_t i = 0; i < min_dim; ++i) {
    gsl_matrix_set(S_mat, i, i, gsl_vector_get(S, i));
  }

  // Push U, S, V in that order
  push_matrix_real(stack, U);
  push_matrix_real(stack, S_mat);
  push_matrix_real(stack, V);

  // Clean up
  gsl_vector_free(S);
  gsl_vector_free(work);
  gsl_matrix_free(A);
  gsl_matrix_free(m.matrix_real);
  
  return 0;
}


// Computes the Frobenius norm of a GSL matrix
double gls_matrix_frobenius_norm(const gsl_matrix* A) {
  double sum = 0.0;
  for (size_t i = 0; i < A->size1; ++i) {
    for (size_t j = 0; j < A->size2; ++j) {
      double val = gsl_matrix_get(A, i, j);
      sum += val * val;
    }
  }
  return sqrt(sum);
}

int matrix_frobenius_norm(Stack *stack) {
  if (stack->top < 0) {
    fprintf(stderr,"No matrix to invert\n");
    return 1;
  }

  // To do a wrapper for real and complex matrices
  return 0;
}

#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

int matrix_pseudoinverse(Stack *stack) {
  if (stack->top < 0) {
    fprintf(stderr,"No matrix to pseudoinvert\n");
    return 1;
  }

  stack_element m = pop(stack);
  size_t rows = m.matrix_real->size1;
  size_t cols = m.matrix_real->size2;
  double tol = DBL_EPSILON * MAX(rows, cols);
    
  if ((m.type != TYPE_MATRIX_REAL) || (rows != cols))
    {
      fprintf(stderr,"Only real square matrix pseudoinversion is supported\n");
      return 1;
    }

  gsl_matrix *U = gsl_matrix_alloc(rows, cols);
  gsl_matrix_memcpy(U, m.matrix_real);

  gsl_matrix *V = gsl_matrix_alloc(cols, cols);
  gsl_vector *S = gsl_vector_alloc(cols);
  gsl_vector *work = gsl_vector_alloc(cols);

  if (gsl_linalg_SV_decomp(U, V, S, work) != 0) {
    fprintf(stderr,"SVD decomposition failed\n");
    gsl_matrix_free(U); gsl_matrix_free(V);
    gsl_vector_free(S); gsl_vector_free(work);
    gsl_matrix_free(m.matrix_real);
    return 1;
  }

  gsl_matrix *S_pinv = gsl_matrix_calloc(cols, rows);
  for (size_t i = 0; i < cols; ++i) {
    double s = gsl_vector_get(S, i);
    if (s > tol) {
      gsl_matrix_set(S_pinv, i, i, 1.0 / s);
    }
  }

  gsl_matrix *VS_pinv = gsl_matrix_alloc(cols, rows);
  gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, V, S_pinv, 0.0, VS_pinv);

  gsl_matrix_transpose(U);
  gsl_matrix *A_pinv = gsl_matrix_alloc(cols, rows);
  gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, VS_pinv, U, 0.0, A_pinv);

  gsl_matrix_free(U);
  gsl_matrix_free(V);
  gsl_vector_free(S);
  gsl_vector_free(work);
  gsl_matrix_free(S_pinv);
  gsl_matrix_free(VS_pinv);
  gsl_matrix_free(m.matrix_real);  // free popped matrix

  push_matrix_real(stack, A_pinv);
  return 0;
}

