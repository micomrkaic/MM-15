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
#include <gsl/gsl_complex.h>                // for GSL_IMAG, GSL_REAL, gsl_c...
#include <gsl/gsl_complex_math.h>           // for gsl_complex_rect
#include <gsl/gsl_matrix_complex_double.h>  // for gsl_matrix_complex_alloc
#include <gsl/gsl_matrix_double.h>          // for gsl_matrix_alloc, gsl_mat...
#include <stdbool.h>                        // for false, true
#include <stdio.h>                          // for fprintf, printf, sscanf
#include <stdlib.h>                         // for size_t, NULL, free
#include <string.h>                         // for strchr, strcmp, strdup
#include "stack.h"                          // for (anonymous struct)::(anon...
#include "registers.h"                      // for registers, MAX_REG, Register

stack_element copy_element(const stack_element* src) {
  stack_element copy;
  copy.type = src->type;

  switch (src->type) {
  case TYPE_REAL:
    copy.real = src->real;
    break;

  case TYPE_COMPLEX:
    copy.complex_val = src->complex_val;
    break;

  case TYPE_STRING:
    copy.string = strdup(src->string);
    break;

  case TYPE_MATRIX_REAL:
    if (src->matrix_real) {
      size_t r = src->matrix_real->size1;
      size_t c = src->matrix_real->size2;
      copy.matrix_real = gsl_matrix_alloc(r, c);
      gsl_matrix_memcpy(copy.matrix_real, src->matrix_real);
    } else {
      copy.matrix_real = NULL;
    }
    break;

  case TYPE_MATRIX_COMPLEX:
    if (src->matrix_complex) {
      size_t r = src->matrix_complex->size1;
      size_t c = src->matrix_complex->size2;
      copy.matrix_complex = gsl_matrix_complex_alloc(r, c);
      gsl_matrix_complex_memcpy(copy.matrix_complex, src->matrix_complex);
    } else {
      copy.matrix_complex = NULL;
    }
    break;
  }

  return copy;
}


void free_element(stack_element* el) {
  switch (el->type) {
  case TYPE_STRING:
    free(el->string);
    break;
  case TYPE_MATRIX_REAL:
    gsl_matrix_free(el->matrix_real);
    break;
  case TYPE_MATRIX_COMPLEX:
    gsl_matrix_complex_free(el->matrix_complex);
    break;
  default:
    break;
  }
  el->type = TYPE_REAL;
  el->real = 0.0;
}


void store_to_register(Stack* stack) {
  if (stack->top < 0) {
    fprintf(stderr, "Stack underflow: need value and register index.\n");
    return;
  }

  stack_element reg_elem = stack->items[stack->top - 1];
  stack_element* value_elem = &stack->items[stack->top];

  if (reg_elem.type != TYPE_REAL) {
    fprintf(stderr, "Type error: register index must be a real number.\n");
    return;
  }

  int reg_index = (int)reg_elem.real;
  if (reg_index < 0 || reg_index >= MAX_REG) {
    fprintf(stderr, "Invalid register index: %d\n", reg_index);
    return;
  }

  // Free old value if occupied
  if (registers[reg_index].occupied) {
    free_element(&registers[reg_index].value);
  }

  registers[reg_index].value = copy_element(value_elem);
  registers[reg_index].occupied = true;

  stack->top -= 2;  // remove reg index and value
}

void recall_from_register(Stack* stack) {
  if (stack->top < 0) {
    fprintf(stderr, "Stack underflow: need register index.\n");
    return;
  }

  stack_element reg_elem = stack->items[stack->top--];

  if (reg_elem.type != TYPE_REAL) {
    fprintf(stderr, "Type error: register index must be a real number.\n");
    return;
  }

  int reg_index = (int)reg_elem.real;
  if (reg_index < 0 || reg_index >= MAX_REG) {
    fprintf(stderr, "Invalid register index: %d\n", reg_index);
    return;
  }

  if (!registers[reg_index].occupied) {
    fprintf(stderr, "Register %d is empty.\n", reg_index);
    return;
  }

  stack_element copy = copy_element(&registers[reg_index].value);

  if (stack->top + 1 >= STACK_SIZE) {
    fprintf(stderr, "Stack overflow.\n");
    return;
  }

  stack->items[++stack->top] = copy;
}

void show_registers_status(void) {
  printf("Register status (%d total):\n", MAX_REG);
  for (int i = 0; i < MAX_REG; ++i) {
    printf("[%c]", registers[i].occupied ? 'x' : ' ');
    if ((i + 1) % 8 == 0) {
      printf(" -> R[%2d–%2d]\n", i - 7, i);
    }
  }
  if (MAX_REG % 8 != 0) printf("\n");  // final newline if not aligned
}

void init_registers(void) {
  for (int i = 0; i < MAX_REG; ++i) {
    registers[i].occupied = false;
    registers[i].value.type = TYPE_REAL;
    registers[i].value.real = 0.0;
  }
}

void free_all_registers(void) {
  for (int i = 0; i < MAX_REG; ++i) {
    if (registers[i].occupied) {
      free_element(&registers[i].value);  // deep free
      registers[i].occupied = false;
    }
  }
}

void save_registers_to_file(const char* filename) {
  FILE* f = fopen(filename, "w");
  if (!f) {
    perror("fopen");
    return;
  }

  for (int i = 0; i < MAX_REG; ++i) {
    if (!registers[i].occupied) continue;

    stack_element* el = &registers[i].value;
    fprintf(f, "REG %d ", i);

    switch (el->type) {
    case TYPE_REAL:
      fprintf(f, "REAL %.17g\n", el->real);
      break;
    case TYPE_COMPLEX:
      fprintf(f, "COMPLEX (%.17g,%.17g)\n", GSL_REAL(el->complex_val), GSL_IMAG(el->complex_val));
      break;
    case TYPE_STRING:
      fprintf(f, "STRING \"%s\"\n", el->string);
      break;
    case TYPE_MATRIX_REAL: {
      gsl_matrix* m = el->matrix_real;
      fprintf(f, "MATRIX_REAL %zu %zu", m->size1, m->size2);
      for (size_t i_cnt = 0; i_cnt < m->size1 * m->size2; ++i_cnt) {
	fprintf(f, " %.17g", m->data[i_cnt]);
      }
      fprintf(f, "\n");
      break;
    }
    case TYPE_MATRIX_COMPLEX: {
      gsl_matrix_complex* m = el->matrix_complex;
      fprintf(f, "MATRIX_COMPLEX %zu %zu", m->size1, m->size2);
      for (size_t i_cnt = 0; i_cnt < m->size1; ++i_cnt) {
        for (size_t j = 0; j < m->size2; ++j) {
	  gsl_complex z = gsl_matrix_complex_get(m, i_cnt, j);
	  fprintf(f, " (%.17g,%.17g)", GSL_REAL(z), GSL_IMAG(z));
        }
      }
    }
    }
  }
  fclose(f);
  printf("Registers saved to %s\n", filename);
}

void load_registers_from_file(const char* filename) {
  FILE* f = fopen(filename, "r");
  if (!f) {
    perror("fopen");
    return;
  }

  // Clear all first
  free_all_registers();

  char line[4096];
  while (fgets(line, sizeof(line), f)) {
    int index;
    char type[32];

    if (sscanf(line, "REG %d %31s", &index, type) != 2) continue;
    if (index < 0 || index >= MAX_REG) continue;

    stack_element el;
    if (strcmp(type, "REAL") == 0) {
      double val;
      sscanf(line, "REG %*d %*s %lf", &val);
      el.type = TYPE_REAL;
      el.real = val;
    } else if (strcmp(type, "COMPLEX") == 0) {
      double re, im;
      sscanf(line, "REG %*d %*s (%lf,%lf)", &re, &im);
      el.type = TYPE_COMPLEX;
      el.complex_val = gsl_complex_rect(re,im);
    } else if (strcmp(type, "STRING") == 0) {
      char* start = strchr(line, '"');
      char* end = strrchr(line, '"');
      if (!start || !end || start == end) continue;
      *end = '\0';
      el.type = TYPE_STRING;
      el.string = strdup(start + 1);
    } else if (strcmp(type, "MATRIX_REAL") == 0) {
      size_t r, c;
      char* ptr = strchr(line, ' ') + 1; // skip "REG"
      ptr = strchr(ptr, ' ') + 1; // skip index
      ptr = strchr(ptr, ' ') + 1; // skip type
      sscanf(ptr, "%zu %zu", &r, &c);
      el.type = TYPE_MATRIX_REAL;
      el.matrix_real = gsl_matrix_alloc(r, c);
      ptr = strchr(ptr, ' ') + 1; // skip rows
      ptr = strchr(ptr, ' ') + 1; // skip cols
      for (size_t i = 0; i < r * c; ++i) {
	double val;
	sscanf(ptr, "%lf", &val);
	el.matrix_real->data[i] = val;
	ptr = strchr(ptr, ' ');
	if (ptr) ptr++;
      }
    } else if (strcmp(type, "MATRIX_COMPLEX") == 0) {
      size_t r, c;
      char* ptr = strchr(line, ' ') + 1;
      ptr = strchr(ptr, ' ') + 1;
      ptr = strchr(ptr, ' ') + 1;
      sscanf(ptr, "%zu %zu", &r, &c);
      el.type = TYPE_MATRIX_COMPLEX;
      el.matrix_complex = gsl_matrix_complex_alloc(r, c);
      ptr = strchr(ptr, ' ') + 1;
      ptr = strchr(ptr, ' ') + 1;
      for (size_t i = 0; i < r * c; ++i) {
	double re = 0, im = 0;
	sscanf(ptr, " (%lf,%lf)", &re, &im);
	gsl_matrix_complex_set(el.matrix_complex, i / c, i % c, gsl_complex_rect(re, im));
	ptr = strchr(ptr + 1, '(');
      }
    } else {
      continue;
    }

    registers[index].value = el;
    registers[index].occupied = true;
  }

  fclose(f);
  printf("Registers loaded from %s\n", filename);
}

#include <stdbool.h>

#define MAX_REG 64

extern Register registers[MAX_REG];  // Your global register array

void find_first_free_register(Stack *stack) {
  for (int i = 0; i < MAX_REG; ++i) {
    if (!registers[i].occupied) {
      push_real(stack,(double)i);
      return;
    }
  }
  printf("All registers are occupied\n");    
}
