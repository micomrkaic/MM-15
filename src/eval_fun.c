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
#include <math.h>
#include <complex.h>
#include <time.h>
#include <readline/history.h>
#include <readline/readline.h>
#include "lexer.h"
#include "stack.h"
#include "string_fun.h"
#include "linear_algebra.h"
#include "matrix_fun.h"
#include "math_parsers.h"
#include "math_helpers.h"
#include "binary_fun.h"
#include "unary_fun.h"
#include "splash_and_help.h"
#include "print_fun.h"
#include "my_date_fun.h"
#include "matrix_fun.h"
#include "poly_fun.h"
#include "stat_fun.h"
#include "registers.h"
#include "compare_fun.h"
#include "eval_fun.h"
#include "words.h"
#include "run_machine.h"
#include "integration_and_zeros.h"
#include "globals.h"

typedef void (*unary_func)(Stack *stack);

typedef struct {
  const char* name;
  unary_func func;
} immutable_unary_op;

static const immutable_unary_op immutable_unary_ops[] = {
  {"sin",   sin_wrapper},
  {"cos",   cos_wrapper},
  {"tan",   tan_wrapper},
  {"asin",  asin_wrapper},
  {"acos",  acos_wrapper},
  {"atan",  atan_wrapper},
  {"sinh",  sinh_wrapper},
  {"cosh",  cosh_wrapper},
  {"tanh",  tanh_wrapper},
  {"asinh", asinh_wrapper},
  {"acosh", acosh_wrapper},
  {"atanh", atanh_wrapper},
  {"exp",   exp_wrapper},
  {"chs",   chs_wrapper},
  {"inv",   inv_wrapper},
  {NULL,    NULL}
};

typedef struct {
  const char* name;
  const char* axis;
  const char* operation;
} matrix_reduce_op;

static const matrix_reduce_op reduce_ops[] = {
  {"cmean", "col", "mean"},
  {"rmean", "row", "mean"},
  {"csum",  "col", "sum"},
  {"rsum",  "row", "sum"},
  {"cvar",  "col", "var"},
  {"rvar",  "row", "var"},
  {"cmin", "col", "min"},
  {"rmin", "row", "min"},
  {"cmax",  "col", "max"},
  {"rmax",  "row", "max"},
  {NULL, NULL, NULL}
};

typedef int (*matrix_func)(Stack*);

typedef struct {
  const char* name;
  matrix_func func;
} matrix_op;

static const matrix_op matrix_ops[] = {
  {"minv",    matrix_inverse},
  {"pinv",    matrix_pseudoinverse},
  {"det",     matrix_determinant},
  {"eig",     matrix_eigen_decompose},
  {"tran",    matrix_transpose},
  {"'",       matrix_transpose},
  {"reshape", reshape_matrix},
  {"get_aij", select_matrix_element},
  {"set_aij", set_matrix_element},
  {"kron",    kronecker_top_two},
  {"diag",    matrix_extract_diagonal},
  {"to_diag", make_diag_matrix},
  {"chol",    matrix_cholesky},
  {"svd",     matrix_svd},
  {"dim",     matrix_dimensions},
  {"eye",     make_unit_matrix},
  {"ones",    make_matrix_of_ones},
  {"rrange",  make_row_range},
  {"zeroes",  make_matrix_of_zeroes},
  {"rand",    make_random_matrix},
  {"randn",   make_gaussian_random_matrix},
  {"join_v",  stack_join_matrix_vertical},
  {"join_h",  stack_join_matrix_horizontal},
  {"cumsum_r",matrix_cumsum_rows},
  {"cumsum_c",matrix_cumsum_cols},
  {NULL,      NULL}
};

// **************** The main loop in this file ****************
void evaluate_line(Stack *stack, char* line) {
  Lexer lexer = {line, 0};
  Token tok;

  if (!is_word_definition(line))   // Check if a new word; insert  if it is
    do {   // The lexer loop
      tok = next_token(&lexer);
      evaluate_one_token(stack, tok);    
    } while (tok.type != TOK_EOF);
}

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
  case TOK_PLUS:
    add_top_two(stack);
    return;
  case TOK_MINUS:
    sub_top_two(stack);
    return;
  case TOK_STAR:
    mul_top_two(stack);
    return;
  case TOK_SLASH:
    div_top_two(stack);
    return;
  case TOK_CARET:
    pow_top_two(stack);
    return;
  case TOK_DOT_STAR:
    dot_mult_top_two(stack);
    return;
  case TOK_DOT_SLASH:
    dot_div_top_two(stack);
    return;
  case TOK_DOT_CARET:
    dot_pow_top_two(stack);
    return;
  case TOK_BRA:
    printf("< \n");
    return;
  case TOK_KET:
    printf("> \n");
    return;
  case TOK_COLON:
    printf(": \n");
    return;
  case TOK_SEMICOLON:
    printf("; \n");
    return;
  case TOK_IDENTIFIER: {
    user_word *m=find_macro(tok.text);
    if (m!=NULL) {
      char *sub_line = strdup(m->body);
      Lexer sub_lexer = {sub_line, 0};
      Token sub_tok;
      do {   // The lexer loop
	sub_tok = next_token(&sub_lexer);
	evaluate_one_token(stack, sub_tok);    
      } while (sub_tok.type != TOK_EOF);
      return;
    }
    
    user_word *w=find_word(tok.text);
    if (w==NULL)
      printf("Unknown identifier!\n");
    else {
      char *sub_line = strdup(w->body);
      Lexer sub_lexer = {sub_line, 0};
      Token sub_tok;
      do {   // The lexer loop
	sub_tok = next_token(&sub_lexer);
	evaluate_one_token(stack, sub_tok);    
      } while (sub_tok.type != TOK_EOF);
    }
    return;
  }
  case TOK_FUNCTION: {

    // Meta level
    if (!strcmp("eval",tok.text)) {
      if (stack->top < 0) {
        fprintf(stderr, "Stack is empty: nothing to evaluate.\n");
        return;
      }
      stack_element t = pop(stack);
      if (t.type != TYPE_STRING) {
        fprintf(stderr, "Top of stack is not a string: cannot evaluate.\n");
      } else evaluate_line(stack, t.string);
      return;
    }

    if (!strcmp("batch",tok.text)) {
      if (stack->top < 0) {
        fprintf(stderr, "Stack is empty: no batch to run.\n");
        return;
      }
      stack_element t = pop(stack);
      if (t.type != TYPE_STRING) {
        fprintf(stderr, "Top of stack is not a string: cannot evaluate.\n");
      } else run_batch(stack, t.string);
      return;
    }
    
    if (!strcmp("run",tok.text)) {
      if (stack->top < 0) {
        fprintf(stderr, "Stack is empty: no program to run.\n");
        return;
      }
      stack_element t = pop(stack);
      if (t.type != TYPE_STRING) {
	fprintf(stderr, "Top of stack is not a string: cannot evaluate.\n");
      } else {
	Program prog = {.count = 0, .label_count = 0};
	if (!load_program_from_file(t.string, &prog)) {
	  fprintf(stderr, "Failed to load program.\n");
	  return;
	}
	list_program(&prog);
	run_RPN_code(stack, &prog);
	free_program(&prog);
      }
      return;
    }

    // ++++++++++++++++ Nullary functions ++++++++++++++++
    // Constants
    if (!strcmp("gravity",tok.text)) {push_real(stack,9.81); return; }
    if (!strcmp("pi",tok.text)) { push_real(stack,M_PI); return; }
    if (!strcmp("e",tok.text)) { push_real(stack,exp(1.0)); return; }
    if (!strcmp("inf",tok.text)) { push_real(stack,INFINITY); return; }
    if (!strcmp("nan",tok.text)) { push_real(stack,NAN); return; }

    // Misc
    if (!strcmp("help",tok.text)) { help_menu(); return; }
    if (!strcmp("listfcns",tok.text)) { list_all_functions_sorted(); return; }
    if (!strcmp("clrhist",tok.text)) { clear_history(); return; }
    if (!strcmp("fuck",tok.text)) { whose_place(); return; }
    
    // Utility functions
    if (!strcmp("pm",tok.text)) {print_matrix(stack); skip_stack_printing = true; return;}
    if (!strcmp("ps",tok.text)) {print_stack(stack,NULL); return;}
    if (!strcmp("print",tok.text)) {print_top_scalar(stack); return;}
    if (!strcmp("setprec",tok.text)) {set_print_precision(stack); return;}
    if (!strcmp("sfs",tok.text)) {swap_fixed_scientific(); return;}
    
    // Date and time functions
    if (!strcmp("days2eoy",tok.text)) {days_to_end_of_year(stack); return; } 
    if (!strcmp("num2date",tok.text)) {make_date_string(stack); return; }
    if (!strcmp("ddays",tok.text)) { delta_days_strings(stack); return; }
    if (!strcmp("today",tok.text)) { push_today_date(stack); return; }
    if (!strcmp("dow",tok.text)) { push_weekday_name_from_date_string(stack); return; }
    if (!strcmp("dateplus",tok.text)) { date_plus_days(stack); return; }
    if (!strcmp("edmy",tok.text)) {  extract_day_month_year(stack); return; }
 
    // Stack functions
    if (!strcmp("drop",tok.text)) { pop_and_free(stack); return; }
    if (!strcmp("clst",tok.text)) { free_stack(stack); return; }
    if (!strcmp("swap",tok.text)) { swap(stack); return; }
    if (!strcmp("dup",tok.text)) { stack_dup(stack); return; }
    if (!strcmp("nip",tok.text)) { stack_nip(stack); return; }
    if (!strcmp("tuck",tok.text)) { stack_tuck(stack); return; }
    if (!strcmp("roll",tok.text)) { stack_roll(stack,2); return; }
    if (!strcmp("over",tok.text)) { stack_over(stack); return; }

    // Polynomial functions
    if (!strcmp("roots",tok.text)) { poly_roots(stack); return;}
    if (!strcmp("pval",tok.text)) { poly_eval(stack); return; }

    // Integration and zeros
    if (!strcmp("integrate",tok.text)) { integrate(stack); return;}
    if (!strcmp("fzero",tok.text)) { find_zero(stack); return; }
    if (!strcmp("set_intg_tol",tok.text)) { set_integration_precision(stack); return; }
    if (!strcmp("set_f0_tol",tok.text))  { set_f0_precision(stack); return; }
 
    // Comparison and logic functions
    if (!strcmp("eq",tok.text)) { dot_cmp_top_two(stack, CMP_EQ); return; }
    if (!strcmp("neq",tok.text)) { dot_cmp_top_two(stack, CMP_NE); return; }
    if (!strcmp("lt",tok.text)) { dot_cmp_top_two(stack, CMP_LT); return; }
    if (!strcmp("leq",tok.text)) { dot_cmp_top_two(stack, CMP_LE); return; }
    if (!strcmp("gt",tok.text)) { dot_cmp_top_two(stack, CMP_GT); return; }
    if (!strcmp("geq",tok.text)) { dot_cmp_top_two(stack, CMP_GE); return; } 
    if (!strcmp("and",tok.text)) { dot_cmp_top_two(stack, CMP_AND); return; } 
    if (!strcmp("or",tok.text)) { dot_cmp_top_two(stack, CMP_OR); return; } 
    if (!strcmp("not",tok.text)) { logical_not_wrapper(stack); return; } 
    
    // Special math functions
    if (!strcmp("npdf",tok.text)) { npdf_wrapper(stack); return; }
    if (!strcmp("ncdf",tok.text)) { ncdf_wrapper(stack); return; }
    if (!strcmp("nquant",tok.text)) { nquant_wrapper(stack); return; }
    if (!strcmp("gamma",tok.text)) { gamma_wrapper(stack); return; }
    if (!strcmp("ln_gamma",tok.text)) { ln_gamma_wrapper(stack); return; }
    if (!strcmp("beta",tok.text)) { beta_wrapper(stack); return; }
    if (!strcmp("ln_beta",tok.text)) { ln_beta_wrapper(stack); return; }

    // Parts of numbers
    if (!strcmp("frac",tok.text)) { frac_wrapper(stack); return; }
    if (!strcmp("intg",tok.text)) { intg_wrapper(stack); return; }
    
    // Register functions
    if (!strcmp("ffr",tok.text)) {  find_first_free_register(stack); return; }
    if (!strcmp("rcl",tok.text)) { recall_from_register(stack); return; }
    if (!strcmp("sto",tok.text)) { store_to_register(stack); return; }
    if (!strcmp("pr",tok.text)) { show_registers_status(); return; }
    if (!strcmp("saveregs",tok.text)) { save_registers_to_file("registers.txt"); return; }
    if (!strcmp("loadregs",tok.text)) { load_registers_from_file("registers.txt"); return; }
    if (!strcmp("clregs",tok.text)) { free_all_registers(); return; }

    // **************** String functions ****************
    if (!strcmp("substr",tok.text)) {my_substring(stack); return;}
    if (!strcmp("scon",tok.text)) {concatenate(stack); return;}
    if (!strcmp("s2l",tok.text)) { to_lower(stack); return; }
    if (!strcmp("s2u",tok.text)) { to_upper(stack); return; }
    if (!strcmp("slen",tok.text)) { string_length(stack); return; }
    if (!strcmp("srev",tok.text)) { string_reverse(stack); return; }       
    if (!strcmp("int2str",tok.text)) { top_to_string(stack); return; }       

    // **************** Macros and user defined word functions ****************
    if (!strcmp("listmacros",tok.text)) {list_macros(); return;}
    if (!strcmp("listwords",tok.text)) {list_words(); return;}
    if (!strcmp("loadwords",tok.text)) {load_words_from_file(); return;}
    if (!strcmp("savewords",tok.text)) {save_words_to_file(); return;}
    if (!strcmp("clrwords",tok.text)) {clear_words(); return;}
    if (!strcmp("selword",tok.text)) {word_select(stack); return;}
    if (!strcmp("delword",tok.text)) {delete_word(stack); return;}
    
    // **************** Matrix operations ****************
    for (int i = 0; matrix_ops[i].name != NULL; ++i) {
      if (!strcmp(tok.text, matrix_ops[i].name)) {
	matrix_ops[i].func(stack);
	return;
      }
    }

    if (!strcmp("split_mat",tok.text)) {split_matrix(stack); return; }
    
    // **************** Immutable unary functions ****************      
    for (int i = 0; immutable_unary_ops[i].name != NULL; ++i) {
      if (!strcmp(tok.text, immutable_unary_ops[i].name)) {
	immutable_unary_ops[i].func(stack);
	return;
      }
    }
    // **************** Mutable unary operations ****************     
    if (!strcmp(tok.text, "split_c")) { split_complex(stack); return; }    
    if (!strcmp(tok.text, "abs")) { abs_wrapper(stack); return; }
    if (!strcmp(tok.text, "re")) { re_wrapper(stack); return; }
    if (!strcmp(tok.text, "im")) { im_wrapper(stack); return; }
    if (!strcmp(tok.text, "arg")) { arg_wrapper(stack); return; }
    if (!strcmp(tok.text, "re2c")) { real2complex(stack); return; }
    if (!strcmp(tok.text, "j2r")) {  join_2_reals(stack); return; }
    if (!strcmp(tok.text, "ln")) { ln_wrapper(stack); return; }
    if (!strcmp(tok.text, "log")) { log_wrapper(stack); return; }
    if (!strcmp(tok.text, "sqrt")) { sqrt_wrapper(stack); return; }   
    
    // Matrix reduction functions
    for (int i = 0; reduce_ops[i].name != NULL; ++i) {
      if (!strcmp(tok.text, reduce_ops[i].name)) {
        matrix_reduce(stack, reduce_ops[i].axis, reduce_ops[i].operation);
        return;
      }
    }
    return;  
    case TOK_VERTICAL:
      printf("| \n");
      return;
      case TOK_UNKNOWN:
	printf("Illegal token.\n");
	return;
	default:
	  printf("Unhandled token type.\n");
	  return;
  }
  }
}

