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
#include <ctype.h>
#include <stdint.h>
#include <math.h>
#include <limits.h>
#include "string_fun.h"
#include "stack.h"

void concatenate(Stack *stack)
{
    if (!stack) {
        fprintf(stderr, "concatenate: null stack\n");
        return;
    }

    if (stack->top < 1 ||
        stack->items[stack->top - 1].type != TYPE_STRING ||
        stack->items[stack->top].type     != TYPE_STRING) {
        fprintf(stderr, "Both top items must be strings\n");
        return;
    }

    char *s1 = stack->items[stack->top - 1].string;
    char *s2 = stack->items[stack->top].string;
    if (!s1) s1 = (char *)"";
    if (!s2) s2 = (char *)"";

    size_t n1 = strlen(s1);
    size_t n2 = strlen(s2);

    /* overflow guard (paranoid, but cheap) */
    if (n1 > SIZE_MAX - n2 - 1) {
        fprintf(stderr, "String length overflow\n");
        return;
    }

    char *out = malloc(n1 + n2 + 1);
    if (!out) {
        fprintf(stderr, "Memory allocation failed\n");
        return;
    }

    memcpy(out,      s1, n1);
    memcpy(out + n1, s2, n2);
    out[n1 + n2] = '\0';

    /* Free old strings and replace the lower one with the concatenation */
    free(stack->items[stack->top].string);
    free(stack->items[stack->top - 1].string);

    stack->items[stack->top - 1].type = TYPE_STRING;
    stack->items[stack->top - 1].string = out;

    /* Pop one item (we consumed two and left one) */
    stack->top -= 1;
}

void to_upper(Stack* stack) {
  if (stack->top < 0 || stack->items[stack->top].type != TYPE_STRING) {
    fprintf(stderr,"Top item must be a string\n");
    return;
  }
  char* orig = stack->items[stack->top].string;
  char* upper = strdup(orig);
  if (!upper) {
    fprintf(stderr,"Memory allocation failed\n");
    return;
  }
  for (char* p = upper; *p; ++p) *p = toupper((unsigned char)*p);
  free(orig);
  stack->items[stack->top].string = upper;
}

void to_lower(Stack* stack) {
  if (stack->top < 0 || stack->items[stack->top].type != TYPE_STRING) {
    fprintf(stderr,"Top item must be a string\n");
    return;
  }
  char* orig = stack->items[stack->top].string;
  char* lower = strdup(orig);
  if (!lower) {
    fprintf(stderr,"Memory allocation failed\n");
    return;
  }
  for (char* p = lower; *p; ++p) *p = tolower((unsigned char)*p);
  free(orig);
  stack->items[stack->top].string = lower;
}

void string_length(Stack* stack) {
  if (stack->top < 0 || stack->items[stack->top].type != TYPE_STRING) {
    fprintf(stderr,"Top item must be a string\n");
    return;
  }
  size_t len = strlen(stack->items[stack->top].string);
  free(stack->items[stack->top].string);
  stack->top--;
  push_real(stack, (double)len);
}

void string_reverse(Stack* stack) {
  if (stack->top < 0 || stack->items[stack->top].type != TYPE_STRING) {
    fprintf(stderr,"Top item must be a string\n");
    return;
  }
  char* str = stack->items[stack->top].string;
  size_t len = strlen(str);
  for (size_t i = 0; i < len / 2; ++i) {
    char tmp = str[i];
    str[i] = str[len - i - 1];
    str[len - i - 1] = tmp;
  }
}

void top_to_string(Stack* stack) {
  if (stack->top < 0) {
    fprintf(stderr, "Error: stack is empty\n");
    return;
  }

  stack_element* el = &stack->items[stack->top];

  if (el->type != TYPE_REAL) {
    fprintf(stderr, "Error: top element is not a real number\n");
    return;
  }

  char buf[32];
  long int_part = (long)el->real;  // truncate toward zero
  snprintf(buf, sizeof(buf), "%ld", int_part);

  push_string(stack, buf);
}

/* Convert a stack element to a non-negative int index (integral) */
static int elem_to_index(const stack_element *e, int *out) {
  if (e->type == TYPE_REAL) {
    double v = e->real;
    if (!isfinite(v) || floor(v) != v || v < 0.0 ||
        v > (double)INT_MAX) return 0;
    *out = (int)v;
    return 1;
  }
  return 0;
}

/* Pops: end_index, start_index, string. Pushes: substring string. */
int my_substring(Stack* stack) {
  if (stack->top < 2) {
    fprintf(stderr, "Error: substring requires end, start, and a string on the stack\n");
    return 1;
  }

  /* Pop in this order: end, start, then string */
  stack_element e_str  = stack->items[stack->top--];
  stack_element e_end  = stack->items[stack->top--];
  stack_element e_start= stack->items[stack->top--];

  /* Validate string type */
  if (e_str.type != TYPE_STRING) {
    fprintf(stderr, "Error: substring requires a string as the first popped value\n");
    return 1;
  }

  /* Convert indices */
  int start, end;
  if (!elem_to_index(&e_start, &start) || !elem_to_index(&e_end, &end)) {
    fprintf(stderr, "Error: substring indices must be non-negative integers\n");
    free(e_str.string); /* clean up since we popped it */
    return 1;
  }

  const char* s = e_str.string;
  size_t len = strlen(s);

  /* Bounds: 0 <= start <= end <= len */
  if ((size_t)start > len || (size_t)end > len || start > end) {
    fprintf(stderr, "Error: substring range [%d, %d) is out of bounds for length %zu\n",
            start, end, len);
    free(e_str.string);
    return 1;
  }

  size_t sublen = (size_t)(end - start);

  char* out = (char*)malloc(sublen + 1);
  if (!out) {
    fprintf(stderr, "Error: memory allocation failed\n");
    free(e_str.string);
    return 1;
  }

  if (sublen) memcpy(out, s + start, sublen);
  out[sublen] = '\0';

  /* We’re done with the original string */
  free(e_str.string);

  /* Push result. Assumes push_string copies the buffer. */
  push_string(stack, out);
  free(out);

  return 0;
}
