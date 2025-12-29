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
#include <ctype.h>                          // for isspace, isdigit
#include <gsl/gsl_complex.h>                // for gsl_complex
#include <gsl/gsl_complex_math.h>           // for gsl_complex_rect
#include <gsl/gsl_matrix_complex_double.h>  // for gsl_matrix_complex, gsl_m...
#include <gsl/gsl_matrix_double.h>          // for gsl_matrix, gsl_matrix_alloc
#include <stdio.h>                          // for fprintf, stderr, NULL
#include <stdlib.h>                         // for free, strtod, strtoul
#include "math_parsers.h"                   // for parse_complex_matrix_literal
#include "stack.h"                          // for load_matrix_from_file

int read_complex(const char* input, gsl_complex* z) {
  double real, imag;
  if (sscanf(input, " ( %lf , %lf ) ", &real, &imag) == 2) {
    *z = gsl_complex_rect(real, imag);
    return 1; // success
  }
  return 0; // failure
}

void read_matrix_from_file(Stack *stack, char *input) {
  int rows, cols;
  char filename[1024];
  if (sscanf(input, "[%d,%d,\"%255[^\"]\"]", &rows, &cols, filename) == 3) {
    printf("rows = %d\n", rows);
    printf("cols = %d\n", cols);
    printf("filename = %s\n", filename);
  } else {
    fprintf(stderr, "Failed to parse input string\n");
  }
  push_matrix_real(stack,load_matrix_from_file(rows, cols, filename));
}

// Parse real matrices: rows cols $ list of real numbers
gsl_matrix* parse_matrix_literal(const char* input) {
  const char* p = input;
  while (isspace(*p)) p++;

  char* endptr;
  size_t rows = strtoul(p, &endptr, 10);
  if (p == endptr) {
    fprintf(stderr, "Expected number of rows\n");
    return NULL;
  }
  p = endptr;

  while (isspace(*p)) p++;

  size_t cols = strtoul(p, &endptr, 10);
  if (p == endptr) {
    fprintf(stderr, "Expected number of columns\n");
    return NULL;
  }
  p = endptr;

  while (isspace(*p)) p++;

  if (*p != '$') {
    fprintf(stderr, "Expected '$' after rows and columns\n");
    return NULL;
  }
  p++; // skip '$'

  double* data = malloc(rows * cols * sizeof(double));
  if (!data) {
    fprintf(stderr, "Memory allocation failed\n");
    return NULL;
  }

  size_t count = 0;
  while (count < rows * cols && *p) {
    while (isspace(*p)) p++;

    double val = strtod(p, &endptr);
    if (p == endptr) {
      fprintf(stderr, "Invalid real number at entry %zu\n", count);
      free(data);
      return NULL;
    }
    p = endptr;
    data[count++] = val;
  }

  if (count != rows * cols) {
    fprintf(stderr, "Matrix element count mismatch: expected %zu, got %zu\n", rows * cols, count);
    free(data);
    return NULL;
  }

  gsl_matrix* m = gsl_matrix_alloc(rows, cols);
  if (!m) {
    fprintf(stderr, "Matrix allocation failed\n");
    free(data);
    return NULL;
  }

  for (size_t i = 0; i < rows; ++i) {
    for (size_t j = 0; j < cols; ++j) {
      gsl_matrix_set(m, i, j, data[i * cols + j]);
    }
  }

  free(data);
  return m;
}

// Parse mixed real/complex matrices: rows cols $ list of real and complex numbers
gsl_matrix_complex* parse_complex_matrix_literal(const char* input) {
  const char* p = input;
  while (isspace(*p)) p++;

  char* endptr;
  size_t rows = strtoul(p, &endptr, 10);
  if (p == endptr) {
    fprintf(stderr, "Expected number of rows\n");
    return NULL;
  }
  p = endptr;

  while (isspace(*p)) p++;

  size_t cols = strtoul(p, &endptr, 10);
  if (p == endptr) {
    fprintf(stderr, "Expected number of columns\n");
    return NULL;
  }
  p = endptr;

  while (isspace(*p)) p++;

  if (*p != '$') {
    fprintf(stderr, "Expected '$' after rows and columns\n");
    return NULL;
  }
  p++; // skip '$'

  gsl_complex* data = malloc(rows * cols * sizeof(gsl_complex));
  if (!data) {
    fprintf(stderr, "Memory allocation failed\n");
    return NULL;
  }

  size_t count = 0;
  while (count < rows * cols && *p) {
    while (isspace(*p)) p++;

    if (*p == '(') {
      p++; // skip '('
      double real = strtod(p, &endptr);
      if (p == endptr) {
	fprintf(stderr, "Invalid real part at entry %zu\n", count);
	free(data);
	return NULL;
      }
      p = endptr;

      while (isspace(*p)) p++;

      if (*p != ',') {
	fprintf(stderr, "Expected ',' between real and imaginary at entry %zu\n", count);
	free(data);
	return NULL;
      }
      p++; // skip ','

      double imag = strtod(p, &endptr);
      if (p == endptr) {
	fprintf(stderr, "Invalid imaginary part at entry %zu\n", count);
	free(data);
	return NULL;
      }
      p = endptr;

      while (isspace(*p)) p++;

      if (*p != ')') {
	fprintf(stderr, "Expected ')' after complex number at entry %zu\n", count);
	free(data);
	return NULL;
      }
      p++; // skip ')'

      data[count++] = gsl_complex_rect(real, imag);
    } else if (isdigit(*p) || (*p == '-' && isdigit(*(p + 1)))) {
      double real = strtod(p, &endptr);
      if (p == endptr) {
	fprintf(stderr, "Invalid real number at entry %zu\n", count);
	free(data);
	return NULL;
      }
      p = endptr;
      data[count++] = gsl_complex_rect(real, 0.0);
    } else {
      fprintf(stderr, "Unexpected character at entry %zu: '%c'\n", count, *p);
      free(data);
      return NULL;
    }
  }

  if (count != rows * cols) {
    fprintf(stderr, "Matrix element count mismatch: expected %zu, got %zu\n", rows * cols, count);
    free(data);
    return NULL;
  }

  gsl_matrix_complex* m = gsl_matrix_complex_alloc(rows, cols);
  if (!m) {
    fprintf(stderr, "Matrix allocation failed\n");
    free(data);
    return NULL;
  }

  for (size_t i = 0; i < rows; ++i) {
    for (size_t j = 0; j < cols; ++j) {
      gsl_matrix_complex_set(m, i, j, data[i * cols + j]);
    }
  }

  free(data);
  return m;
}

