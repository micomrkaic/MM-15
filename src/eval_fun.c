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

#include "errors.h"
#include "eval_fun.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <readline/history.h>
#include <gsl/gsl_complex.h>        // for gsl_complex
#include <gsl/gsl_math.h>           // for M_PI
#include <stdbool.h>                // for true
#include "lexer.h"
#include "stack.h"
#include "stack_io.h"
#include "string_fun.h"
#include "linear_algebra.h"
#include "matrix_fun.h"
#include "math_parsers.h"
#include "math_helpers.h"
#include "binary_fun.h"
#include "unary_fun.h"
#include "help.h"
#include "print_fun.h"
#include "my_date_fun.h"
#include "poly_fun.h"
#include "stat_fun.h"
#include "registers.h"
#include "compare_fun.h"
#include "words.h"
#include "run_machine.h"
#include "integration_and_zeros.h"
#include "globals.h"
#include "my_astronomy.h"
#include "ops.h"

#define MAX_WORD_DEPTH 256
static int  word_depth = 0;       /* current nesting of word/macro calls */
static bool word_aborted = false; /* set when the depth limit is hit: unwind */

// **************** The main loop in this file ****************
/* Each top-level token is atomic: a snapshot of the stack is taken before
   it runs, and if the token reports an error the stack is restored and the
   rest of the line is skipped. A user word is one token for this purpose. */
void evaluate_line(Stack *stack, char* line) {
  int def = is_word_definition(line);
  if (def != 0) {
    // definition line (accepted or rejected) is already handled; do not lex it
    return;
  }

  Stack before;
  init_stack(&before);
  word_aborted = false;
  Lexer lexer = (Lexer){ .input = line, .pos = 0 };
  Token tok;
  do {
    tok = next_token(&lexer);
    if (tok.type == TOK_EOF) break;
    long errors_before = error_count;
    copy_stack(&before, stack);
    evaluate_one_token(stack, tok);
    if (error_count > errors_before) {
      copy_stack(stack, &before);
      break;
    }
  } while (!word_aborted);
  free_stack(&before);
  word_aborted = false;
}

/* void evaluate_line(Stack *stack, char* line) { */
/*   Lexer lexer = {line, 0}; */
/*   Token tok; */

/*   if (!is_word_definition(line))   // Check if a new word; insert  if it is */
/*     do {   // The lexer loop */
/*       tok = next_token(&lexer); */
/*       evaluate_one_token(stack, tok);     */
/*     } while (tok.type != TOK_EOF); */
/* } */

// **************** Process one token ****************
void evaluate_one_token(Stack *stack, Token tok) {
  switch (tok.type) {
  case TOK_EOF:
    return;
  case TOK_NUMBER:
    push_real(stack, atof(tok.text));
    return;
  case TOK_COMPLEX: {
    gsl_complex z;
    if (read_complex(tok.text, &z)) push_complex(stack,z);
    return;}
  case TOK_STRING:
    push_string(stack, tok.text);
    return;
  case TOK_MATRIX_FILE:
    read_matrix_from_file(stack, tok.text);
    return;
  case TOK_MATRIX_INLINE_REAL:
    push_matrix_real(stack,parse_matrix_literal(tok.text));
    return;
  case TOK_MATRIX_INLINE_COMPLEX:
    push_matrix_complex(stack,parse_complex_matrix_literal(tok.text));
    return;
  case TOK_MATRIX_INLINE_MIXED:
    push_matrix_complex(stack,parse_complex_matrix_literal(tok.text));
    return;
  case TOK_PLUS: case TOK_MINUS: case TOK_STAR: case TOK_SLASH: case TOK_CARET:
  case TOK_DOT_STAR: case TOK_DOT_SLASH: case TOK_DOT_CARET: {
    int idx = find_op(tok.text);
    if (idx >= 0) call_op(stack, idx);
    else mm_error("Unknown operator: %s", tok.text);
    return;
  }
  case TOK_BRA: case TOK_KET: case TOK_VERTICAL:
    mm_error("Illegal token: %s", tok.text);
    return;
  case TOK_COLON:
    mm_error("':' starts a word definition: ': name body ;' on one line");
    return;
  case TOK_SEMICOLON:
    mm_error("';' without a matching ':'");
    return;
  case TOK_IDENTIFIER: {
    user_word *w = find_macro(tok.text);
    if (w == NULL) w = find_word(tok.text);
    if (w == NULL) {
      mm_error("Unknown word: %s", tok.text);
      return;
    }
    /* A word may call itself; a runaway recursion used to overflow the C
       stack and crash. Nesting is counted across macros and words. */
    if (word_depth >= MAX_WORD_DEPTH) {
      mm_error("Error: word nesting deeper than %d (in '%s'); aborting.\n",
              MAX_WORD_DEPTH, w->name);
      word_aborted = true;
      return;
    }
    char *sub_line = strdup(w->body);
    Lexer sub_lexer = {sub_line, 0};
    Token sub_tok;
    word_depth++;
    do {   // The lexer loop
      sub_tok = next_token(&sub_lexer);
      evaluate_one_token(stack, sub_tok);
    } while (sub_tok.type != TOK_EOF && !word_aborted);
    word_depth--;
    free(sub_line);
    return;
  }
  case TOK_FUNCTION: {

    int idx = find_op(tok.text);
    if (idx >= 0) call_op(stack, idx);
    else mm_error("Unknown command: %s", tok.text);
    return;
  }
  case TOK_XT:
    mm_error("%s: execution tokens arrive in the next step", tok.text);
    return;
  case TOK_UNKNOWN:
    mm_error("Illegal token: %s", tok.text);
    return;
  default:
    mm_error("Unhandled token: %s", tok.text);
    return;
  }
}

