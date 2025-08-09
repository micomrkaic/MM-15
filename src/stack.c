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


// A heterogenous stack with operations for an RPN calculator
// Mico Mrkaic, mrkaic.mico@gmail.com

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <string.h>
#include <complex.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_complex_math.h>
#include "stack.h"

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
  if ((stack->items[stack->top].type == TYPE_REAL)
      || (stack->items[stack->top].type == TYPE_COMPLEX)) {
    stack->items[stack->top + 1] = stack->items[stack->top];
    stack->top++;
  }
  if (stack->items[stack->top].type == TYPE_MATRIX_REAL) {
    stack->items[stack->top + 1].type = TYPE_MATRIX_REAL;  
    size_t rows = stack->items[stack->top].matrix_real->size1;
    size_t cols = stack->items[stack->top].matrix_real->size2;
    stack->items[stack->top + 1].matrix_real = gsl_matrix_alloc(rows, cols);
    gsl_matrix_memcpy(
		      stack->items[stack->top + 1].matrix_real,
		      stack->items[stack->top].matrix_real);
    stack->top++;
  }
  if (stack->items[stack->top].type == TYPE_MATRIX_COMPLEX) {
    stack->items[stack->top + 1].type = TYPE_MATRIX_COMPLEX;
    size_t rows = stack->items[stack->top].matrix_complex->size1;
    size_t cols = stack->items[stack->top].matrix_complex->size2;
    stack->items[stack->top + 1].matrix_complex =
      gsl_matrix_complex_alloc(rows, cols);
    gsl_matrix_complex_memcpy(stack->items[stack->top + 1].matrix_complex,
			      stack->items[stack->top].matrix_complex);
    stack->top++;
  }
  if (stack->items[stack->top].type == TYPE_STRING) {
    stack->items[stack->top + 1].type = TYPE_STRING;
    stack->items[stack->top + 1].string =
      strdup(stack->items[stack->top].string);
    stack->top++;
  }
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

stack_element* view_top(Stack* stack) {
  if (stack->top < 0) {
    fprintf(stderr,"Stack is empty\n");
    return NULL;
  }
  return &stack->items[stack->top];
}

void free_stack(Stack* stack) {
  while (stack->top >= 0) {
    switch (stack->items[stack->top].type) {
    case TYPE_STRING:
      free(stack->items[stack->top].string);
      break;
    case TYPE_MATRIX_REAL:
      gsl_matrix_free(stack->items[stack->top].matrix_real);
      break;
    case TYPE_MATRIX_COMPLEX:
      gsl_matrix_complex_free(stack->items[stack->top].matrix_complex);
      break;
    default:
      break;
    }
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
  if (stack->top < 0) {
    fprintf(stderr, "Stack is empty\n");
    return -1;  // Or some sentinel value you define
  }
  return stack->items[stack->top-1].type;
}

int save_stack_to_file(Stack* stack, const char* filename) {
  FILE* file = fopen(filename, "wb");
  if (!file) {
    perror("fopen");
    return -1;
  }

  // Save the number of elements
  if (fwrite(&stack->top, sizeof(int), 1, file) != 1) {
    perror("fwrite top");
    fclose(file);
    return -1;
  }

  for (int i = 0; i <= stack->top; ++i) {
    stack_element* elem = &stack->items[i];

    // Save the type first
    if (fwrite(&elem->type, sizeof(value_type), 1, file) != 1) {
      perror("fwrite type");
      fclose(file);
      return -1;
    }

    switch (elem->type) {
    case TYPE_REAL:
      if (fwrite(&elem->real, sizeof(double), 1, file) != 1) {
	perror("fwrite real");
	fclose(file);
	return -1;
      }
      break;

    case TYPE_COMPLEX:
      if (fwrite(&elem->complex_val, sizeof(gsl_complex), 1, file) != 1) {
	perror("fwrite complex");
	fclose(file);
	return -1;
      }
      break;

    case TYPE_STRING: {
      size_t len = strlen(elem->string) + 1; // Include null terminator
      if (fwrite(&len, sizeof(size_t), 1, file) != 1 ||
	  fwrite(elem->string, sizeof(char), len, file) != len) {
	perror("fwrite string");
	fclose(file);
	return -1;
      }
      break;
    }

    case TYPE_MATRIX_REAL: {
      size_t rows = elem->matrix_real->size1;
      size_t cols = elem->matrix_real->size2;
      if (fwrite(&rows, sizeof(size_t), 1, file) != 1 ||
	  fwrite(&cols, sizeof(size_t), 1, file) != 1 ||
	  fwrite(elem->matrix_real->data, sizeof(double), rows * cols, file) != rows * cols) {
	perror("fwrite matrix_real");
	fclose(file);
	return -1;
      }
      break;
    }

    case TYPE_MATRIX_COMPLEX: {
      size_t rows = elem->matrix_complex->size1;
      size_t cols = elem->matrix_complex->size2;
      if (fwrite(&rows, sizeof(size_t), 1, file) != 1 ||
	  fwrite(&cols, sizeof(size_t), 1, file) != 1 ||
	  fwrite(elem->matrix_complex->data, sizeof(double), 2*rows*cols, file) != 2*rows*cols)
	{
	  perror("fwrite matrix_complex");
	  fclose(file);
	  return -1;
	}
      break;
    }
      
    default:
      fprintf(stderr, "Unknown type: %d\n", elem->type);
      fclose(file);
      return -1;
    }
  }

  fclose(file);
  return 0;
}

int load_stack_from_file(Stack* stack, const char* filename) {
  FILE* file = fopen(filename, "rb");
  if (!file) {
    perror("fopen");
    return -1;
  }

  // Read the number of elements (top index)
  if (fread(&stack->top, sizeof(int), 1, file) != 1) {
    perror("fread top");
    fclose(file);
    return -1;
  }

  for (int i = 0; i <= stack->top; ++i) {
    stack_element* elem = &stack->items[i];

    // Read type
    if (fread(&elem->type, sizeof(value_type), 1, file) != 1) {
      perror("fread type");
      fclose(file);
      return -1;
    }

    switch (elem->type) {
    case TYPE_REAL:
      if (fread(&elem->real, sizeof(double), 1, file) != 1) {
	perror("fread real");
	fclose(file);
	return -1;
      }
      break;

    case TYPE_COMPLEX:
      if (fread(&elem->complex_val, sizeof(gsl_complex), 1, file) != 1) {
	perror("fread complex");
	fclose(file);
	return -1;
      }
      break;

    case TYPE_STRING: {
      size_t len;
      if (fread(&len, sizeof(size_t), 1, file) != 1) {
	perror("fread string length");
	fclose(file);
	return -1;
      }

      elem->string = malloc(len);
      if (!elem->string) {
	perror("malloc string");
	fclose(file);
	return -1;
      }

      if (fread(elem->string, sizeof(char), len, file) != len) {
	perror("fread string");
	free(elem->string);
	fclose(file);
	return -1;
      }
      break;
    }

    case TYPE_MATRIX_REAL: {
      size_t rows, cols;
      if (fread(&rows, sizeof(size_t), 1, file) != 1 ||
	  fread(&cols, sizeof(size_t), 1, file) != 1) {
	perror("fread matrix_real size");
	fclose(file);
	return -1;
      }

      elem->matrix_real = gsl_matrix_alloc(rows, cols);
      if (!elem->matrix_real) {
	fprintf(stderr, "Failed to allocate matrix_real\n");
	fclose(file);
	return -1;
      }

      if (fread(elem->matrix_real->data, sizeof(double), rows * cols, file) != rows * cols) {
	perror("fread matrix_real data");
	gsl_matrix_free(elem->matrix_real);
	fclose(file);
	return -1;
      }
      break;
    }

    case TYPE_MATRIX_COMPLEX: {
      size_t rows, cols;
      if (fread(&rows, sizeof(size_t), 1, file) != 1 ||
	  fread(&cols, sizeof(size_t), 1, file) != 1) {
	perror("fread matrix_complex size");
	fclose(file);
	return -1;
      }

      elem->matrix_complex = gsl_matrix_complex_alloc(rows, cols);
      if (!elem->matrix_complex) {
	fprintf(stderr, "Failed to allocate matrix_complex\n");
	fclose(file);
	return -1;
      }

      if (fread(elem->matrix_complex->data, sizeof(double), 2*rows*cols, file) != 2*rows*cols)
	{
	  perror("fread matrix_complex data");
	  gsl_matrix_complex_free(elem->matrix_complex);
	  fclose(file);
	  return -1;
	}
      break;
    }
      
    default:
      fprintf(stderr, "Unknown type: %d\n", elem->type);
      fclose(file);
      return -1;
    }
  }

  fclose(file);
  return 0;
}

int copy_stack(Stack* dest, const Stack* src) {
  dest->top = src->top;

  for (int i = 0; i <= src->top; ++i) {
    stack_element src_elem = src->items[i];
    stack_element* dest_elem = &dest->items[i];
    dest_elem->type = src_elem.type;

    switch (src_elem.type) {
    case TYPE_REAL:
      dest_elem->real = src_elem.real;
      break;

    case TYPE_COMPLEX:
      dest_elem->complex_val = src_elem.complex_val;
      break;

    case TYPE_STRING:
      dest_elem->string = strdup(src_elem.string);
      if (!dest_elem->string) {
	fprintf(stderr, "Error: failed to allocate string in stack copy.\n");
	return 0;
      }
      break;

    case TYPE_MATRIX_REAL:
      if (!src_elem.matrix_real) {
	dest_elem->matrix_real = NULL;
	break;
      }
      dest_elem->matrix_real = gsl_matrix_alloc(src_elem.matrix_real->size1,
						src_elem.matrix_real->size2);
      if (!dest_elem->matrix_real) {
	fprintf(stderr, "Error: failed to allocate real matrix.\n");
	return 0;
      }
      gsl_matrix_memcpy(dest_elem->matrix_real, src_elem.matrix_real);
      break;

    case TYPE_MATRIX_COMPLEX:
      if (!src_elem.matrix_complex) {
	dest_elem->matrix_complex = NULL;
	break;
      }
      dest_elem->matrix_complex = gsl_matrix_complex_alloc(src_elem.matrix_complex->size1,
							   src_elem.matrix_complex->size2);
      if (!dest_elem->matrix_complex) {
	fprintf(stderr, "Error: failed to allocate complex matrix.\n");
	return 0;
      }
      gsl_matrix_complex_memcpy(dest_elem->matrix_complex, src_elem.matrix_complex);
      break;

    default:
      fprintf(stderr, "Error: unknown type in stack copy.\n");
      return 0;
    }
  }

  return 1; // success
}

void stack_nip(Stack* stack) {
  if (stack->top < 1) {
    fprintf(stderr, "nip: stack underflow\n");
    return;
  }
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
  stack_element copy = stack->items[stack->top - 1];
  stack->items[++stack->top] = copy;
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
