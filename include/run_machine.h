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

#ifndef RUN_MACHINE_H
#define RUN_MACHINE_H

#include <stdbool.h>
#include "stack.h"

#define MAX_PROGRAM 1024
#define MAX_LABELS 128

// === Types ===
typedef enum {
  INSTR_WORD,
  INSTR_LABEL,
  INSTR_GOTO,
  INSTR_GOSUB,
  INSTR_RTN,
  INSTR_END,
  INSTR_TEST
} instr_type;

typedef struct {
  instr_type type;
  char* arg;
} Instruction;

typedef struct {
  char label[32];
  int pc;
} label_entry;

typedef struct {
  Instruction program[MAX_PROGRAM];
  int count;
  label_entry labels[MAX_LABELS];
  int label_count;
} Program;

// Top-of-stack comparisons
bool is_top_eq_0(Stack* stack);
bool is_top_neq_0(Stack* stack);
bool is_top_gt_0(Stack* stack);
bool is_top_lt_0(Stack* stack);
bool is_top_gte_0(Stack* stack);
bool is_top_lte_0(Stack* stack);

bool is_top_eq(Stack* stack);
bool is_top_neq(Stack* stack);
bool is_top_gt(Stack* stack);
bool is_top_lt(Stack* stack);
bool is_top_gte(Stack* stack);
bool is_top_lte(Stack* stack);

// Counter comparisons
bool is_ctr_eq_0(Stack* stack);
bool is_ctr_neq_0(Stack* stack);
bool is_ctr_gt_0(Stack* stack);
bool is_ctr_lt_0(Stack* stack);
bool is_ctr_gte_0(Stack* stack);
bool is_ctr_lte_0(Stack* stack);

// Dispatcher
bool is_ctr_compare(Stack* stack, const char* op);

// Batch execution
int run_batch(Stack * stack, char *fname);

// Program execution
void list_program(const Program* prog);
bool load_program_from_file(const char* filename, Program* prog);
void free_program(Program* prog);
void run_RPN_code(Stack* stack, Program* prog);

#endif // RUN_MACHINE_H
