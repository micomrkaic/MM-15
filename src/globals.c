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

#include "globals.h"

bool fixed_point = true;
bool verbose_mode = false;
bool completed_batch=false;
bool test_flag = false;
bool skip_stack_printing = false;
int print_precision = 6;
int selected_function = 0;
char path_to_data_and_programs[MAX_PATH];
double intg_tolerance = 1.0e-5;
double fsolve_tolerance = 1.0e-6;

#include <stdio.h>
#include <complex.h>
#include <stdbool.h>
#include <string.h>
#include "globals.h"

//#pragma message ("HOME_DIR=" HOME_DIR)

int set_print_precision(Stack* stack) {
  if (stack->top < 0) {
    fprintf(stderr, "Error: Stack underflow. Expected number for precision\n");
    return 1;
  }

  stack_element elem = stack->items[stack->top--];
  int precision;
  if (elem.type == TYPE_REAL) {
    precision = (int)elem.real;
  } else if (elem.type == TYPE_COMPLEX) {
    if (GSL_IMAG(elem.complex_val) != 0.0) {
      fprintf(stderr, "Error: Complex value must be real to set precision\n");
      return 1;
    }
    precision = (int)GSL_REAL(elem.complex_val);
  } else {
    fprintf(stderr, "Error: Expected number on stack to set print precision\n");
    return 1;
  }
  if (precision < 0 || precision > 20) {
    fprintf(stderr, "Error: Precision value %d is out of valid range (0–20)\n", precision);
    return 1;
  }
  print_precision = precision;
  return 0;
}

void swap_fixed_scientific(void) {
  fixed_point = !fixed_point;
}

void save_config(const char* filename) {
  FILE* f = fopen(filename, "w");
  if (!f) {
    perror("Failed to open config file for writing");
    return;
  }

  fprintf(f, "print_precision = %d\n", print_precision);
  fprintf(f, "fixed_point = %d\n", fixed_point);
  fprintf(f, "verbose_mode = %d\n", verbose_mode);
  fprintf(f, "selected_function = %d\n", selected_function);

  fclose(f);
}

int load_config(const char* filename) {
  //  printf("Config file: %s\n", filename);
  FILE* f = fopen(filename, "r");
  if (!f) {
    perror("Failed to open config file for reading");
    return 1;
  }

  char line[256];
  while (fgets(line, sizeof(line), f)) {
    // Remove trailing newline
    line[strcspn(line, "\r\n")] = '\0';

    char* equal_sign = strchr(line, '=');
    if (!equal_sign) continue;

    *equal_sign = '\0';
    char* key = line;
    char* value = equal_sign + 1;

    // Trim leading spaces from value
    while (*value == ' ') value++;

    if (strcmp(key, "print_precision") == 0) {
      print_precision = atoi(value);
    } else if (strcmp(key, "fixed_point") == 0) {
      fixed_point = atoi(value);
    } else if (strcmp(key, "verbose_mode") == 0) {
      verbose_mode = atoi(value);
    } else if (strcmp(key, "selected_function") == 0) {
      selected_function = atoi(value);
    } else if (strcmp(key, "path_to_data_and_programs") == 0) {
      strncpy(path_to_data_and_programs, value, MAX_PATH - 1);
      path_to_data_and_programs[MAX_PATH - 1] = '\0';
    }
  }

  fclose(f);
  return 0;
}
