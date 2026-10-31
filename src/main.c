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
 * along with Mico's MM-15 Calculator. If not, see
 * <https://www.gnu.org/licenses/>.
 */

/* TODO as of January 18, 2026
   . use pop() consistently instead of accessing stack internally through the pointer
   . make sure strings do not leak after date operations
   . fix the date format in my_date.c to throw error on e.g., "33.14.2026"
   . physical constants
   . call from emacs and shell as a calculator/evaluator
   . implement in JSON save_stack_to_file in the interpreter and
   load_stack_from_file . Simple plotting of a vector vs vector for a simple
   function plot etc. . select submatrices; resize matrices and add/remove rows
   and/or columns . clean up and consolidate binary_fun.c; cleanup the dispatch
   table . clean up the interpreter to have only one dispatch table in the VM .
   Overlay for registers -- store variables; recall values with <- . load
   program, list program, run program -> separate instructions . implement loop
   counters and easier comparison registers for iterations; . fully implement
   counters and tests . sto ind, rcl ind. . Test full HP-41 style programming
   with GTO, RTN, XEQ, ISG, DSE, LBL etc. and labels . Automatic cleanup of
   matrices and strings with __cleanup__
*/

#define _POSIX_C_SOURCE 200809L

#include "errors.h"
#include "eval_fun.h"          // for evaluate_line
#include "ops.h"               // for print_ops_markdown
#include "globals.h"           // for CONFIG_PATH, HISTORY_PATH, completed_...
#include "print_fun.h"         // for print_stack
#include "registers.h"         // for free_all_registers, init_registers
#include "splash.h"            // for splash_screen
#include "stack.h"             // for copy_stack, free_stack, init_stack
#include "tab_completion.h"    // for function_name_completion
#include "words.h"             // for list_macros, load_macros_from_file
#include <ctype.h>             // for isspace
#include <errno.h>             // for errno
#include <gsl/gsl_errno.h>     // for gsl_set_error_handler
#include <gsl/gsl_rng.h>       // for gsl_rng_alloc, gsl_rng, gsl_rng_mt19937
#include <readline/history.h>  // for add_history, read_history, stifle_his...
#include <readline/readline.h> // for readline, rl_attempted_completion_fun...
#include <stdbool.h>           // for false
#include <stdio.h>             // for fprintf, perror, stderr, NULL
#include <stdlib.h>            // for free, WEXITSTATUS, system, WIFEXITED
#include <string.h>            // for strcmp
#include <unistd.h>            // for isatty

// Globals
gsl_rng
    *global_rng; // Global random number generator, used throughout the program
Register registers[MAX_REG] = {0}; // Global or static array

#define MM15_VERSION "2.0"

// Local declarations
void my_error_handler(const char *reason, const char *file, int line,
                      int gsl_errno);
int repl(void);

void my_error_handler(const char *reason, const char *file, int line,
                      int gsl_errno) {
  mm_error("GSL ERROR: %s (%s:%d) [code=%d]\n", reason, file, line,
          gsl_errno);
  // Do NOT call abort(); this allows graceful recovery
}

/* Read all of cmd's stdout into a malloc'd buffer.
   Returns NULL on failure. Caller owns the returned string. */
static char *capture_command_output(const char *cmd, int *wait_status_out) {
  if (wait_status_out)
    *wait_status_out = 0;

  errno = 0;
  FILE *fp = popen(cmd, "r"); // runs via /bin/sh -c cmd (POSIX)
  if (!fp)
    return NULL;

  size_t cap = 4096;
  size_t len = 0;
  char *buf = malloc(cap);
  if (!buf) {
    pclose(fp);
    return NULL;
  }

  for (;;) {
    if (len + 2048 + 1 > cap) {
      cap *= 2;
      char *tmp = realloc(buf, cap);
      if (!tmp) {
        free(buf);
        pclose(fp);
        return NULL;
      }
      buf = tmp;
    }
    size_t n = fread(buf + len, 1, 2048, fp);
    len += n;
    if (n == 0) {
      if (ferror(fp)) {
        free(buf);
        pclose(fp);
        return NULL;
      }
      break; // EOF
    }
  }

  buf[len] = '\0';
  int st = pclose(fp);
  if (wait_status_out)
    *wait_status_out = st;

  // Trim trailing whitespace/newlines (optional but usually nicer)
  while (len > 0 && isspace((unsigned char)buf[len - 1])) {
    buf[--len] = '\0';
  }
  return buf;
}

static bool batch_mode = false;   /* -e, -f, or stdin that is not a terminal */
static bool autoload = true;      /* -n turns off macros/config at startup */

static void startup(Stack *stack, Stack *old_stack) {
  init_stack(stack);
  init_stack(old_stack);
  init_registers();
  if (autoload) {
    load_macros_from_file();
    load_config(CONFIG_PATH);
  }
}

static void shutdown(Stack *stack, Stack *old_stack) {
  if (autoload && !batch_mode) save_config(CONFIG_PATH);
  free_stack(old_stack);
  free_stack(stack);
  free_all_registers();
}

/* One line of input, shared by the REPL and batch mode. Returns false on "q". */
static bool handle_line(Stack *stack, Stack *old_stack, char *line) {
  if (strcmp(line, "q") == 0) return false;

  if (line[0] == '!') {
    int st = 0;
    char *out = capture_command_output(line + 1, &st);
    if (!out) {
      mm_perror("popen/capture_command_output");
    } else {
      push_string(stack, out);
      if (WIFEXITED(st) && WEXITSTATUS(st) != 0)
        mm_error("Command exited with status %d", WEXITSTATUS(st));
      else if (!WIFEXITED(st))
        mm_error("Command terminated abnormally");
      free(out);
    }
    return true;
  }

  if (!strcmp(line, "undo")) {
    copy_stack(stack, old_stack);
  } else {
    copy_stack(old_stack, stack); // Preserve the stack
    evaluate_line(stack, line);
  }
  return true;
}

int repl(void) {
  Stack stack, old_stack;
  splash_screen();
  startup(&stack, &old_stack);
  if (verbose_mode) list_macros();
  read_history(HISTORY_PATH);
  stifle_history(1000); // Keep only the last 1000 commands
  rl_attempted_completion_function = function_name_completion;

  while (1) {
    char *line = readline("MM_RPN>> ");
    if (!line) { putchar('\n'); break; }
    if (*line) add_history(line);
    bool go_on = handle_line(&stack, &old_stack, line);
    free(line);
    if (!go_on) break;
    if (completed_batch)
      completed_batch = false;
    else if (!skip_stack_printing)
      print_stack(&stack, NULL);
    skip_stack_printing = false;
  }

  write_history(HISTORY_PATH);
  shutdown(&stack, &old_stack);
  return 0;
}

/* Batch mode: no splash, no echo of the stack after each line. The lines
   from -e/-f/stdin run in order; the final stack is printed once at the
   end, and the exit status is 1 if any error was reported. */
static int run_lines_from(Stack *stack, Stack *old_stack, FILE *f) {
  char *line = NULL;
  size_t cap = 0;
  ssize_t n;
  bool go_on = true;
  while (go_on && (n = getline(&line, &cap, f)) != -1) {
    while (n > 0 && (line[n - 1] == '\n' || line[n - 1] == '\r')) line[--n] = '\0';
    go_on = handle_line(stack, old_stack, line);
    skip_stack_printing = false;
    completed_batch = false;
  }
  free(line);
  return go_on;
}

static void usage_text(FILE *f) {
  fprintf(f,
    "usage: mm_15 [-n] [-e EXPR]... [-f FILE]...\n"
    "       mm_15 --commands | --version | -h\n"
    "\n"
    "  (no args)   interactive calculator (reads stdin when it is not a terminal)\n"
    "  -e EXPR     evaluate EXPR as one line; may be repeated\n"
    "  -f FILE     run the lines of FILE ('-' is stdin); may be repeated\n"
    "  -n          do not load the saved macros and config\n"
    "\n"
    "Non-interactive runs print the final stack and exit with status 1 if\n"
    "any error occurred.\n");
}

int main(int argc, char **argv) {
  enum { MAXJOBS = 256 };
  struct { char kind; const char *arg; } jobs[MAXJOBS];
  int njobs = 0;

  for (int i = 1; i < argc; i++) {
    const char *a = argv[i];
    if (!strcmp(a, "-h") || !strcmp(a, "--help")) { usage_text(stdout); return 0; }
    if (!strcmp(a, "--commands")) { print_ops_markdown(); return 0; }
    if (!strcmp(a, "--version")) { printf("MM-15 %s (%s)\n", MM15_VERSION, VERSION); return 0; }
    if (!strcmp(a, "-n")) { autoload = false; continue; }
    if ((!strcmp(a, "-e") || !strcmp(a, "-f")) && i + 1 < argc && njobs < MAXJOBS) {
      jobs[njobs].kind = a[1];
      jobs[njobs].arg = argv[++i];
      njobs++;
      continue;
    }
    usage_text(stderr);
    return 2;
  }

  global_rng = gsl_rng_alloc(gsl_rng_mt19937); // Init random number generation
  gsl_set_error_handler(&my_error_handler);

  batch_mode = (njobs > 0) || !isatty(STDIN_FILENO);
  /* Line-buffer stdout in batch mode so output and error messages interleave
     in order and nothing is lost if a sanitizer aborts the process at exit. */
  if (batch_mode) setvbuf(stdout, NULL, _IOLBF, 0);
  if (!batch_mode) {
    repl();
    gsl_rng_free(global_rng);
    return 0;
  }

  Stack stack, old_stack;
  startup(&stack, &old_stack);
  bool go_on = true;
  if (njobs == 0) {
    go_on = run_lines_from(&stack, &old_stack, stdin);
  } else {
    for (int j = 0; j < njobs && go_on; j++) {
      if (jobs[j].kind == 'e') {
        char *copy = strdup(jobs[j].arg);
        go_on = handle_line(&stack, &old_stack, copy);
        free(copy);
      } else {
        FILE *f = strcmp(jobs[j].arg, "-") ? fopen(jobs[j].arg, "r") : stdin;
        if (!f) { mm_perror(jobs[j].arg); continue; }
        go_on = run_lines_from(&stack, &old_stack, f);
        if (f != stdin) fclose(f);
      }
    }
  }
  print_stack(&stack, NULL);
  shutdown(&stack, &old_stack);
  gsl_rng_free(global_rng);
  return error_count ? 1 : 0;
}
