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

#include <stdbool.h>      // for false, bool, true
#include <stdio.h>        // for fclose, fprintf, NULL, stderr, printf, fopen
#include <stdlib.h>       // for free
#include <string.h>       // for strcmp, strdup, strncmp, strcspn, strchr
#include "eval_fun.h"     // for evaluate_line
#include "globals.h"      // for completed_batch
#include "run_machine.h"  // for Program, INSTR_GOSUB, INSTR_GOTO, INSTR_LABEL
#include "stack.h"        // for (anonymous struct)::(anonymous), TYPE_REAL

#define MAX_COUNTERS 32

int program_counter = 0;
int cond_counters[MAX_COUNTERS] = {0};

// Functions to test the stack and registers
/* "top_eq0?", "top_ge0?",  "top_gt0?", "top_le0?",  "top_lt0?", */
/* "top_eg?", "top_ge?",  "top_gt?", "top_le?",  "top_lt?", */
/* "ctr_eq0?", "ctr_ge0?",  "ctr_gt0?", "ctr_le0?",  "ctr_lt0?", */
/* "goto", "xeq", "rtn", "end", "lbl", */

bool is_top_eq_0(Stack* stack) {
  return (stack->top >= 0 &&
	  stack->items[stack->top].type == TYPE_REAL &&
	  stack->items[stack->top].real == 0.0);
}

bool is_top_neq_0(Stack* stack) {
  return (stack->top >= 0 &&
	  stack->items[stack->top].type == TYPE_REAL &&
	  stack->items[stack->top].real != 0.0);
}

bool is_top_gt_0(Stack* stack) {
  return (stack->top >= 0 &&
	  stack->items[stack->top].type == TYPE_REAL &&
	  stack->items[stack->top].real > 0.0);
}

bool is_top_lt_0(Stack* stack) {
  return (stack->top >= 0 &&
	  stack->items[stack->top].type == TYPE_REAL &&
	  stack->items[stack->top].real < 0.0);
}

bool is_top_gte_0(Stack* stack) {
  return (stack->top >= 0 &&
	  stack->items[stack->top].type == TYPE_REAL &&
	  stack->items[stack->top].real >= 0.0);
}

bool is_top_lte_0(Stack* stack) {
  return (stack->top >= 0 &&
	  stack->items[stack->top].type == TYPE_REAL &&
	  stack->items[stack->top].real <= 0.0);
}

bool is_top_eq(Stack* stack) {
  return (stack->top >= 1 &&
	  stack->items[stack->top].type == TYPE_REAL &&
	  stack->items[stack->top - 1].type == TYPE_REAL &&
	  stack->items[stack->top - 1].real == stack->items[stack->top].real);
}

bool is_top_neq(Stack* stack) {
  return (stack->top >= 1 &&
	  stack->items[stack->top].type == TYPE_REAL &&
	  stack->items[stack->top - 1].type == TYPE_REAL &&
	  stack->items[stack->top - 1].real != stack->items[stack->top].real);
}

bool is_top_gt(Stack* stack) {
  return (stack->top >= 1 &&
	  stack->items[stack->top].type == TYPE_REAL &&
	  stack->items[stack->top - 1].type == TYPE_REAL &&
	  stack->items[stack->top - 1].real > stack->items[stack->top].real);
}

bool is_top_lt(Stack* stack) {
  return (stack->top >= 1 &&
	  stack->items[stack->top].type == TYPE_REAL &&
	  stack->items[stack->top - 1].type == TYPE_REAL &&
	  stack->items[stack->top - 1].real < stack->items[stack->top].real);
}

bool is_top_gte(Stack* stack) {
  return (stack->top >= 1 &&
	  stack->items[stack->top].type == TYPE_REAL &&
	  stack->items[stack->top - 1].type == TYPE_REAL &&
	  stack->items[stack->top - 1].real >= stack->items[stack->top].real);
}

bool is_top_lte(Stack* stack) {
  return (stack->top >= 1 &&
	  stack->items[stack->top].type == TYPE_REAL &&
	  stack->items[stack->top - 1].type == TYPE_REAL &&
	  stack->items[stack->top - 1].real <= stack->items[stack->top].real);
}

// **************** Comparisons with counters ****************
bool is_ctr_eq_0(Stack* stack) {
  if (stack->top < 0 || stack->items[stack->top].type != TYPE_REAL) return false;
  int index = (int)stack->items[stack->top].real;
  if (index < 0 || index >= MAX_COUNTERS) return false;
  return cond_counters[index] == 0.0;
}

bool is_ctr_neq_0(Stack* stack) {
  if (stack->top < 0 || stack->items[stack->top].type != TYPE_REAL) return false;
  int index = (int)stack->items[stack->top].real;
  if (index < 0 || index >= MAX_COUNTERS) return false;
  return cond_counters[index] != 0.0;
}

bool is_ctr_gt_0(Stack* stack) {
  if (stack->top < 0 || stack->items[stack->top].type != TYPE_REAL) return false;
  int index = (int)stack->items[stack->top].real;
  if (index < 0 || index >= MAX_COUNTERS) return false;
  return cond_counters[index] > 0.0;
}

bool is_ctr_lt_0(Stack* stack) {
  if (stack->top < 0 || stack->items[stack->top].type != TYPE_REAL) return false;
  int index = (int)stack->items[stack->top].real;
  if (index < 0 || index >= MAX_COUNTERS) return false;
  return cond_counters[index] < 0.0;
}

bool is_ctr_gte_0(Stack* stack) {
  if (stack->top < 0 || stack->items[stack->top].type != TYPE_REAL) return false;
  int index = (int)stack->items[stack->top].real;
  if (index < 0 || index >= MAX_COUNTERS) return false;
  return cond_counters[index] >= 0.0;
}

bool is_ctr_lte_0(Stack* stack) {
  if (stack->top < 0 || stack->items[stack->top].type != TYPE_REAL) return false;
  int index = (int)stack->items[stack->top].real;
  if (index < 0 || index >= MAX_COUNTERS) return false;
  return cond_counters[index] <= 0.0;
}

bool is_ctr_compare(Stack* stack, const char* op) {
  if (stack->top < 0 || stack->items[stack->top].type != TYPE_REAL) return false;
  int index = (int)stack->items[stack->top].real;
  if (index < 0 || index >= MAX_COUNTERS) return false;

  double value = cond_counters[index];

  if (strcmp(op, "==") == 0) return value == 0.0;
  if (strcmp(op, "!=") == 0) return value != 0.0;
  if (strcmp(op, "<")  == 0) return value <  0.0;
  if (strcmp(op, "<=") == 0) return value <= 0.0;
  if (strcmp(op, ">")  == 0) return value >  0.0;
  if (strcmp(op, ">=") == 0) return value >= 0.0;

  fprintf(stderr, "Unknown operator: %s\n", op);
  return false;
}

// Build the dispatch table
typedef bool (*compare_fn)(Stack* stack);

typedef struct {
    const char* name;
    compare_fn fn;
} compare_dispatch_entry;

compare_dispatch_entry compare_dispatch_table[] = {
    {"top_eq0?",     is_top_eq_0},
    {"top_neq0?",    is_top_neq_0},
    {"top_gt0?",     is_top_gt_0},
    {"top_lt0?",     is_top_lt_0},
    {"top_gte0?",    is_top_gte_0},
    {"top_lte0?",    is_top_lte_0},
    {"top_eq?",      is_top_eq},
    {"top_neq?",     is_top_neq},
    {"top_gt?",      is_top_gt},
    {"top_lt?",      is_top_lt},
    {"top_gte?",     is_top_gte},
    {"top_lte?",     is_top_lte},
    {"ctr_eq0?",  is_ctr_eq_0},
    {"ctr_neq0?", is_ctr_neq_0},
    {"ctr_gt0?",  is_ctr_gt_0},
    {"ctr_lt0?",  is_ctr_lt_0},
    {"ctr_gte0?", is_ctr_gte_0},
    {"ctr_lte0?", is_ctr_lte_0},
    {NULL, NULL}  // Sentinel
};

static compare_fn get_compare_fn(const char* name) {
    for (int i = 0; compare_dispatch_table[i].name != NULL; ++i) {
        if (strcmp(compare_dispatch_table[i].name, name) == 0) {
            return compare_dispatch_table[i].fn;
        }
    }
    return NULL;
}

int run_batch(Stack *stack, char *fname) {
  FILE* f = fopen(fname, "r");
  if (!f) {
    perror("Failed to open input file");
    return 1;
  } else printf("Running batch: %s\n",fname);

  char *line = NULL;
  size_t len = 0;

  while (getline(&line, &len, f) != -1) {
    // Remove trailing newline
    line[strcspn(line, "\r\n")] = '\0';
    evaluate_line(stack, line);
  }
  free(line);
  fclose(f);
  completed_batch = true;
  return 0;
}

static bool evaluate_test_condition(Stack* stack, const char* test_name) {
  bool result;
  compare_fn fn = get_compare_fn(test_name);
  if (fn) {
    result = fn(stack);
  } else {
    fprintf(stderr, "Unknown condition: %s\n", test_name);
    result=false;
  }
  return result;
}

static int find_label(const Program* prog, const char* label) {
  for (int i = 0; i < prog->label_count; ++i) {
    if (strcmp(prog->labels[i].label, label) == 0)
      return prog->labels[i].pc;
  }
  return -1;
}

void list_program(const Program* prog) {
  printf("--- Program Listing ---\n");
  for (int i = 0; i < prog->count; ++i) {
    const char* type_str =
      prog->program[i].type == INSTR_WORD ? "WORD" :
      prog->program[i].type == INSTR_LABEL ? "LBL" :
      prog->program[i].type == INSTR_GOTO ? "GOTO" :
      prog->program[i].type == INSTR_GOSUB ? "GOSUB" :
      prog->program[i].type == INSTR_RTN ? "RTN" :
      prog->program[i].type == INSTR_TEST ? "TEST" :
      "END";
    printf("%3d: %-6s %s\n", i, type_str, prog->program[i].arg ? prog->program[i].arg : "");
  }
}

bool load_program_from_file(const char *filename, Program *prog) {
  FILE *f = fopen(filename, "r");
  if (!f) { perror("open program"); return false; }

  char line[256];

  while (fgets(line, sizeof line, f)) {
    /* chomp \r\n */
    line[strcspn(line, "\r\n")] = '\0';
    if (line[0] == '\0') continue;

    /* prepare instruction */
    Instruction instr = {0};
    instr.arg = NULL;

    if (strncmp(line, "LBL ", 4) == 0) {
      /* ensure room for another label */
      if (prog->label_count >= MAX_LABELS) {
        fprintf(stderr, "too many labels (max %d)\n", MAX_LABELS);
        fclose(f);
        return false;
      }
      int idx = prog->label_count;

      /* copy label name safely and ensure NUL */
      size_t cap = sizeof prog->labels[idx].label;   /* 32 in your struct */
      int wrote = snprintf(prog->labels[idx].label, cap, "%s", line + 4);
      if (wrote < 0 || (size_t)wrote >= cap) {
        fprintf(stderr, "label name too long (max %zu)\n", cap - 1);
        fclose(f);
        return false;
      }

      prog->labels[idx].pc = prog->count;
      prog->label_count++;
      instr.type = INSTR_LABEL;

    } else if (strncmp(line, "GOTO ", 5) == 0) {
      instr.type = INSTR_GOTO;
      instr.arg = strdup(line + 5);
      if (!instr.arg) { fclose(f); return false; }

    } else if (strncmp(line, "GOSUB ", 6) == 0) {
      instr.type = INSTR_GOSUB;
      instr.arg = strdup(line + 6);
      if (!instr.arg) { fclose(f); return false; }

    } else if (strcmp(line, "RTN") == 0) {
      instr.type = INSTR_RTN;

    } else if (strcmp(line, "END") == 0) {
      instr.type = INSTR_END;

    } else if (strchr(line, '?') != NULL) {
      instr.type = INSTR_TEST;
      instr.arg = strdup(line);
      if (!instr.arg) { fclose(f); return false; }

    } else {
      instr.type = INSTR_WORD;
      instr.arg = strdup(line);
      if (!instr.arg) { fclose(f); return false; }
    }

    /* ensure room for another instruction */
    if (prog->count >= MAX_PROGRAM) {
      fprintf(stderr, "program too long (max %d)\n", MAX_PROGRAM);
      if (instr.arg) free(instr.arg);  /* avoid leaking the just-created arg */
      fclose(f);
      return false;
    }

    prog->program[prog->count++] = instr;
  }

  fclose(f);
  return true;
}


void free_program(Program* prog) {
    for (int i = 0; i < prog->count; ++i) {
        free(prog->program[i].arg);  // Safe even if arg is NULL
        prog->program[i].arg = NULL; // Optional: clear pointer after freeing
    }
    // If you dynamically allocated `prog` itself, you can free it too:
    // free(prog);
}


void run_RPN_code(Stack* stack, Program* prog) {
  int pc = 0;
  int call_stack[MAX_PROGRAM];
  int call_top = -1;

  while (pc < prog->count) {
    Instruction instr = prog->program[pc];
    switch (instr.type) {
    case INSTR_WORD:
      evaluate_line(stack, instr.arg);
      pc++;
      break;
    case INSTR_LABEL:
      pc++;
      break;
    case INSTR_GOTO: {
      int target = find_label(prog, instr.arg);
      if (target >= 0) pc = target;
      else {
	fprintf(stderr,"Invalid label: %s\n", instr.arg);
	return;
      }
      break;
    }
    case INSTR_GOSUB: {
      int target = find_label(prog, instr.arg);
      if (target >= 0) {
	call_stack[++call_top] = pc + 1;
	pc = target;
      } else {
	fprintf(stderr,"Invalid subroutine label: %s\n", instr.arg);
	return;
      }
      break;
    }
    case INSTR_RTN:
      if (call_top >= 0)
	pc = call_stack[call_top--];
      else {
	fprintf(stderr,"Return stack underflow\n");
	return;
      }
      break;
    case INSTR_END:
      return;
    case INSTR_TEST:
      if (evaluate_test_condition(stack, instr.arg)) {
	pc++;
      } else {
	pc += 2;
      }
      break;
    }
  }
}

