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
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "string_fun.h"
#include "stack.h"

void concatenate(Stack* stack) {
  if (stack->top < 1
      || stack->items[stack->top - 1].type != TYPE_STRING
      || stack->items[stack->top].type != TYPE_STRING) {
    fprintf(stderr,"Both top items must be strings\n");
    return;
  }

  const char* str1 = stack->items[stack->top - 1].string;
  const char* str2 = stack->items[stack->top].string;

  size_t new_len = strlen(str1) + strlen(str2) + 1;
  char* result = malloc(new_len);
  if (!result) {
    fprintf(stderr,"Memory allocation failed\n");
    return;
  }

  strcpy(result, str1);
  strcat(result, str2);

  free(stack->items[stack->top].string);
  free(stack->items[stack->top - 1].string);
  stack->top -= 2;
  push_string(stack, result);
  free(result);
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

