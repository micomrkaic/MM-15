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
#include <gsl/gsl_randist.h>
#include <gsl/gsl_cdf.h>
#include "stack.h"
#include "math_parsers.h"
#include "binary_fun.h"
#include "unary_fun.h"


#include "stat_fun.h"

// Wrapper for standard normal PDF: mean = 0, sigma = 1
double standard_normal_pdf(double x) {
  return gsl_ran_gaussian_pdf(x, 1.0);  // sigma = 1
}

// Wrapper for standard normal cumulative distribution function
double standard_normal_cdf(double x) {
  return gsl_cdf_ugaussian_P(x);  // P = lower tail CDF
}

// Wrapper for standard normal quantile function (inverse CDF)
double standard_normal_quantile(double p) {
  return gsl_cdf_ugaussian_Pinv(p);  // P-inverse = quantile
}

void matrix_column_means(Stack* stack) {
  if (stack->top < 0) {
    fprintf(stderr, "Stack underflow: need a matrix to compute column means.\n");
    return;
  }

  stack_element* top = &stack->items[stack->top];

  if (top->type == TYPE_MATRIX_REAL) {
    gsl_matrix* mat = top->matrix_real;
    if (!mat) {
      fprintf(stderr, "Null real matrix.\n");
      return;
    }

    size_t rows = mat->size1;
    size_t cols = mat->size2;

    gsl_matrix* mean = gsl_matrix_calloc(1, cols);
    if (!mean) {
      fprintf(stderr, "Failed to allocate result matrix.\n");
      return;
    }

    for (size_t j = 0; j < cols; ++j) {
      double sum = 0.0;
      for (size_t i = 0; i < rows; ++i) {
	sum += gsl_matrix_get(mat, i, j);
      }
      gsl_matrix_set(mean, 0, j, sum / rows);
    }

    stack_element result;
    result.type = TYPE_MATRIX_REAL;
    result.matrix_real = mean;
    if (stack->top + 1 >= STACK_SIZE) {
      fprintf(stderr, "Stack overflow.\n");
      gsl_matrix_free(mean);
      return;
    }
    stack->items[++stack->top] = result;

  } else if (top->type == TYPE_MATRIX_COMPLEX) {
    gsl_matrix_complex* mat = top->matrix_complex;
    if (!mat) {
      fprintf(stderr, "Null complex matrix.\n");
      return;
    }

    size_t rows = mat->size1;
    size_t cols = mat->size2;

    gsl_matrix_complex* mean = gsl_matrix_complex_calloc(1, cols);
    if (!mean) {
      fprintf(stderr, "Failed to allocate complex result matrix.\n");
      return;
    }

    for (size_t j = 0; j < cols; ++j) {
      gsl_complex sum = gsl_complex_rect(0.0, 0.0);
      for (size_t i = 0; i < rows; ++i) {
	gsl_complex val = gsl_matrix_complex_get(mat, i, j);
	sum = gsl_complex_add(sum, val);
      }
      gsl_complex mean_val = gsl_complex_div_real(sum, (double)rows);
      gsl_matrix_complex_set(mean, 0, j, mean_val);
    }

    stack_element result;
    result.type = TYPE_MATRIX_COMPLEX;
    result.matrix_complex = mean;
    if (stack->top + 1 >= STACK_SIZE) {
      fprintf(stderr, "Stack overflow.\n");
      gsl_matrix_complex_free(mean);
      return;
    }
    stack->items[++stack->top] = result;

  } else {
    fprintf(stderr, "Type error: top stack item must be a matrix (real or complex).\n");
  }
}

/* void matrix_reduce(Stack* stack, const char* axis, const char* op) { */
/*   if (stack->top < 0) { */
/*     fprintf(stderr, "Stack underflow: need a matrix to compute reduction.\n"); */
/*     return; */
/*   } */

/*   if ((!axis) || (!(strcmp(axis, "row") == 0) && (!(strcmp(axis, "col") == 0)))) { */
/*     fprintf(stderr, "Invalid axis: must be \"row\" or \"col\".\n"); */
/*     return; */
/*   } */
/*   if (!op || (strcmp(op, "sum") != 0 && strcmp(op, "mean") != 0 && strcmp(op, "var") != 0)) { */
/*     fprintf(stderr, "Invalid operation: must be \"sum\", \"mean\", or \"var\".\n"); */
/*     return; */
/*   } */

/*   stack_element* top = &stack->items[stack->top]; */

/*   bool compute_rows = strcmp(axis, "row") == 0; */
/*   bool compute_cols = strcmp(axis, "col") == 0; */
/*   bool do_sum = strcmp(op, "sum") == 0; */
/*   bool do_mean = strcmp(op, "mean") == 0; */
/*   bool do_var = strcmp(op, "var") == 0; */

/*   if (top->type == TYPE_MATRIX_REAL) { */
/*     gsl_matrix* mat = top->matrix_real; */
/*     if (!mat) { */
/*       fprintf(stderr, "Null real matrix.\n"); */
/*       return; */
/*     } */

/*     size_t rows = mat->size1; */
/*     size_t cols = mat->size2; */

/*     gsl_matrix* result = NULL; */

/*     if (compute_rows) { */
/*       result = gsl_matrix_calloc(rows, 1); */
/*       if (!result) { */
/* 	fprintf(stderr, "Failed to allocate result matrix.\n"); */
/* 	return; */
/*       } */
/*       for (size_t i = 0; i < rows; ++i) { */
/* 	double acc = 0.0; */
/* 	double acc_sq = 0.0; */
/* 	for (size_t j = 0; j < cols; ++j) { */
/* 	  double val = gsl_matrix_get(mat, i, j); */
/* 	  acc += val; */
/* 	  acc_sq += val * val; */
/* 	} */
/* 	double res = 0.0; */
/* 	if (do_sum) { */
/* 	  res = acc; */
/* 	} else if (do_mean) { */
/* 	  res = acc / cols; */
/* 	} else if (do_var) { */
/* 	  double mean = acc / cols; */
/* 	  res = (acc_sq - cols * mean * mean) / (cols - 1); */
/* 	} */
/* 	gsl_matrix_set(result, i, 0, res); */
/*       } */
/*     } else if (compute_cols) { */
/*       result = gsl_matrix_calloc(1, cols); */
/*       if (!result) { */
/* 	fprintf(stderr, "Failed to allocate result matrix.\n"); */
/* 	return; */
/*       } */
/*       for (size_t j = 0; j < cols; ++j) { */
/* 	double acc = 0.0; */
/* 	double acc_sq = 0.0; */
/* 	for (size_t i = 0; i < rows; ++i) { */
/* 	  double val = gsl_matrix_get(mat, i, j); */
/* 	  acc += val; */
/* 	  acc_sq += val * val; */
/* 	} */
/* 	double res = 0.0; */
/* 	if (do_sum) { */
/* 	  res = acc; */
/* 	} else if (do_mean) { */
/* 	  res = acc / rows; */
/* 	} else if (do_var) { */
/* 	  double mean = acc / rows; */
/* 	  res = (acc_sq - rows * mean * mean) / (rows - 1); */
/* 	} */
/* 	gsl_matrix_set(result, 0, j, res); */
/*       } */
/*     } */

/*     stack_element out; */
/*     out.type = TYPE_MATRIX_REAL; */
/*     out.matrix_real = result; */
/*     if (stack->top + 1 >= STACK_SIZE) { */
/*       fprintf(stderr, "Stack overflow.\n"); */
/*       gsl_matrix_free(result); */
/*       return; */
/*     } */
/*     stack->items[++stack->top] = out; */

/*   } else if (top->type == TYPE_MATRIX_COMPLEX) { */
/*     gsl_matrix_complex* mat = top->matrix_complex; */
/*     if (!mat) { */
/*       fprintf(stderr, "Null complex matrix.\n"); */
/*       return; */
/*     } */

/*     size_t rows = mat->size1; */
/*     size_t cols = mat->size2; */

/*     gsl_matrix_complex* result = NULL; */

/*     if (compute_rows) { */
/*       result = gsl_matrix_complex_calloc(rows, 1); */
/*       if (!result) { */
/* 	fprintf(stderr, "Failed to allocate complex result matrix.\n"); */
/* 	return; */
/*       } */
/*       for (size_t i = 0; i < rows; ++i) { */
/* 	gsl_complex acc = gsl_complex_rect(0.0, 0.0); */
/* 	gsl_complex acc_sq = gsl_complex_rect(0.0, 0.0); */
/* 	for (size_t j = 0; j < cols; ++j) { */
/* 	  gsl_complex val = gsl_matrix_complex_get(mat, i, j); */
/* 	  acc = gsl_complex_add(acc, val); */
/* 	  gsl_complex val_sq = gsl_complex_mul(val, gsl_complex_conjugate(val)); */
/* 	  acc_sq = gsl_complex_add(acc_sq, val_sq); */
/* 	} */
/* 	gsl_complex res = gsl_complex_rect(0.0, 0.0); */
/* 	if (do_sum) { */
/* 	  res = acc; */
/* 	} else if (do_mean) { */
/* 	  res = gsl_complex_div_real(acc, (double)cols); */
/* 	} else if (do_var) { */
/* 	  double mean_norm = gsl_complex_abs(acc) / cols; */
/* 	  double acc_sq_real = GSL_REAL(acc_sq); */
/* 	  res = gsl_complex_rect((acc_sq_real - cols * mean_norm * mean_norm) / (cols - 1), 0.0); */
/* 	} */
/* 	gsl_matrix_complex_set(result, i, 0, res); */
/*       } */
/*     } else if (compute_cols) { */
/*       result = gsl_matrix_complex_calloc(1, cols); */
/*       if (!result) { */
/* 	fprintf(stderr, "Failed to allocate complex result matrix.\n"); */
/* 	return; */
/*       } */
/*       for (size_t j = 0; j < cols; ++j) { */
/* 	gsl_complex acc = gsl_complex_rect(0.0, 0.0); */
/* 	gsl_complex acc_sq = gsl_complex_rect(0.0, 0.0); */
/* 	for (size_t i = 0; i < rows; ++i) { */
/* 	  gsl_complex val = gsl_matrix_complex_get(mat, i, j); */
/* 	  acc = gsl_complex_add(acc, val); */
/* 	  gsl_complex val_sq = gsl_complex_mul(val, gsl_complex_conjugate(val)); */
/* 	  acc_sq = gsl_complex_add(acc_sq, val_sq); */
/* 	} */
/* 	gsl_complex res = gsl_complex_rect(0.0, 0.0); */
/* 	if (do_sum) { */
/* 	  res = acc; */
/* 	} else if (do_mean) { */
/* 	  res = gsl_complex_div_real(acc, (double)rows); */
/* 	} else if (do_var) { */
/* 	  double mean_norm = gsl_complex_abs(acc) / rows; */
/* 	  double acc_sq_real = GSL_REAL(acc_sq); */
/* 	  res = gsl_complex_rect((acc_sq_real - rows * mean_norm * mean_norm) / (rows - 1), 0.0); */
/* 	} */
/* 	gsl_matrix_complex_set(result, 0, j, res); */
/*       } */
/*     } */

/*     stack_element out; */
/*     out.type = TYPE_MATRIX_COMPLEX; */
/*     out.matrix_complex = result; */
/*     if (stack->top + 1 >= STACK_SIZE) { */
/*       fprintf(stderr, "Stack overflow.\n"); */
/*       gsl_matrix_complex_free(result); */
/*       return; */
/*     } */
/*     stack->items[++stack->top] = out; */

/*   } else { */
/*     fprintf(stderr, "Type error: top stack item must be a matrix (real or complex).\n"); */
/*   } */
/* } */


void matrix_reduce(Stack* stack, const char* axis, const char* op) {
  if (stack->top < 0) {
    fprintf(stderr, "Stack underflow: need a matrix to compute reduction.\n");
    return;
  }

  if ((!axis) || (!(strcmp(axis, "row") == 0) && (!(strcmp(axis, "col") == 0)))) {
    fprintf(stderr, "Invalid axis: must be \"row\" or \"col\".\n");
    return;
  }

  if (!op || (
      strcmp(op, "sum") != 0 &&
      strcmp(op, "mean") != 0 &&
      strcmp(op, "var") != 0 &&
      strcmp(op, "min") != 0 &&
      strcmp(op, "max") != 0)) {
    fprintf(stderr, "Invalid operation: must be \"sum\", \"mean\", \"var\", \"min\", or \"max\".\n");
    return;
  }

  stack_element* top = &stack->items[stack->top];
  bool compute_rows = strcmp(axis, "row") == 0;
  bool compute_cols = strcmp(axis, "col") == 0;
  bool do_sum  = strcmp(op, "sum") == 0;
  bool do_mean = strcmp(op, "mean") == 0;
  bool do_var  = strcmp(op, "var") == 0;
  bool do_min  = strcmp(op, "min") == 0;
  bool do_max  = strcmp(op, "max") == 0;

  if (top->type == TYPE_MATRIX_REAL) {
    gsl_matrix* mat = top->matrix_real;
    if (!mat) {
      fprintf(stderr, "Null real matrix.\n");
      return;
    }
    size_t rows = mat->size1;
    size_t cols = mat->size2;
    gsl_matrix* result = NULL;

    if (compute_rows) {
      result = gsl_matrix_calloc(rows, 1);
      for (size_t i = 0; i < rows; ++i) {
        double acc = 0.0, acc_sq = 0.0;
        double extreme = do_max ? -GSL_POSINF : GSL_POSINF;
        for (size_t j = 0; j < cols; ++j) {
          double val = gsl_matrix_get(mat, i, j);
          acc += val;
          acc_sq += val * val;
          if (do_min && val < extreme) extreme = val;
          if (do_max && val > extreme) extreme = val;
        }
        double res = 0.0;
        if (do_sum) res = acc;
        else if (do_mean) res = acc / cols;
        else if (do_var) {
          double mean = acc / cols;
          res = (acc_sq - cols * mean * mean) / (cols - 1);
        } else if (do_min || do_max) {
          res = extreme;
        }
        gsl_matrix_set(result, i, 0, res);
      }
    } else if (compute_cols) {
      result = gsl_matrix_calloc(1, cols);
      for (size_t j = 0; j < cols; ++j) {
        double acc = 0.0, acc_sq = 0.0;
        double extreme = do_max ? -GSL_POSINF : GSL_POSINF;
        for (size_t i = 0; i < rows; ++i) {
          double val = gsl_matrix_get(mat, i, j);
          acc += val;
          acc_sq += val * val;
          if (do_min && val < extreme) extreme = val;
          if (do_max && val > extreme) extreme = val;
        }
        double res = 0.0;
        if (do_sum) res = acc;
        else if (do_mean) res = acc / rows;
        else if (do_var) {
          double mean = acc / rows;
          res = (acc_sq - rows * mean * mean) / (rows - 1);
        } else if (do_min || do_max) {
          res = extreme;
        }
        gsl_matrix_set(result, 0, j, res);
      }
    }

    stack_element out = {.type = TYPE_MATRIX_REAL, .matrix_real = result};
    if (stack->top + 1 >= STACK_SIZE) {
      fprintf(stderr, "Stack overflow.\n");
      gsl_matrix_free(result);
      return;
    }
    stack->items[++stack->top] = out;

  } else if (top->type == TYPE_MATRIX_COMPLEX) {
    gsl_matrix_complex* mat = top->matrix_complex;
    if (!mat) {
      fprintf(stderr, "Null complex matrix.\n");
      return;
    }
    size_t rows = mat->size1;
    size_t cols = mat->size2;
    gsl_matrix_complex* result = NULL;

    if (compute_rows) {
      result = gsl_matrix_complex_calloc(rows, 1);
      for (size_t i = 0; i < rows; ++i) {
        gsl_complex acc = gsl_complex_rect(0.0, 0.0);
        gsl_complex acc_sq = gsl_complex_rect(0.0, 0.0);
        double maxabs = -GSL_POSINF, minabs = GSL_POSINF;
        gsl_complex maxz = gsl_complex_rect(0, 0), minz = gsl_complex_rect(0, 0);

        for (size_t j = 0; j < cols; ++j) {
          gsl_complex val = gsl_matrix_complex_get(mat, i, j);
          acc = gsl_complex_add(acc, val);
          gsl_complex val_sq = gsl_complex_mul(val, gsl_complex_conjugate(val));
          acc_sq = gsl_complex_add(acc_sq, val_sq);

          double absval = gsl_complex_abs(val);
          if (do_max && absval > maxabs) { maxabs = absval; maxz = val; }
          if (do_min && absval < minabs) { minabs = absval; minz = val; }
        }
        gsl_complex res = gsl_complex_rect(0.0, 0.0);
        if (do_sum) res = acc;
        else if (do_mean) res = gsl_complex_div_real(acc, (double)cols);
        else if (do_var) {
          double mean_norm = gsl_complex_abs(acc) / cols;
          double acc_sq_real = GSL_REAL(acc_sq);
          res = gsl_complex_rect((acc_sq_real - cols * mean_norm * mean_norm) / (cols - 1), 0.0);
        } else if (do_max) res = maxz;
        else if (do_min) res = minz;

        gsl_matrix_complex_set(result, i, 0, res);
      }
    } else if (compute_cols) {
      result = gsl_matrix_complex_calloc(1, cols);
      for (size_t j = 0; j < cols; ++j) {
        gsl_complex acc = gsl_complex_rect(0.0, 0.0);
        gsl_complex acc_sq = gsl_complex_rect(0.0, 0.0);
        double maxabs = -GSL_POSINF, minabs = GSL_POSINF;
        gsl_complex maxz = gsl_complex_rect(0, 0), minz = gsl_complex_rect(0, 0);

        for (size_t i = 0; i < rows; ++i) {
          gsl_complex val = gsl_matrix_complex_get(mat, i, j);
          acc = gsl_complex_add(acc, val);
          gsl_complex val_sq = gsl_complex_mul(val, gsl_complex_conjugate(val));
          acc_sq = gsl_complex_add(acc_sq, val_sq);

          double absval = gsl_complex_abs(val);
          if (do_max && absval > maxabs) { maxabs = absval; maxz = val; }
          if (do_min && absval < minabs) { minabs = absval; minz = val; }
        }
        gsl_complex res = gsl_complex_rect(0.0, 0.0);
        if (do_sum) res = acc;
        else if (do_mean) res = gsl_complex_div_real(acc, (double)rows);
        else if (do_var) {
          double mean_norm = gsl_complex_abs(acc) / rows;
          double acc_sq_real = GSL_REAL(acc_sq);
          res = gsl_complex_rect((acc_sq_real - rows * mean_norm * mean_norm) / (rows - 1), 0.0);
        } else if (do_max) res = maxz;
        else if (do_min) res = minz;

        gsl_matrix_complex_set(result, 0, j, res);
      }
    }

    stack_element out = {.type = TYPE_MATRIX_COMPLEX, .matrix_complex = result};
    if (stack->top + 1 >= STACK_SIZE) {
      fprintf(stderr, "Stack overflow.\n");
      gsl_matrix_complex_free(result);
      return;
    }
    stack->items[++stack->top] = out;

  } else {
    fprintf(stderr, "Type error: top stack item must be a matrix (real or complex).\n");
  }
}
