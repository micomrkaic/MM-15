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


// A heterogenous stack with operations for an RPN calculator
// Mico Mrkaic, mrkaic.mico@gmail.com

#define _POSIX_C_SOURCE 200809L
#include <gsl/gsl_complex.h>                // for gsl_complex
#include <gsl/gsl_matrix_complex_double.h>  // for gsl_matrix_complex_alloc
#include <gsl/gsl_matrix_double.h>          // for gsl_matrix_alloc, gsl_mat...
#include <stdio.h>                          // for fclose, fprintf, perror
#include <stdlib.h>                         // for free, malloc
#include <string.h>                         // for strdup, strlen
#include "stack.h"                          // for (anonymous struct)::(anon...

void stack_element_free(stack_element* e) {
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
    break;
  }
}

int stack_element_clone(stack_element* dst, const stack_element* src) {
  dst->type = src->type;
  switch (src->type) {
  case TYPE_REAL:
    dst->real = src->real;
    return 0;
  case TYPE_COMPLEX:
    dst->complex_val = src->complex_val;
    return 0;
  case TYPE_STRING:
    if (!src->string) { dst->string = NULL; return 0; }
    dst->string = strdup(src->string);
    return dst->string ? 0 : -1;
  case TYPE_MATRIX_REAL:
    if (!src->matrix_real) { dst->matrix_real = NULL; return 0; }
    dst->matrix_real = gsl_matrix_alloc(src->matrix_real->size1,
					src->matrix_real->size2);
    if (!dst->matrix_real) return -1;
    gsl_matrix_memcpy(dst->matrix_real, src->matrix_real);
    return 0;
  case TYPE_MATRIX_COMPLEX:
    if (!src->matrix_complex) { dst->matrix_complex = NULL; return 0; }
    dst->matrix_complex = gsl_matrix_complex_alloc(src->matrix_complex->size1,
						   src->matrix_complex->size2);
    if (!dst->matrix_complex) return -1;
    gsl_matrix_complex_memcpy(dst->matrix_complex, src->matrix_complex);
    return 0;
  default:
    return -1;
  }
}

void init_stack(Stack* stack) {
  stack->top = -1;
}

int stack_size(const Stack* stack) {
  return stack->top + 1;
}

void push_real(Stack* stack, double value) {
  if (stack->top >= STACK_SIZE - 1) {
    fprintf(stderr,"Stack overflow\n");
    return;
  }
  stack->top++;
  stack->items[stack->top].type = TYPE_REAL;
  stack->items[stack->top].real = value;
}

void push_complex(Stack* stack, gsl_complex value) {
  if (stack->top >= STACK_SIZE - 1) {
    fprintf(stderr,"Stack overflow\n");
    return;
  }
  stack->top++;
  stack->items[stack->top].type = TYPE_COMPLEX;
  stack->items[stack->top].complex_val = value;
}

void push_string(Stack* stack, const char* str) {
  if (stack->top >= STACK_SIZE - 1) {
    fprintf(stderr,"Stack overflow\n");
    return;
  }
  stack->top++;
  stack->items[stack->top].type = TYPE_STRING;
  stack->items[stack->top].string = strdup(str);
  if (!stack->items[stack->top].string) {
    fprintf(stderr,"Memory allocation failed\n");
    stack->top--;
  }
}

void push_matrix_real(Stack* stack, gsl_matrix* matrix) {
  if (stack->top >= STACK_SIZE - 1) {
    fprintf(stderr,"Stack overflow\n");
    return;
  }
  if (NULL == matrix) {
    fprintf(stderr,"NULL pointer to matrix, exiting!\n");
    return;
  }
  stack->top++;
  stack->items[stack->top].type = TYPE_MATRIX_REAL;
  stack->items[stack->top].matrix_real = matrix;
}

void push_matrix_complex(Stack* stack, gsl_matrix_complex* matrix) {
  if (stack->top >= STACK_SIZE - 1) {
    fprintf(stderr,"Stack overflow\n");
    return;
  }
  stack->top++;
  stack->items[stack->top].type = TYPE_MATRIX_COMPLEX;
  stack->items[stack->top].matrix_complex = matrix;
}

stack_element pop(Stack* stack) {
  stack_element popped;
  if (stack->top < 0) {
    printf("Stack underflow\n");
    popped.type = TYPE_REAL;
    popped.real = 0.0;
    return popped;
  }
  popped = stack->items[stack->top];
  stack->top--;
  return popped;
}

void swap(Stack* stack) {
  if (stack->top < 1) {
    fprintf(stderr,"Too few elements on stack!\n");
  } else {
    stack_element temp_top = stack->items[stack->top];
    stack_element temp_second = stack->items[stack->top-1];
    stack->items[stack->top] = temp_second;
    stack->items[stack->top-1] = temp_top;
  }
}

int stack_dup(Stack* stack) {
  if (stack->top < 0) {
    fprintf(stderr,"Stack is empty! Cannot duplicate.\n");
    return -1; // Error
  }
  if (stack->top + 1 >= STACK_SIZE) {
    fprintf(stderr,"Stack overflow! Cannot duplicate.\n");
    return -1; // Error
  }
  if (stack_element_clone(&stack->items[stack->top + 1],
		    &stack->items[stack->top]) != 0) {
    fprintf(stderr, "dup: failed to duplicate element\n");
    return -1;
  }
  stack->top++;
  return 0; // Success
}

stack_element check_top(Stack* stack) {
  stack_element top;
  if (stack->top < 0) {
    printf("Stack underflow\n");
    top.type = TYPE_REAL;
    top.real = 0.0;
    return top;
  }
  top = stack->items[stack->top];
  return top;
}

stack_element pop_and_free(Stack* stack) {
  stack_element popped;
  popped.type = TYPE_REAL;
  popped.real = 0.0;
  if (stack->top < 0) {
    printf("Stack underflow\n");
    return popped;
  }
  stack_element_free(&stack->items[stack->top]);
  stack->top--;
  return popped;
}

stack_element* view_top(Stack* stack) {
  if (stack->top < 0) {
    fprintf(stderr,"Stack is empty\n");
    return NULL;
  }
  return &stack->items[stack->top];
}

void free_stack(Stack* stack) {
  while (stack->top >= 0) {
    stack_element_free(&stack->items[stack->top]);
    stack->top--;
  }
}

gsl_matrix* load_matrix_from_file(int rows, int cols, const char* filename) {
  FILE* f = fopen(filename, "r");
  if (!f) {
    perror("Failed to open matrix file");
    return NULL;
  }

  gsl_matrix* m = gsl_matrix_alloc(rows, cols);
  if (!m) {
    fclose(f);
    fprintf(stderr, "Failed to allocate matrix.\n");
    return NULL;
  }

  for (int i = 0; i < rows; ++i) {
    for (int j = 0; j < cols; ++j) {
      double val;
      if (fscanf(f, "%lf", &val) != 1) {
	fprintf(stderr, "Failed to read value at [%d, %d] from file '%s'\n", i, j, filename);
	gsl_matrix_free(m);
	fclose(f);
	return NULL;
      }
      gsl_matrix_set(m, i, j, val);
    }
  }

  fclose(f);
  return m;
}

value_type stack_top_type(const Stack* stack) {
  if (stack->top < 0) {
    fprintf(stderr, "Stack is empty\n");
    return -1;  // Or some sentinel value you define
  }
  return stack->items[stack->top].type;
}

value_type stack_next2_top_type(const Stack* stack) {
  if (stack->top < 1) {
    fprintf(stderr, "Stack has fewer than 2 elements\n");
    return -1;  // Or some sentinel value you define
  }
  return stack->items[stack->top-1].type;
}

int copy_stack(Stack* dest, const Stack* src) {
  // Release whatever dest currently owns; otherwise every snapshot/undo
  // abandons the previous deep copy and leaks it.
  free_stack(dest);          // leaves dest->top == -1

  for (int i = 0; i <= src->top; ++i) {
    if (stack_element_clone(&dest->items[i], &src->items[i]) != 0) {
      // Roll back the partial copy so dest stays consistent.
      for (int j = 0; j < i; ++j)
        stack_element_free(&dest->items[j]);
      dest->top = -1;
      fprintf(stderr, "Error: failed to copy stack element %d.\n", i);
      return 0;
    }
  }

  dest->top = src->top;      // commit only on full success
  return 1; // success
}

void stack_nip(Stack* stack) {
  if (stack->top < 1) {
    fprintf(stderr, "nip: stack underflow\n");
    return;
  }
  // Free the element being discarded, then move the top down into its slot.
  // This is a move, not a copy: no aliasing, no double free.
  stack_element_free(&stack->items[stack->top - 1]);
  stack->items[stack->top - 1] = stack->items[stack->top];
  stack->top--;
}

void stack_tuck(Stack* stack) {
  if (stack->top < 1 || stack->top + 1 >= STACK_SIZE) {
    fprintf(stderr, "tuck: stack underflow or overflow\n");
    return;
  }

  stack_over(stack);
  swap(stack);
}

void stack_over(Stack* stack) {
  if (stack->top < 1 || stack->top + 1 >= STACK_SIZE) {
    fprintf(stderr, "over: stack underflow or overflow\n");
    return;
  }
  // Deep copy: a shallow struct copy would alias the matrix/string pointer
  // into two slots and double-free at cleanup.
  if (stack_element_clone(&stack->items[stack->top + 1],
		    &stack->items[stack->top - 1]) != 0) {
    fprintf(stderr, "over: failed to copy element\n");
    return;
  }
  stack->top++;
}

void stack_roll(Stack* stack, int n) {
  if (n < 0 || stack->top - n < 0) {
    fprintf(stderr, "roll: invalid depth %d\n", n);
    return;
  }
  stack_element temp = stack->items[stack->top - n];
  for (int i = stack->top - n; i < stack->top; i++) {
    stack->items[i] = stack->items[i + 1];
  }
  stack->items[stack->top] = temp;
}
