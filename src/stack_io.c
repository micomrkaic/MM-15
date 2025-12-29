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

#include "stack_io.h"
#include <errno.h>                          // for EINVAL, errno
#include <gsl/gsl_complex.h>                // for GSL_IMAG, GSL_REAL, gsl_c...
#include <gsl/gsl_complex_math.h>           // for gsl_complex_rect
#include <gsl/gsl_matrix_complex_double.h>  // for gsl_matrix_complex_alloc
#include <gsl/gsl_matrix_double.h>          // for gsl_matrix_alloc, gsl_mat...
#include <stdint.h>                         // for uint32_t, uint8_t, uint16_t
#include <stdio.h>                          // for fprintf, perror, stderr
#include <stdlib.h>                         // for free, malloc, mkstemp
#include <string.h>                         // for strlen, memcpy, memcmp
#include <unistd.h>                         // for unlink, close, fsync

/* -----------------------------
 * On-disk format (v1)
 * -----------------------------
 *
 * Header:
 *   magic[8]      = "MM15STK\0"
 *   version u32   = 1
 *   endian  u8    = 1 (LE) or 2 (BE)
 *   reserved[3]   = 0
 *   count   u32   = number of elements on stack (top+1), 0 allowed
 *
 * Each element:
 *   type u32
 *   payload depends on type
 *
 * Payloads:
 *   REAL:          f64
 *   COMPLEX:       f64 re, f64 im
 *   STRING:        u32 byte_len (N, no NUL), then N bytes
 *   MATRIX_REAL:   u32 rows, u32 cols, then rows*cols f64 (row-major)
 *   MATRIX_COMPLEX:u32 rows, u32 cols, then rows*cols pairs (re f64, im f64)
 */

#define MM15_STK_MAGIC_LEN 8
static const unsigned char MM15_STK_MAGIC[MM15_STK_MAGIC_LEN] = {
  'M','M','1','5','S','T','K','\0'
};

#define MM15_STK_VERSION 1u

/* Hard caps to prevent stupid/corrupt files from allocating the universe. */
#define MM15_MAX_STRING_BYTES (1024u * 1024u)   /* 1 MiB per string */
#define MM15_MAX_MATRIX_DIM   20000u            /* sanity cap, tune as you like */

/* -----------------------------
 * Little helpers
 * ----------------------------- */

static int host_is_little_endian(void) {
  uint16_t x = 1;
  return *((uint8_t *)&x) == 1;
}

static uint8_t host_endian_tag(void) {
  return host_is_little_endian() ? 1u : 2u;
}

static int write_exact(FILE *f, const void *buf, size_t n, const char *what) {
  if (n == 0) return 0;
  if (fwrite(buf, 1, n, f) != n) {
    perror(what);
    return -1;
  }
  return 0;
}

static int read_exact(FILE *f, void *buf, size_t n, const char *what) {
  if (n == 0) return 0;
  if (fread(buf, 1, n, f) != n) {
    if (feof(f)) fprintf(stderr, "Unexpected EOF while reading %s\n", what);
    else perror(what);
    return -1;
  }
  return 0;
}

static int write_u32(FILE *f, uint32_t v, const char *what) {
  return write_exact(f, &v, sizeof(v), what);
}

static int read_u32(FILE *f, uint32_t *v, const char *what) {
  return read_exact(f, v, sizeof(*v), what);
}

static int write_u8(FILE *f, uint8_t v, const char *what) {
  return write_exact(f, &v, sizeof(v), what);
}

static int read_u8(FILE *f, uint8_t *v, const char *what) {
  return read_exact(f, v, sizeof(*v), what);
}

static int write_f64(FILE *f, double x, const char *what) {
  return write_exact(f, &x, sizeof(x), what);
}

static int read_f64(FILE *f, double *x, const char *what) {
  return read_exact(f, x, sizeof(*x), what);
}

/* Write atomic: filename.tmpXXXXXX in same directory, then rename(). */
static int open_atomic_tmp(const char *filename, char **tmp_path_out, FILE **file_out, int *fd_out) {
  size_t n = strlen(filename);
  const char *suffix = ".tmpXXXXXX";
  size_t m = strlen(suffix);

  char *tmp = (char *)malloc(n + m + 1);
  if (!tmp) {
    perror("malloc tmp path");
    return -1;
  }

  memcpy(tmp, filename, n);
  memcpy(tmp + n, suffix, m + 1);

  int fd = mkstemp(tmp);
  if (fd < 0) {
    perror("mkstemp");
    free(tmp);
    return -1;
  }

  FILE *f = fdopen(fd, "wb");
  if (!f) {
    perror("fdopen");
    close(fd);
    unlink(tmp);
    free(tmp);
    return -1;
  }

  *tmp_path_out = tmp;
  *file_out = f;
  *fd_out = fd; /* same as fileno(f), but keep it explicit */
  return 0;
}

/* You should already have something like this; if so, use yours.
 * This version is defensive and only frees what it knows. */
static void stack_clear_for_load(Stack *stack) {
  if (!stack) return;

  /* If your stack uses top=-1 for empty, keep that convention. */
  if (stack->top < 0) return;

  for (int i = 0; i <= stack->top; ++i) {
    stack_element *e = &stack->items[i];
    switch (e->type) {
    case TYPE_STRING:
      free(e->string);
      e->string = NULL;
      break;
    case TYPE_MATRIX_REAL:
      if (e->matrix_real) gsl_matrix_free(e->matrix_real);
      e->matrix_real = NULL;
      break;
    case TYPE_MATRIX_COMPLEX:
      if (e->matrix_complex) gsl_matrix_complex_free(e->matrix_complex);
      e->matrix_complex = NULL;
      break;
    default:
      /* Scalars: nothing to free. */
      break;
    }
    /* Optional: set type to something safe */
    /* e->type = TYPE_REAL; e->real = 0.0; */
  }
  stack->top = -1;
}

/* -----------------------------
 * Public API
 * ----------------------------- */

int save_stack_to_file(const Stack *stack, const char *filename) {
  if (!stack || !filename) {
    errno = EINVAL;
    return -1;
  }

  /* Validate top before writing. Your convention might differ. */
  if (stack->top < -1 || stack->top >= STACK_SIZE) {
    fprintf(stderr, "save_stack_to_file: invalid stack->top=%d\n", stack->top);
    errno = EINVAL;
    return -1;
  }

  uint32_t count = (stack->top < 0) ? 0u : (uint32_t)(stack->top + 1);

  char *tmp_path = NULL;
  FILE *file = NULL;
  int fd = -1;

  if (open_atomic_tmp(filename, &tmp_path, &file, &fd) != 0) {
    return -1;
  }

  int rc = -1; /* assume failure */

  /* Header */
  if (write_exact(file, MM15_STK_MAGIC, MM15_STK_MAGIC_LEN, "write magic") != 0) goto done;
  if (write_u32(file, MM15_STK_VERSION, "write version") != 0) goto done;
  if (write_u8(file, host_endian_tag(), "write endian") != 0) goto done;

  unsigned char reserved[3] = {0,0,0};
  if (write_exact(file, reserved, sizeof(reserved), "write reserved") != 0) goto done;

  if (write_u32(file, count, "write count") != 0) goto done;

  /* Elements */
  for (uint32_t i = 0; i < count; ++i) {
    const stack_element *elem = &stack->items[i];

    uint32_t type_u32 = (uint32_t)elem->type;
    if (write_u32(file, type_u32, "write elem type") != 0) goto done;

    switch (elem->type) {
    case TYPE_REAL:
      if (write_f64(file, elem->real, "write real") != 0) goto done;
      break;

    case TYPE_COMPLEX: {
      /* Always serialize as (re, im) doubles. */
      double re = GSL_REAL(elem->complex_val);
      double im = GSL_IMAG(elem->complex_val);
      if (write_f64(file, re, "write complex re") != 0) goto done;
      if (write_f64(file, im, "write complex im") != 0) goto done;
      break;
    }

    case TYPE_STRING: {
      if (!elem->string) {
	fprintf(stderr, "save_stack_to_file: NULL string at index %u\n", i);
	goto done;
      }
      size_t len = strlen(elem->string);
      if (len > MM15_MAX_STRING_BYTES) {
	fprintf(stderr, "save_stack_to_file: string too large (%zu bytes)\n", len);
	goto done;
      }
      if (write_u32(file, (uint32_t)len, "write string len") != 0) goto done;
      if (write_exact(file, elem->string, len, "write string bytes") != 0) goto done;
      break;
    }

    case TYPE_MATRIX_REAL: {
      if (!elem->matrix_real) {
	fprintf(stderr, "save_stack_to_file: NULL real matrix at index %u\n", i);
	goto done;
      }
      uint32_t rows = (uint32_t)elem->matrix_real->size1;
      uint32_t cols = (uint32_t)elem->matrix_real->size2;

      if (rows == 0 || cols == 0 || rows > MM15_MAX_MATRIX_DIM || cols > MM15_MAX_MATRIX_DIM) {
	fprintf(stderr, "save_stack_to_file: insane matrix dims %u x %u\n", rows, cols);
	goto done;
      }

      if (write_u32(file, rows, "write mreal rows") != 0) goto done;
      if (write_u32(file, cols, "write mreal cols") != 0) goto done;

      /* Robust to tda != cols */
      for (uint32_t r = 0; r < rows; ++r) {
	for (uint32_t c = 0; c < cols; ++c) {
	  double x = gsl_matrix_get(elem->matrix_real, r, c);
	  if (write_f64(file, x, "write mreal elem") != 0) goto done;
	}
      }
      break;
    }

    case TYPE_MATRIX_COMPLEX: {
      if (!elem->matrix_complex) {
	fprintf(stderr, "save_stack_to_file: NULL complex matrix at index %u\n", i);
	goto done;
      }

      uint32_t rows = (uint32_t)elem->matrix_complex->size1;
      uint32_t cols = (uint32_t)elem->matrix_complex->size2;

      if (rows == 0 || cols == 0 || rows > MM15_MAX_MATRIX_DIM || cols > MM15_MAX_MATRIX_DIM) {
	fprintf(stderr, "save_stack_to_file: insane complex matrix dims %u x %u\n", rows, cols);
	goto done;
      }

      if (write_u32(file, rows, "write mcplx rows") != 0) goto done;
      if (write_u32(file, cols, "write mcplx cols") != 0) goto done;

      for (uint32_t r = 0; r < rows; ++r) {
	for (uint32_t c = 0; c < cols; ++c) {
	  gsl_complex z = gsl_matrix_complex_get(elem->matrix_complex, r, c);
	  double re = GSL_REAL(z);
	  double im = GSL_IMAG(z);
	  if (write_f64(file, re, "write mcplx re") != 0) goto done;
	  if (write_f64(file, im, "write mcplx im") != 0) goto done;
	}
      }
      break;
    }

    default:
      fprintf(stderr, "save_stack_to_file: Unknown type: %u\n", type_u32);
      goto done;
    }
  }

  if (fflush(file) != 0) {
    perror("fflush");
    goto done;
  }

  /* Try to make it durable; if you don’t care about power-loss safety, you can drop fsync. */
  if (fsync(fd) != 0) {
    perror("fsync");
    goto done;
  }

  if (fclose(file) != 0) {
    perror("fclose");
    file = NULL;
    goto done;
  }
  file = NULL;

  if (rename(tmp_path, filename) != 0) {
    perror("rename");
    goto done;
  }

  rc = 0;

 done:
  if (file) fclose(file);
  if (rc != 0 && tmp_path) unlink(tmp_path);
  free(tmp_path);
  return rc;
}

int load_stack_from_file(Stack *stack, const char *filename) {
  if (!stack || !filename) {
    errno = EINVAL;
    return -1;
  }

  FILE *file = fopen(filename, "rb");
  if (!file) {
    perror("fopen");
    return -1;
  }

  int rc = -1;

  /* Header */
  unsigned char magic[MM15_STK_MAGIC_LEN];
  if (read_exact(file, magic, MM15_STK_MAGIC_LEN, "read magic") != 0) goto done;
  if (memcmp(magic, MM15_STK_MAGIC, MM15_STK_MAGIC_LEN) != 0) {
    fprintf(stderr, "load_stack_from_file: not an MM-15 stack file (bad magic)\n");
    goto done;
  }

  uint32_t version = 0;
  if (read_u32(file, &version, "read version") != 0) goto done;
  if (version != MM15_STK_VERSION) {
    fprintf(stderr, "load_stack_from_file: unsupported version %u\n", version);
    goto done;
  }

  uint8_t endian = 0;
  if (read_u8(file, &endian, "read endian") != 0) goto done;
  if (endian != host_endian_tag()) {
    fprintf(stderr, "load_stack_from_file: endian mismatch (file=%u host=%u)\n",
            (unsigned)endian, (unsigned)host_endian_tag());
    goto done;
  }

  unsigned char reserved[3];
  if (read_exact(file, reserved, sizeof(reserved), "read reserved") != 0) goto done;

  uint32_t count = 0;
  if (read_u32(file, &count, "read count") != 0) goto done;

  if (count > (uint32_t)STACK_SIZE) {
    fprintf(stderr, "load_stack_from_file: file stack too large (%u > STACK_SIZE)\n", count);
    goto done;
  }

  /* Clear stack first to avoid leaks */
  stack_clear_for_load(stack);

  /* Populate */
  if (count == 0) {
    stack->top = -1;
    rc = 0;
    goto done;
  }

  stack->top = (int)count - 1;

  for (uint32_t i = 0; i < count; ++i) {
    stack_element *elem = &stack->items[i];

    uint32_t type_u32 = 0;
    if (read_u32(file, &type_u32, "read elem type") != 0) goto done;
    elem->type = (value_type)type_u32;

    switch (elem->type) {
    case TYPE_REAL: {
      double x = 0.0;
      if (read_f64(file, &x, "read real") != 0) goto done;
      elem->real = x;
      break;
    }

    case TYPE_COMPLEX: {
      double re = 0.0, im = 0.0;
      if (read_f64(file, &re, "read complex re") != 0) goto done;
      if (read_f64(file, &im, "read complex im") != 0) goto done;
      elem->complex_val = gsl_complex_rect(re, im);
      break;
    }

    case TYPE_STRING: {
      uint32_t len = 0;
      if (read_u32(file, &len, "read string len") != 0) goto done;
      if (len > MM15_MAX_STRING_BYTES) {
	fprintf(stderr, "load_stack_from_file: string length too large (%u)\n", len);
	goto done;
      }

      elem->string = (char *)malloc((size_t)len + 1);
      if (!elem->string) {
	perror("malloc string");
	goto done;
      }
      if (read_exact(file, elem->string, len, "read string bytes") != 0) goto done;
      elem->string[len] = '\0';
      break;
    }

    case TYPE_MATRIX_REAL: {
      uint32_t rows = 0, cols = 0;
      if (read_u32(file, &rows, "read mreal rows") != 0) goto done;
      if (read_u32(file, &cols, "read mreal cols") != 0) goto done;

      if (rows == 0 || cols == 0 || rows > MM15_MAX_MATRIX_DIM || cols > MM15_MAX_MATRIX_DIM) {
	fprintf(stderr, "load_stack_from_file: insane matrix dims %u x %u\n", rows, cols);
	goto done;
      }

      elem->matrix_real = gsl_matrix_alloc(rows, cols);
      if (!elem->matrix_real) {
	fprintf(stderr, "Failed to allocate matrix_real (%u x %u)\n", rows, cols);
	goto done;
      }

      for (uint32_t r = 0; r < rows; ++r) {
	for (uint32_t c = 0; c < cols; ++c) {
	  double x = 0.0;
	  if (read_f64(file, &x, "read mreal elem") != 0) goto done;
	  gsl_matrix_set(elem->matrix_real, r, c, x);
	}
      }
      break;
    }

    case TYPE_MATRIX_COMPLEX: {
      uint32_t rows = 0, cols = 0;
      if (read_u32(file, &rows, "read mcplx rows") != 0) goto done;
      if (read_u32(file, &cols, "read mcplx cols") != 0) goto done;

      if (rows == 0 || cols == 0 || rows > MM15_MAX_MATRIX_DIM || cols > MM15_MAX_MATRIX_DIM) {
	fprintf(stderr, "load_stack_from_file: insane complex matrix dims %u x %u\n", rows, cols);
	goto done;
      }

      elem->matrix_complex = gsl_matrix_complex_alloc(rows, cols);
      if (!elem->matrix_complex) {
	fprintf(stderr, "Failed to allocate matrix_complex (%u x %u)\n", rows, cols);
	goto done;
      }

      for (uint32_t r = 0; r < rows; ++r) {
	for (uint32_t c = 0; c < cols; ++c) {
	  double re = 0.0, im = 0.0;
	  if (read_f64(file, &re, "read mcplx re") != 0) goto done;
	  if (read_f64(file, &im, "read mcplx im") != 0) goto done;
	  gsl_matrix_complex_set(elem->matrix_complex, r, c, gsl_complex_rect(re, im));
	}
      }
      break;
    }

    default:
      fprintf(stderr, "load_stack_from_file: Unknown type: %u\n", type_u32);
      goto done;
    }
  }

  rc = 0;

 done:
  if (rc != 0) {
    /* On failure, avoid leaving partially-loaded allocated stuff around. */
    stack_clear_for_load(stack);
  }
  fclose(file);
  return rc;
}
