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

/* As of December 29, 2025
   . TODO add examples of use into README.md
   . TODO improve documentation
*/

/* TODO for later
   . call from emacs and shell as a calculator/evaluator
   . implement in JSON save_stack_to_file in the interpreter and load_stack_from_file
   . Simple plotting of a vector vs vector for a simple function plot etc.
   . select submatrices; resize matrices and add/remove rows and/or columns
   . clean up and consolidate binary_fun.c; cleanup the dispatch table
   . clean up the interpreter to have only one dispatch table in the VM
   . Overlay for registers -- store variables; recall values with <-
   . load program, list program, run program -> separate instructions
   . implement loop counters and easier comparison registers for iterations;
   . fully implement counters and tests
   . sto ind, rcl ind.
   . Test full HP-41 style programming with GTO, RTN, XEQ, ISG, DSE, LBL etc. and labels
   . Automatic cleanup of matrices and strings with __cleanup__    
*/

#define _POSIX_C_SOURCE 200809L

#include <gsl/gsl_errno.h>      // for gsl_set_error_handler
#include <gsl/gsl_rng.h>        // for gsl_rng_alloc, gsl_rng, gsl_rng_mt19937
#include <readline/history.h>   // for add_history, read_history, stifle_his...
#include <readline/readline.h>  // for readline, rl_attempted_completion_fun...
#include <stdbool.h>            // for false
#include <stdio.h>              // for fprintf, perror, stderr, NULL
#include <stdlib.h>             // for free, WEXITSTATUS, system, WIFEXITED
#include <string.h>             // for strcmp
#include <ctype.h>              // for isspace
#include <errno.h>              // for errno
#include "eval_fun.h"           // for evaluate_line
#include "globals.h"            // for CONFIG_PATH, HISTORY_PATH, completed_...
#include "print_fun.h"          // for print_stack
#include "registers.h"          // for free_all_registers, init_registers
#include "splash.h"             // for splash_screen
#include "stack.h"              // for copy_stack, free_stack, init_stack
#include "tab_completion.h"     // for function_name_completion
#include "words.h"              // for list_macros, load_macros_from_file

// Globals
gsl_rng * global_rng; // Global random number generator, used throughout the program
Register registers[MAX_REG] ={0};  // Global or static array

// Local declarations
void my_error_handler(const char *reason, const char *file, int line, int gsl_errno);
int repl(void);

void my_error_handler(const char *reason, const char *file, int line, int gsl_errno) {
  fprintf(stderr, "GSL ERROR: %s (%s:%d) [code=%d]\n", reason, file, line, gsl_errno);
  // Do NOT call abort(); this allows graceful recovery
}

/* Read all of cmd's stdout into a malloc'd buffer.
   Returns NULL on failure. Caller owns the returned string. */
static char *capture_command_output(const char *cmd, int *wait_status_out) {
  if (wait_status_out) *wait_status_out = 0;

  errno = 0;
  FILE *fp = popen(cmd, "r");          // runs via /bin/sh -c cmd (POSIX)
  if (!fp) return NULL;

  size_t cap = 4096;
  size_t len = 0;
  char *buf = malloc(cap);
  if (!buf) { pclose(fp); return NULL; }

  for (;;) {
    if (len + 2048 + 1 > cap) {
      cap *= 2;
      char *tmp = realloc(buf, cap);
      if (!tmp) { free(buf); pclose(fp); return NULL; }
      buf = tmp;
    }
    size_t n = fread(buf + len, 1, 2048, fp);
    len += n;
    if (n == 0) {
      if (ferror(fp)) { free(buf); pclose(fp); return NULL; }
      break; // EOF
    }
  }

  buf[len] = '\0';
  int st = pclose(fp);
  if (wait_status_out) *wait_status_out = st;

  // Trim trailing whitespace/newlines (optional but usually nicer)
  while (len > 0 && isspace((unsigned char)buf[len - 1])) {
    buf[--len] = '\0';
  }
  return buf;
}


int repl(void) {

  Stack stack;
  Stack old_stack;

  // Initialize everything needed
  splash_screen();
  init_stack(&stack);
  init_stack(&old_stack);
  init_registers();
  load_macros_from_file();
  if (verbose_mode) list_macros();
  load_config(CONFIG_PATH); // #define APP_CFG_DIR  HOME_DIR "/.config/mm_15"
  read_history(HISTORY_PATH);
  stifle_history(1000);  // Keep only the last 1000 commands 
  rl_attempted_completion_function = function_name_completion;

  // The main REPL loop
  while (1) {
    char* line = readline("MM_RPN>> ");
    if (!line || strcmp(line, "q") == 0) {
      free(line);
      break;
    }
    if (*line) add_history(line);

    if (line[0] == '!') {
      const char *cmd = line + 1;

      // If you want stderr too, do: "<cmd> 2>&1"
      // Easiest is to build a wrapper string:
      // char wrapped[...]; snprintf(wrapped, sizeof wrapped, "%s 2>&1", cmd); cmd = wrapped;

      int st = 0;
      char *out = capture_command_output(cmd, &st);
      if (!out) {
	perror("popen/capture_command_output");
      } else {
	push_string(&stack, out);
	if (WIFEXITED(st) && WEXITSTATUS(st) != 0) {
	  fprintf(stderr, "Command exited with status %d\n", WEXITSTATUS(st));
	} else if (!WIFEXITED(st)) {
	  fprintf(stderr, "Command terminated abnormally\n");
	}
      }

      free(line);
      continue;
    }
   
    if (!strcmp(line, "undo")) // Restore the old stack
      { copy_stack(&stack, &old_stack);
      } else {
      copy_stack(&old_stack, &stack);  // Preserve the stack
      evaluate_line(&stack, line);
    }
    if (completed_batch)
      completed_batch = false;
    else
      if (!skip_stack_printing) print_stack(&stack,NULL);
    skip_stack_printing = false;
    free(line);
  }

  // Save config, history, and cleanup
  save_config(CONFIG_PATH);
  write_history(HISTORY_PATH);
  free_stack(&old_stack);
  free_stack(&stack);
  free_all_registers();
  return 0;
}

int main(void) {
  global_rng = gsl_rng_alloc(gsl_rng_mt19937); // Init random number generation
  gsl_set_error_handler(&my_error_handler);
  repl();
  return 0;
}
