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

#ifndef GLOBALS_H
#define GLOBALS_H

#include <stdbool.h>
#include <gsl/gsl_rng.h>
#include "stack.h"

#define MAX_PATH 2048

#define HISTORY_FILE ".rpn_history"
#define WORDS_FILE  "user_words.txt"
#define REGISTERS_FILE "registers.txt"
#define CONFIG_FILE "config.txt"
#define MACROS_FILE "predefined_macros.txt"
#define STACK_FILE "stack_backup.dat"

/* Per-user file paths, resolved at run time from $XDG_CONFIG_HOME or $HOME
   (previously baked in at compile time, which broke installs for other
   users and the test sandbox). The directory is created on first use.
   The returned string lives in a small ring of static buffers. */
const char *app_path(const char *file);

#define CONFIG_PATH      app_path(CONFIG_FILE)
#define WORDS_PATH       app_path(WORDS_FILE)
#define REGISTERS_PATH   app_path(REGISTERS_FILE)
#define HISTORY_PATH     app_path(HISTORY_FILE)
#define MACROS_PATH      app_path(MACROS_FILE)
#define STACK_PATH       app_path(STACK_FILE)

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
int load_config(const char* filename);

#endif // GLOBALS_H

