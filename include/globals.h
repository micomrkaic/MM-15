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

#ifndef GLOBALS_H
#define GLOBALS_H

#include <stdbool.h>
#include <gsl/gsl_rng.h>
#include "stack.h"

#define MAX_PATH 2048

extern gsl_rng * global_rng;

extern bool fixed_point;
extern bool verbose_mode;
extern bool completed_batch;
extern bool test_flag;
extern bool skip_stack_printing;
extern int print_precision;
extern int selected_function;
extern char path_to_data_and_programs[MAX_PATH];

extern double intg_tolerance;
extern double fsolve_tolerance;

int set_print_precision(Stack* stack);
void swap_fixed_scientific(void);
void save_config(const char* filename);
void load_config(const char* filename);

#endif // GLOBALS_H

