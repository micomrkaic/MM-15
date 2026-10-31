#define _POSIX_C_SOURCE 200809L
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

#include "errors.h"
#include <gsl/gsl_math.h>
#include <math.h>
#include <readline/history.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ops.h"
#include "binary_fun.h"
#include "compare_fun.h"
#include "globals.h"
#include "help.h"
#include "numerics.h"
#include "linear_algebra.h"
#include "math_helpers.h"
#include "matrix_fun.h"
#include "my_astronomy.h"
#include "my_date_fun.h"
#include "poly_fun.h"
#include "print_fun.h"
#include "registers.h"
#include "stack_io.h"
#include "stat_fun.h"
#include "string_fun.h"
#include "unary_fun.h"
#include "words.h"
#include "eval_fun.h"
#include "run_machine.h"

/* ---- meta: evaluate a string, run a batch file, run a program file ---- */
static int pop_string_arg(Stack *stack, const char *what, char **out) {
  stack_element t = pop(stack);
  if (t.type != TYPE_STRING) {
    stack_element_free(&t);
    return mm_error("%s: expected a string on the stack", what);
  }
  *out = t.string;
  return 0;
}

static int op_eval(Stack *stack) {
  char *s;
  if (pop_string_arg(stack, "eval", &s) != 0) return 0;
  evaluate_line(stack, s);
  free(s);
  return 0;
}

static int op_batch(Stack *stack) {
  char *s;
  if (pop_string_arg(stack, "batch", &s) != 0) return 0;
  run_batch(stack, s);
  free(s);
  return 0;
}

static int op_run(Stack *stack) {
  char *s;
  if (pop_string_arg(stack, "run", &s) != 0) return 0;
  Program prog = {.count = 0, .label_count = 0};
  if (!load_program_from_file(s, &prog)) {
    mm_error("Failed to load program %s", s);
  } else {
    if (verbose_mode) list_program(&prog);
    run_RPN_code(stack, &prog);
  }
  free_program(&prog);
  free(s);
  return 0;
}

/* ---- small operations that live here ---- */

/* n roll: move the item at depth n (0 = top) to the top; 1 roll = swap. */
static int op_roll_n(Stack *stack) {
  stack_element e = pop(stack);
  if (e.type != TYPE_REAL || e.real != (int)e.real || e.real < 0) {
    stack_element_free(&e);
    return mm_error("roll: depth must be a non-negative integer");
  }
  stack_roll(stack, (int)e.real);
  return 0;
}

/* n pick: copy the item at depth n (0 = top) to the top; 0 pick = dup. */
static int op_pick(Stack *stack) {
  stack_element e = pop(stack);
  if (e.type != TYPE_REAL || e.real != (int)e.real || e.real < 0) {
    stack_element_free(&e);
    return mm_error("pick: depth must be a non-negative integer");
  }
  int n = (int)e.real;
  if (stack->top - n < 0) return mm_error("pick: depth %d exceeds the stack", n);
  if (stack->top + 1 >= STACK_SIZE) return mm_error("Stack overflow");
  if (stack_element_clone(&stack->items[stack->top + 1], &stack->items[stack->top - n]) != 0)
    return mm_error("pick: failed to copy element");
  stack->top++;
  return 0;
}

static int op_depth(Stack *stack) { push_real(stack, stack->top + 1); return 0; }

/* ---- real-only arithmetic helpers ---- */
static int pop_two_reals(Stack *stack, const char *what, double *y, double *x) {
  if (stack->items[stack->top].type != TYPE_REAL || stack->items[stack->top - 1].type != TYPE_REAL)
    return mm_error("%s: needs two real numbers", what);
  *x = pop(stack).real;
  *y = pop(stack).real;
  return 0;
}

static int op_pct(Stack *stack) {          /* y x -- y y*x/100  (HP %) */
  double y, x;
  if (pop_two_reals(stack, "%", &y, &x)) return 0;
  push_real(stack, y); push_real(stack, y * x / 100.0); return 0;
}
static int op_pctt(Stack *stack) {         /* y x -- y 100*x/y  (HP %T) */
  double y, x;
  if (pop_two_reals(stack, "%t", &y, &x)) return 0;
  if (y == 0.0) return mm_error("%%t: total is zero");
  push_real(stack, y); push_real(stack, 100.0 * x / y); return 0;
}
static int op_pctchg(Stack *stack) {       /* y x -- y 100*(x-y)/y  (HP delta%) */
  double y, x;
  if (pop_two_reals(stack, "%chg", &y, &x)) return 0;
  if (y == 0.0) return mm_error("%%chg: base is zero");
  push_real(stack, y); push_real(stack, 100.0 * (x - y) / y); return 0;
}
static int op_mod(Stack *stack) {          /* floored modulo, sign of x */
  double y, x;
  if (pop_two_reals(stack, "mod", &y, &x)) return 0;
  if (x == 0.0) return mm_error("mod: modulus is zero");
  double r = fmod(y, x);
  if (r != 0.0 && ((r < 0) != (x < 0))) r += x;
  push_real(stack, r); return 0;
}
static int op_fact(Stack *stack) {
  if (stack->items[stack->top].type != TYPE_REAL) return mm_error("fact: needs a real number");
  double x = pop(stack).real;
  if (x < 0.0 && x == floor(x)) return mm_error("fact: negative integer");
  double r = tgamma(x + 1.0);
  if (!isfinite(r)) return mm_error("fact: overflow");
  push_real(stack, x == floor(x) && x <= 22 ? round(r) : r); return 0;
}
static int op_ncr(Stack *stack) {
  double n, k;
  if (pop_two_reals(stack, "ncr", &n, &k)) return 0;
  if (n != floor(n) || k != floor(k) || n < 0 || k < 0 || k > n) return mm_error("ncr: need integers 0 <= k <= n");
  if (k > n - k) k = n - k;
  double r = 1.0;
  for (double i = 1.0; i <= k; i += 1.0) r = r * (n - k + i) / i;
  push_real(stack, r < 9007199254740992.0 ? round(r) : r); return 0;
}
static int op_npr(Stack *stack) {
  double n, k;
  if (pop_two_reals(stack, "npr", &n, &k)) return 0;
  if (n != floor(n) || k != floor(k) || n < 0 || k < 0 || k > n) return mm_error("npr: need integers 0 <= k <= n");
  double r = 1.0;
  for (double i = n - k + 1.0; i <= n; i += 1.0) r *= i;
  if (!isfinite(r)) return mm_error("npr: overflow");
  push_real(stack, r); return 0;
}

static gsl_complex conj_c(gsl_complex z) { return gsl_complex_conjugate(z); }
static double conj_r(double x) { return x; }

static void conj_wrapper(Stack *stack) {
  switch (stack_top_type(stack)) {
  case TYPE_REAL: return;
  case TYPE_COMPLEX: apply_complex_unary(stack, conj_c); return;
  case TYPE_MATRIX_REAL: apply_real_matrix_unary_inplace(stack, conj_r); return;
  case TYPE_MATRIX_COMPLEX: apply_complex_matrix_unary_inplace(stack, conj_c); return;
  default: mm_error("conj: unsupported type");
  }
}

/* ---- numerical methods: pop reals, run, push the result ---- */
static int pop_real_arg(Stack *stack, const char *what, double *out) {
  if (stack->items[stack->top].type != TYPE_REAL) return mm_error("%s: expected a real number", what);
  *out = pop(stack).real;
  return 0;
}
static int op_exec(Stack *stack) { double xt; if (pop_real_arg(stack, "exec", &xt)) return -1; return xt_exec(stack, xt); }
static int op_tol(Stack *stack) {
  double t; if (pop_real_arg(stack, "tol", &t)) return -1;
  if (!(t > 0.0 && t < 1.0)) return mm_error("tol: tolerance must be between 0 and 1");
  num_tol = t; return 0;
}
static int op_nevals(Stack *stack) { push_real(stack, (double)num_evals); return 0; }
static int op_interr(Stack *stack) { push_real(stack, num_last_err); return 0; }
#define NUM3(nm, call) static int op_##nm(Stack *stack) {                      \
  double xt, b, a, r;                                                        \
  if (pop_real_arg(stack, #nm, &xt) || pop_real_arg(stack, #nm, &b) || pop_real_arg(stack, #nm, &a)) return -1; \
  if (call != 0) return -1;                                                  \
  push_real(stack, r); return 0; }
#define NUM2(nm, call) static int op_##nm(Stack *stack) {                      \
  double xt, a, r;                                                           \
  if (pop_real_arg(stack, #nm, &xt) || pop_real_arg(stack, #nm, &a)) return -1; \
  if (call != 0) return -1;                                                  \
  push_real(stack, r); return 0; }
static double err_dummy;
NUM3(integrate, num_integrate(stack, xt, a, b, &r, &err_dummy))
NUM3(romberg,   num_romberg(stack, xt, a, b, &r))
NUM3(fzero,     num_fzero(stack, xt, a, b, &r))
NUM2(solve,     num_solve(stack, xt, a, &r))
NUM3(minimize,  num_minimize(stack, xt, a, b, &r))
NUM2(deriv,     num_deriv(stack, xt, a, &r))
NUM3(fsum,      num_fsum(stack, xt, a, b, false, &r))
NUM3(fprod,     num_fsum(stack, xt, a, b, true, &r))

/* GENERATED ONCE by gen_ops.py from the 1.x dispatcher and help table; now maintained by hand. */
static int op_add(Stack *stack) { (void)stack; (void)(add_top_two(stack)); return 0; }
static int op_sub(Stack *stack) { (void)stack; (void)(sub_top_two(stack)); return 0; }
static int op_mul(Stack *stack) { (void)stack; (void)(mul_top_two(stack)); return 0; }
static int op_div(Stack *stack) { (void)stack; (void)(div_top_two(stack)); return 0; }
static int op_pow_(Stack *stack) { (void)stack; (void)(pow_top_two(stack)); return 0; }
static int op_pow(Stack *stack) { (void)stack; (void)(pow_top_two(stack)); return 0; }
static int op_dot_mul(Stack *stack) { (void)stack; (void)(dot_mult_top_two(stack)); return 0; }
static int op_dot_div(Stack *stack) { (void)stack; (void)(dot_div_top_two(stack)); return 0; }
static int op_dot_pow(Stack *stack) { (void)stack; (void)(dot_pow_top_two(stack)); return 0; }
static int op_chs(Stack *stack) { (void)stack; (void)(chs_wrapper(stack)); return 0; }
static int op_inv(Stack *stack) { (void)stack; (void)(inv_wrapper(stack)); return 0; }
static int op_abs(Stack *stack) { (void)stack; (void)(abs_wrapper(stack)); return 0; }
static int op_sqrt(Stack *stack) { (void)stack; (void)(sqrt_wrapper(stack)); return 0; }
static int op_frac(Stack *stack) { (void)stack; (void)(frac_wrapper(stack)); return 0; }
static int op_intg(Stack *stack) { (void)stack; (void)(intg_wrapper(stack)); return 0; }
static int op_exp(Stack *stack) { (void)stack; (void)(exp_wrapper(stack)); return 0; }
static int op_ln(Stack *stack) { (void)stack; (void)(ln_wrapper(stack)); return 0; }
static int op_log(Stack *stack) { (void)stack; (void)(log_wrapper(stack)); return 0; }
static int op_sin(Stack *stack) { (void)stack; (void)(sin_wrapper(stack)); return 0; }
static int op_cos(Stack *stack) { (void)stack; (void)(cos_wrapper(stack)); return 0; }
static int op_tan(Stack *stack) { (void)stack; (void)(tan_wrapper(stack)); return 0; }
static int op_asin(Stack *stack) { (void)stack; (void)(asin_wrapper(stack)); return 0; }
static int op_acos(Stack *stack) { (void)stack; (void)(acos_wrapper(stack)); return 0; }
static int op_atan(Stack *stack) { (void)stack; (void)(atan_wrapper(stack)); return 0; }
static int op_sinh(Stack *stack) { (void)stack; (void)(sinh_wrapper(stack)); return 0; }
static int op_cosh(Stack *stack) { (void)stack; (void)(cosh_wrapper(stack)); return 0; }
static int op_tanh(Stack *stack) { (void)stack; (void)(tanh_wrapper(stack)); return 0; }
static int op_asinh(Stack *stack) { (void)stack; (void)(asinh_wrapper(stack)); return 0; }
static int op_acosh(Stack *stack) { (void)stack; (void)(acosh_wrapper(stack)); return 0; }
static int op_atanh(Stack *stack) { (void)stack; (void)(atanh_wrapper(stack)); return 0; }
static int op_re(Stack *stack) { (void)stack; (void)(re_wrapper(stack)); return 0; }
static int op_im(Stack *stack) { (void)stack; (void)(im_wrapper(stack)); return 0; }
static int op_arg(Stack *stack) { (void)stack; (void)(arg_wrapper(stack)); return 0; }
static int op_conj(Stack *stack) { (void)stack; (void)(conj_wrapper(stack)); return 0; }
static int op_re2c(Stack *stack) { (void)stack; (void)(real2complex(stack)); return 0; }
static int op_split_c(Stack *stack) { (void)stack; (void)(split_complex(stack)); return 0; }
static int op_j2r(Stack *stack) { (void)stack; (void)(join_2_reals(stack)); return 0; }
static int op_npdf(Stack *stack) { (void)stack; (void)(npdf_wrapper(stack)); return 0; }
static int op_ncdf(Stack *stack) { (void)stack; (void)(ncdf_wrapper(stack)); return 0; }
static int op_nquant(Stack *stack) { (void)stack; (void)(nquant_wrapper(stack)); return 0; }
static int op_gamma(Stack *stack) { (void)stack; (void)(gamma_wrapper(stack)); return 0; }
static int op_ln_gamma(Stack *stack) { (void)stack; (void)(ln_gamma_wrapper(stack)); return 0; }
static int op_beta(Stack *stack) { (void)stack; (void)(beta_wrapper(stack)); return 0; }
static int op_ln_beta(Stack *stack) { (void)stack; (void)(ln_beta_wrapper(stack)); return 0; }
static int op_pi(Stack *stack) { (void)stack; (void)(push_real(stack, M_PI)); return 0; }
static int op_e(Stack *stack) { (void)stack; (void)(push_real(stack, exp(1.0))); return 0; }
static int op_gravity(Stack *stack) { (void)stack; (void)(push_real(stack, 9.80665)); return 0; }
static int op_inf(Stack *stack) { (void)stack; (void)(push_real(stack, INFINITY)); return 0; }
static int op_nan(Stack *stack) { (void)stack; (void)(push_real(stack, NAN)); return 0; }
static int op_eq(Stack *stack) { (void)stack; (void)(dot_cmp_top_two(stack, CMP_EQ)); return 0; }
static int op_neq(Stack *stack) { (void)stack; (void)(dot_cmp_top_two(stack, CMP_NE)); return 0; }
static int op_lt(Stack *stack) { (void)stack; (void)(dot_cmp_top_two(stack, CMP_LT)); return 0; }
static int op_leq(Stack *stack) { (void)stack; (void)(dot_cmp_top_two(stack, CMP_LE)); return 0; }
static int op_gt(Stack *stack) { (void)stack; (void)(dot_cmp_top_two(stack, CMP_GT)); return 0; }
static int op_geq(Stack *stack) { (void)stack; (void)(dot_cmp_top_two(stack, CMP_GE)); return 0; }
static int op_and(Stack *stack) { (void)stack; (void)(dot_cmp_top_two(stack, CMP_AND)); return 0; }
static int op_or(Stack *stack) { (void)stack; (void)(dot_cmp_top_two(stack, CMP_OR)); return 0; }
static int op_not(Stack *stack) { (void)stack; (void)(logical_not_wrapper(stack)); return 0; }
static int op_dup(Stack *stack) { (void)stack; (void)(stack_dup(stack)); return 0; }
static int op_drop(Stack *stack) { (void)stack; (void)(pop_and_free(stack)); return 0; }
static int op_swap(Stack *stack) { (void)stack; (void)(swap(stack)); return 0; }
static int op_over(Stack *stack) { (void)stack; (void)(stack_over(stack)); return 0; }
static int op_nip(Stack *stack) { (void)stack; (void)(stack_nip(stack)); return 0; }
static int op_tuck(Stack *stack) { (void)stack; (void)(stack_tuck(stack)); return 0; }
static int op_clst(Stack *stack) { (void)stack; (void)(free_stack(stack)); return 0; }
static int op_savestack(Stack *stack) { (void)stack; (void)(save_stack_to_file(stack, STACK_PATH)); return 0; }
static int op_loadstack(Stack *stack) { (void)stack; (void)(load_stack_from_file(stack, STACK_PATH)); return 0; }
static int op_scon(Stack *stack) { (void)stack; (void)(concatenate(stack)); return 0; }
static int op_substr(Stack *stack) { (void)stack; (void)(my_substring(stack)); return 0; }
static int op_s2l(Stack *stack) { (void)stack; (void)(to_lower(stack)); return 0; }
static int op_s2u(Stack *stack) { (void)stack; (void)(to_upper(stack)); return 0; }
static int op_slen(Stack *stack) { (void)stack; (void)(string_length(stack)); return 0; }
static int op_srev(Stack *stack) { (void)stack; (void)(string_reverse(stack)); return 0; }
static int op_int2str(Stack *stack) { (void)stack; (void)(top_to_string(stack)); return 0; }
static int op_tran(Stack *stack) { (void)stack; (void)(matrix_transpose(stack)); return 0; }
static int op_tick_tran(Stack *stack) { (void)stack; (void)(matrix_transpose(stack)); return 0; }
static int op_minv(Stack *stack) { (void)stack; (void)(matrix_inverse(stack)); return 0; }
static int op_pinv(Stack *stack) { (void)stack; (void)(matrix_pseudoinverse(stack)); return 0; }
static int op_det(Stack *stack) { (void)stack; (void)(matrix_determinant(stack)); return 0; }
static int op_eig(Stack *stack) { (void)stack; (void)(matrix_eigen_decompose(stack)); return 0; }
static int op_chol(Stack *stack) { (void)stack; (void)(matrix_cholesky(stack)); return 0; }
static int op_svd(Stack *stack) { (void)stack; (void)(matrix_svd(stack)); return 0; }
static int op_kron(Stack *stack) { (void)stack; (void)(kronecker_top_two(stack)); return 0; }
static int op_dim(Stack *stack) { (void)stack; (void)(matrix_dimensions(stack)); return 0; }
static int op_reshape(Stack *stack) { (void)stack; (void)(reshape_matrix(stack)); return 0; }
static int op_get_aij(Stack *stack) { (void)stack; (void)(select_matrix_element(stack)); return 0; }
static int op_set_aij(Stack *stack) { (void)stack; (void)(set_matrix_element(stack)); return 0; }
static int op_split_mat(Stack *stack) { (void)stack; (void)(split_matrix(stack)); return 0; }
static int op_diag(Stack *stack) { (void)stack; (void)(matrix_extract_diagonal(stack)); return 0; }
static int op_to_diag(Stack *stack) { (void)stack; (void)(make_diag_matrix(stack)); return 0; }
static int op_eye(Stack *stack) { (void)stack; (void)(make_unit_matrix(stack)); return 0; }
static int op_ones(Stack *stack) { (void)stack; (void)(make_matrix_of_ones(stack)); return 0; }
static int op_zeroes(Stack *stack) { (void)stack; (void)(make_matrix_of_zeroes(stack)); return 0; }
static int op_rand(Stack *stack) { (void)stack; (void)(make_random_matrix(stack)); return 0; }
static int op_randn(Stack *stack) { (void)stack; (void)(make_gaussian_random_matrix(stack)); return 0; }
static int op_rrange(Stack *stack) { (void)stack; (void)(make_row_range(stack)); return 0; }
static int op_join_v(Stack *stack) { (void)stack; (void)(stack_join_matrix_vertical(stack)); return 0; }
static int op_join_h(Stack *stack) { (void)stack; (void)(stack_join_matrix_horizontal(stack)); return 0; }
static int op_cumsum_r(Stack *stack) { (void)stack; (void)(matrix_cumsum_rows(stack)); return 0; }
static int op_cumsum_c(Stack *stack) { (void)stack; (void)(matrix_cumsum_cols(stack)); return 0; }
static int op_cmean(Stack *stack) { (void)stack; (void)(matrix_reduce(stack, "col", "mean")); return 0; }
static int op_rmean(Stack *stack) { (void)stack; (void)(matrix_reduce(stack, "row", "mean")); return 0; }
static int op_csum(Stack *stack) { (void)stack; (void)(matrix_reduce(stack, "col", "sum")); return 0; }
static int op_rsum(Stack *stack) { (void)stack; (void)(matrix_reduce(stack, "row", "sum")); return 0; }
static int op_cvar(Stack *stack) { (void)stack; (void)(matrix_reduce(stack, "col", "var")); return 0; }
static int op_rvar(Stack *stack) { (void)stack; (void)(matrix_reduce(stack, "row", "var")); return 0; }
static int op_cmin(Stack *stack) { (void)stack; (void)(matrix_reduce(stack, "col", "min")); return 0; }
static int op_rmin(Stack *stack) { (void)stack; (void)(matrix_reduce(stack, "row", "min")); return 0; }
static int op_cmax(Stack *stack) { (void)stack; (void)(matrix_reduce(stack, "col", "max")); return 0; }
static int op_rmax(Stack *stack) { (void)stack; (void)(matrix_reduce(stack, "row", "max")); return 0; }
static int op_roots(Stack *stack) { (void)stack; (void)(poly_roots(stack)); return 0; }
static int op_pval(Stack *stack) { (void)stack; (void)(poly_eval(stack)); return 0; }
static int op_sto(Stack *stack) { (void)stack; (void)(store_to_register(stack)); return 0; }
static int op_rcl(Stack *stack) { (void)stack; (void)(recall_from_register(stack)); return 0; }
static int op_ffr(Stack *stack) { (void)stack; (void)(find_first_free_register(stack)); return 0; }
static int op_pr(Stack *stack) { (void)stack; (void)(show_registers_status()); return 0; }
static int op_saveregs(Stack *stack) { (void)stack; (void)(save_registers_to_file(REGISTERS_PATH)); return 0; }
static int op_loadregs(Stack *stack) { (void)stack; (void)(load_registers_from_file(REGISTERS_PATH)); return 0; }
static int op_clregs(Stack *stack) { (void)stack; (void)(free_all_registers()); return 0; }
static int op_today(Stack *stack) { (void)stack; (void)(push_today_date(stack)); return 0; }
static int op_now(Stack *stack) { (void)stack; (void)(push_now_time(stack)); return 0; }
static int op_tz_offset(Stack *stack) { (void)stack; (void)(tz_offset(stack)); return 0; }
static int op_ddays(Stack *stack) { (void)stack; (void)(delta_days_strings(stack)); return 0; }
static int op_dateplus(Stack *stack) { (void)stack; (void)(date_plus_days(stack)); return 0; }
static int op_dow(Stack *stack) { (void)stack; (void)(push_weekday_name_from_date_string(stack)); return 0; }
static int op_edmy(Stack *stack) { (void)stack; (void)(extract_day_month_year(stack)); return 0; }
static int op_num2date(Stack *stack) { (void)stack; (void)(make_date_string(stack)); return 0; }
static int op_days2eoy(Stack *stack) { (void)stack; (void)(days_to_end_of_year(stack)); return 0; }
static int op_sunrise(Stack *stack) { (void)stack; (void)(sunrise(stack)); return 0; }
static int op_sunset(Stack *stack) { (void)stack; (void)(sunset(stack)); return 0; }
static int op_dawn(Stack *stack) { (void)stack; (void)(dawn(stack)); return 0; }
static int op_dusk(Stack *stack) { (void)stack; (void)(dusk(stack)); return 0; }
static int op_print(Stack *stack) { (void)stack; (void)(print_top_scalar(stack)); return 0; }
static int op_pm(Stack *stack) { print_matrix(stack); skip_stack_printing = true; return 0; }
static int op_ps(Stack *stack) { (void)stack; (void)(print_stack(stack, NULL)); return 0; }
static int op_setprec(Stack *stack) { (void)stack; (void)(set_print_precision(stack)); return 0; }
static int op_sfs(Stack *stack) { (void)stack; (void)(swap_fixed_scientific()); return 0; }
static int op_listwords(Stack *stack) { (void)stack; (void)(list_words()); return 0; }
static int op_listmacros(Stack *stack) { (void)stack; (void)(list_macros()); return 0; }
static int op_loadwords(Stack *stack) { (void)stack; (void)(load_words_from_file()); return 0; }
static int op_savewords(Stack *stack) { (void)stack; (void)(save_words_to_file()); return 0; }
static int op_delword(Stack *stack) { (void)stack; (void)(delete_word(stack)); return 0; }
static int op_clrwords(Stack *stack) { (void)stack; (void)(clear_words()); return 0; }
static int op_help(Stack *stack) { (void)stack; (void)(help_menu()); return 0; }
static int op_listfcns(Stack *stack) { (void)stack; (void)(list_all_functions_sorted()); return 0; }
static int op_clrhist(Stack *stack) { (void)stack; (void)(clear_history()); return 0; }
static int op_fuck(Stack *stack) { (void)stack; (void)(whose_place()); return 0; }

const Op ops[] = {
  /* ---- Arithmetic ---- */
  { "+", 2, op_add, "Arithmetic", "y x -- y+x", "add scalars or matrices; a scalar added to a matrix is added elementwise", "3 4 +" },
  { "-", 2, op_sub, "Arithmetic", "y x -- y-x", "subtract scalars or matrices (a scalar elementwise)", "10 4 -" },
  { "*", 2, op_mul, "Arithmetic", "y x -- y*x", "multiply scalars, or matrix product for matrices", "[2 2 $ 1 2 3 4] [2 1 $ 1 1] *" },
  { "/", 2, op_div, "Arithmetic", "y x -- y/x", "divide scalars, or a matrix by a scalar; division by zero is an error", "1 3 /" },
  { "^", 2, op_pow_, "Arithmetic", "y x -- y^x", "power; a square matrix to an integer power is a matrix power; a negative base with a fractional exponent promotes to complex", "2 10 ^" },
  { "pow", 2, op_pow, "Arithmetic", "y x -- y^x", "power (alias of ^)", "2 10 pow" },
  { ".*", 2, op_dot_mul, "Arithmetic", "A B -- A.*B", "elementwise matrix product", "[2 2 $ 1 2 3 4] [2 2 $ 1 1 2 2] .*" },
  { "./", 2, op_dot_div, "Arithmetic", "A B -- A./B", "elementwise division (matrix or scalar operands)", "[2 2 $ 1 2 3 4] [2 2 $ 1 2 1 2] ./" },
  { ".^", 2, op_dot_pow, "Arithmetic", "A B -- A.^B", "elementwise power (matrix or scalar operands)", "[2 2 $ 1 2 3 4] 2 .^" },
  { "chs", 1, op_chs, "Arithmetic", "x -- -x", "change sign (elementwise for matrices)", "5 chs" },
  { "inv", 1, op_inv, "Arithmetic", "x -- 1/x", "reciprocal (elementwise for matrices; see minv for the matrix inverse)", "4 inv" },
  { "abs", 1, op_abs, "Arithmetic", "z -- |z|", "absolute value; magnitude for complex; elementwise for matrices", "(3,4) abs" },
  { "sqrt", 1, op_sqrt, "Arithmetic", "x -- sqrt(x)", "square root; a negative real promotes to complex", "-4 sqrt" },
  { "%", 2, op_pct, "Arithmetic", "y x -- y y*x/100", "percent of y (HP %; keeps y)", "200 25 %" },
  { "%t", 2, op_pctt, "Arithmetic", "y x -- y 100*x/y", "x as a percent of the total y (HP %T; keeps y)", "200 50 %t" },
  { "%chg", 2, op_pctchg, "Arithmetic", "y x -- y 100*(x-y)/y", "percent change from y to x (HP delta%; keeps y)", "80 100 %chg" },
  { "mod", 2, op_mod, "Arithmetic", "y x -- y mod x", "floored modulo (result has the sign of x)", "-7 3 mod" },
  { "fact", 1, op_fact, "Arithmetic", "x -- x!", "factorial; Gamma(x+1) for non-integers", "5 fact" },
  { "ncr", 2, op_ncr, "Arithmetic", "n k -- C(n,k)", "combinations", "52 5 ncr" },
  { "npr", 2, op_npr, "Arithmetic", "n k -- P(n,k)", "permutations", "10 3 npr" },
  { "frac", 1, op_frac, "Arithmetic", "x -- frac(x)", "fractional part, x - intg(x), with the sign of x", "3.7 frac" },
  { "intg", 1, op_intg, "Arithmetic", "x -- int(x)", "integer part, truncated toward zero as on HP calculators (-3.7 gives -3)", "3.7 intg" },
  /* ---- Exp/Log ---- */
  { "exp", 1, op_exp, "Exp/Log", "x -- e^x", "exponential (elementwise for matrices)", "1 exp" },
  { "ln", 1, op_ln, "Exp/Log", "x -- ln(x)", "natural logarithm; a negative real promotes to complex", "10 ln" },
  { "log", 1, op_log, "Exp/Log", "x -- log10(x)", "base-10 logarithm; a negative real promotes to complex", "100 log" },
  /* ---- Trigonometry ---- */
  { "sin", 1, op_sin, "Trigonometry", "z -- sin(z)", "sine of a real or complex argument (radians); elementwise for matrices", "0.5 sin" },
  { "cos", 1, op_cos, "Trigonometry", "z -- cos(z)", "cosine of real or complex argument", "1 cos" },
  { "tan", 1, op_tan, "Trigonometry", "z -- tan(z)", "tangent of real or complex argument", "0.3 tan" },
  { "asin", 1, op_asin, "Trigonometry", "z -- asin(z)", "inverse sine (principal value)", "0.5 asin" },
  { "acos", 1, op_acos, "Trigonometry", "z -- acos(z)", "inverse cosine (principal value)", "0.5 acos" },
  { "atan", 1, op_atan, "Trigonometry", "z -- atan(z)", "inverse tangent (principal value)", "1 atan" },
  { "sinh", 1, op_sinh, "Trigonometry", "z -- sinh(z)", "hyperbolic sine", "1 sinh" },
  { "cosh", 1, op_cosh, "Trigonometry", "z -- cosh(z)", "hyperbolic cosine", "1 cosh" },
  { "tanh", 1, op_tanh, "Trigonometry", "z -- tanh(z)", "hyperbolic tangent", "1 tanh" },
  { "asinh", 1, op_asinh, "Trigonometry", "z -- asinh(z)", "inverse hyperbolic sine", "1 asinh" },
  { "acosh", 1, op_acosh, "Trigonometry", "z -- acosh(z)", "inverse hyperbolic cosine", "2 acosh" },
  { "atanh", 1, op_atanh, "Trigonometry", "z -- atanh(z)", "inverse hyperbolic tangent", "0.5 atanh" },
  /* ---- Complex ---- */
  { "re", 1, op_re, "Complex", "z -- Re(z)", "real part (a real is unchanged)", "(1,2) re" },
  { "im", 1, op_im, "Complex", "z -- Im(z)", "imaginary part (0 for a real)", "(1,2) im" },
  { "arg", 1, op_arg, "Complex", "z -- arg(z)", "argument (phase) in radians, in (-pi, pi]", "(0,1) arg" },
  { "conj", 1, op_conj, "Complex", "z -- conj(z)", "complex conjugate (elementwise for matrices; identity for reals)", "(1,2) conj" },
  { "re2c", 1, op_re2c, "Complex", "x -- (x,0)", "promote a real to complex", "5 re2c" },
  { "split_c", 1, op_split_c, "Complex", "z -- Re(z) Im(z)", "split a complex number into real and imaginary parts", "(1,2) split_c" },
  { "j2r", 2, op_j2r, "Complex", "re im -- (re,im)", "join two reals into a complex number (imaginary part on top)", "1 2 j2r" },
  /* ---- Special functions ---- */
  { "npdf", 1, op_npdf, "Special functions", "x -- phi(x)", "standard normal density", "0 npdf" },
  { "ncdf", 1, op_ncdf, "Special functions", "x -- Phi(x)", "standard normal cumulative distribution", "1.96 ncdf" },
  { "nquant", 1, op_nquant, "Special functions", "p -- x", "standard normal quantile (inverse of ncdf), 0 < p < 1", "0.975 nquant" },
  { "gamma", 1, op_gamma, "Special functions", "x -- Gamma(x)", "gamma function", "5 gamma" },
  { "ln_gamma", 1, op_ln_gamma, "Special functions", "x -- ln Γ(x)", "log gamma function", "10 ln_gamma" },
  { "beta", 2, op_beta, "Special functions", "a b -- B(a,b)", "beta function", "2 3 beta" },
  { "ln_beta", 2, op_ln_beta, "Special functions", "a b -- ln B(a,b)", "log beta function", "2 3 ln_beta" },
  /* ---- Constants ---- */
  { "pi", 0, op_pi, "Constants", "-- π", "push π", "pi" },
  { "e", 0, op_e, "Constants", "-- e", "push Euler's number", "e" },
  { "gravity", 0, op_gravity, "Constants", "-- g", "standard gravity, 9.80665 m/s^2", "gravity" },
  { "inf", 0, op_inf, "Constants", "-- +∞", "positive infinity", "inf" },
  { "nan", 0, op_nan, "Constants", "-- NaN", "not-a-number", "nan" },
  /* ---- Logic ---- */
  { "eq", 2, op_eq, "Logic", "a b -- flag", "1 if a equals b, else 0; numbers, strings, or elementwise for matrices", "2 2 eq" },
  { "neq", 2, op_neq, "Logic", "a b -- flag", "1 if a differs from b; numbers, strings, or elementwise for matrices", "2 3 neq" },
  { "lt", 2, op_lt, "Logic", "a b -- flag", "1 if a < b (complex numbers compare by magnitude); elementwise for matrices", "2 3 lt" },
  { "leq", 2, op_leq, "Logic", "a b -- flag", "1 if a <= b; elementwise for matrices", "2 3 leq" },
  { "gt", 2, op_gt, "Logic", "a b -- flag", "1 if a > b; elementwise for matrices", "3 2 gt" },
  { "geq", 2, op_geq, "Logic", "a b -- flag", "1 if a >= b; elementwise for matrices", "3 2 geq" },
  { "and", 2, op_and, "Logic", "a b -- flag", "logical and (0 is false, anything else true)", "1 0 and" },
  { "or", 2, op_or, "Logic", "a b -- flag", "logical or", "1 0 or" },
  { "not", 1, op_not, "Logic", "a -- flag", "logical not", "0 not" },
  /* ---- Stack ---- */
  { "dup", 1, op_dup, "Stack", "x -- x x", "duplicate top of stack", "5 dup" },
  { "drop", 1, op_drop, "Stack", "x --", "drop the top stack element", "1 2 drop" },
  { "swap", 2, op_swap, "Stack", "a b -- b a", "swap top two elements", "1 2 swap" },
  { "over", 2, op_over, "Stack", "a b -- a b a", "copy second item to top", "1 2 over" },
  { "nip", 2, op_nip, "Stack", "a b -- b", "drop second-from-top element (keep top)", "1 2 nip" },
  { "tuck", 2, op_tuck, "Stack", "a b -- b a b", "duplicate second item and tuck below top", "1 2 tuck" },
  { "roll", 1, op_roll_n, "Stack", "... n -- ...", "move the item at depth n to the top (1 roll = swap, 2 roll = rot)", "1 2 3 2 roll" },
  { "pick", 1, op_pick, "Stack", "... n -- ... v", "copy the item at depth n to the top (0 pick = dup)", "1 2 3 2 pick" },
  { "depth", 0, op_depth, "Stack", "-- n", "number of items on the stack", "depth" },
  { "clst", 0, op_clst, "Stack", "... --", "clear the stack", "clst" },
  { "savestack", 0, op_savestack, "Stack", "--", "save the whole stack to stack_backup.dat in the config directory", "savestack" },
  { "loadstack", 0, op_loadstack, "Stack", "-- ...", "push the stack saved by savestack", "loadstack" },
  /* ---- Strings ---- */
  { "scon", 2, op_scon, "Strings", "s1 s2 -- s1s2", "concatenate two strings", "\"foo\" \"bar\" scon" },
  { "substr", 3, op_substr, "Strings", "s start end -- sub", "substring from index start (0-based) up to end (exclusive)", "\"abcdef\" 1 3 substr" },
  { "s2l", 1, op_s2l, "Strings", "s -- s_lower", "convert string to lowercase", "\"Hello\" s2l" },
  { "s2u", 1, op_s2u, "Strings", "s -- s_upper", "convert string to uppercase", "\"Hello\" s2u" },
  { "slen", 1, op_slen, "Strings", "s -- n", "length of a string in bytes", "\"hello\" slen" },
  { "srev", 1, op_srev, "Strings", "s -- s_rev", "reverse string", "\"abc\" srev" },
  { "int2str", 1, op_int2str, "Strings", "x -- s", "integer part of x as a string", "123 int2str" },
  /* ---- Matrices ---- */
  { "tran", 1, op_tran, "Matrices", "A -- A^T", "transpose", "[2 3 $ 1 2 3 4 5 6] tran" },
  { "'", 1, op_tick_tran, "Matrices", "A -- A'", "transpose (alias of tran)", "[2 3 $ 1 2 3 4 5 6] '" },
  { "minv", 1, op_minv, "Matrices", "A -- A^-1", "inverse of a square real or complex matrix (error if singular)", "[2 2 $ 4 7 2 6] minv" },
  { "pinv", 1, op_pinv, "Matrices", "A -- A^+", "Moore-Penrose pseudoinverse of a square real matrix (SVD)", "[2 2 $ 1 2 2 4] pinv" },
  { "det", 1, op_det, "Matrices", "A -- det(A)", "determinant of a square real or complex matrix", "[2 2 $ 4 7 2 6] det" },
  { "eig", 1, op_eig, "Matrices", "A -- V L", "eigenvectors (columns of V) and eigenvalues (diagonal of L) of a square real matrix; both complex", "[2 2 $ 2 1 1 2] eig" },
  { "chol", 1, op_chol, "Matrices", "A -- L", "Cholesky factor of a symmetric positive definite real matrix, A = L L^T", "[2 2 $ 4 2 2 3] chol" },
  { "svd", 1, op_svd, "Matrices", "A -- U S V", "singular value decomposition of a real matrix, A = U S V^T", "[2 2 $ 4 7 2 6] svd" },
  { "kron", 2, op_kron, "Matrices", "A B -- A(x)B", "Kronecker product", "[2 2 $ 1 2 3 4] [1 2 $ 5 6] kron" },
  { "dim", 1, op_dim, "Matrices", "A -- A rows cols", "push the dimensions (the matrix stays)", "[2 3 $ 1 2 3 4 5 6] dim" },
  { "reshape", 3, op_reshape, "Matrices", "A rows cols -- B", "reshape in row-major order; rows*cols must match", "[2 3 $ 1 2 3 4 5 6] 3 2 reshape" },
  { "get_aij", 3, op_get_aij, "Matrices", "A i j -- A a_ij", "push element (i,j), 0-based (the matrix stays)", "[2 2 $ 1 2 3 4] 0 1 get_aij" },
  { "set_aij", 4, op_set_aij, "Matrices", "A i j x -- A", "set element (i,j), 0-based, in place; a complex x promotes a real matrix", "[2 2 $ 1 2 3 4] 0 1 9 set_aij" },
  { "split_mat", 1, op_split_mat, "Matrices", "A -- a11 a12 ...", "push all elements as scalars, row by row", "[2 2 $ 1 2 3 4] split_mat" },
  { "diag", 1, op_diag, "Matrices", "A -- v", "main diagonal as a row vector", "[2 2 $ 1 2 3 4] diag" },
  { "to_diag", 1, op_to_diag, "Matrices", "v -- D", "diagonal matrix from a row or column vector", "[1 3 $ 1 2 3] to_diag" },
  { "eye", 1, op_eye, "Matrices", "n -- I", "n x n identity matrix", "3 eye" },
  { "ones", 2, op_ones, "Matrices", "rows cols -- A", "matrix filled with ones", "2 3 ones" },
  { "zeroes", 2, op_zeroes, "Matrices", "rows cols -- A", "matrix filled with zeros", "2 3 zeroes" },
  { "rand", 2, op_rand, "Matrices", "rows cols -- A", "matrix of uniform(0,1) randoms", "2 2 rand" },
  { "randn", 2, op_randn, "Matrices", "rows cols -- A", "matrix of standard normal randoms", "2 2 randn" },
  { "rrange", 3, op_rrange, "Matrices", "start step end -- v", "row vector start, start+step, ... up to end", "0 0.25 1 rrange" },
  { "join_v", 2, op_join_v, "Matrices", "A B -- [A; B]", "stack two matrices vertically (same number of columns)", "[1 2 $ 1 2] [1 2 $ 3 4] join_v" },
  { "join_h", 2, op_join_h, "Matrices", "A B -- [A B]", "join two matrices side by side (same number of rows)", "[2 1 $ 1 2] [2 1 $ 3 4] join_h" },
  { "cumsum_r", 1, op_cumsum_r, "Matrices", "A -- B", "cumulative sum along each row", "[2 2 $ 1 2 3 4] cumsum_r" },
  { "cumsum_c", 1, op_cumsum_c, "Matrices", "A -- B", "cumulative sum down each column", "[2 2 $ 1 2 3 4] cumsum_c" },
  { "cmean", 1, op_cmean, "Matrices", "A -- v", "mean of each column (row vector)", "[2 2 $ 1 2 3 4] cmean" },
  { "rmean", 1, op_rmean, "Matrices", "A -- v", "mean of each row (column vector)", "[2 2 $ 1 2 3 4] rmean" },
  { "csum", 1, op_csum, "Matrices", "A -- v", "sum of each column (row vector)", "[2 2 $ 1 2 3 4] csum" },
  { "rsum", 1, op_rsum, "Matrices", "A -- v", "sum of each row (column vector)", "[2 2 $ 1 2 3 4] rsum" },
  { "cvar", 1, op_cvar, "Matrices", "A -- v", "sample variance of each column (row vector)", "[2 2 $ 1 2 3 4] cvar" },
  { "rvar", 1, op_rvar, "Matrices", "A -- v", "sample variance of each row (column vector)", "[2 2 $ 1 2 3 4] rvar" },
  { "cmin", 1, op_cmin, "Matrices", "A -- v", "minimum of each column", "[2 2 $ 1 2 3 4] cmin" },
  { "rmin", 1, op_rmin, "Matrices", "A -- v", "minimum of each row", "[2 2 $ 1 2 3 4] rmin" },
  { "cmax", 1, op_cmax, "Matrices", "A -- v", "maximum of each column", "[2 2 $ 1 2 3 4] cmax" },
  { "rmax", 1, op_rmax, "Matrices", "A -- v", "maximum of each row", "[2 2 $ 1 2 3 4] rmax" },
  /* ---- Polynomials ---- */
  { "roots", 1, op_roots, "Polynomials", "coeffs -- roots", "roots of a real polynomial; coefficients highest degree first, result is a complex row vector", "[1 3 $ 1 -3 2] roots" },
  { "pval", 2, op_pval, "Polynomials", "coeffs x -- y", "evaluate a polynomial (coefficients highest degree first, real or complex) at x", "[1 3 $ 1 0 -1] 2 pval" },
  /* ---- Numerics ---- */
  { "exec", 1, op_exec, "Numerics", "xt --", "run an execution token (@name pushes one)", "3 @sq exec" },
  { "integrate", 3, op_integrate, "Numerics", "a b xt -- I", "integral of f over [a,b] (tanh-sinh with adaptive bisection; handles endpoint singularities)", "0 pi @sin integrate" },
  { "romberg", 3, op_romberg, "Numerics", "a b xt -- I", "integral of f over [a,b] by Romberg extrapolation", "0 1 @exp romberg" },
  { "fzero", 3, op_fzero, "Numerics", "a b xt -- x", "root of f in the bracket [a,b] where f changes sign (Brent)", "2 4 @sin fzero" },
  { "solve", 2, op_solve, "Numerics", "guess xt -- x", "root of f near one guess (bracket search, then Brent)", "3 @sin solve" },
  { "minimize", 3, op_minimize, "Numerics", "a b xt -- x", "argument of the minimum of f in [a,b] (Brent)", "0 5 @par minimize" },
  { "deriv", 2, op_deriv, "Numerics", "x xt -- f'(x)", "derivative of f at x (Ridders extrapolation)", "2 @exp deriv" },
  { "fsum", 3, op_fsum, "Numerics", "lo hi xt -- S", "sum of f(i) for integer i from lo to hi", "1 1000 @invsq fsum" },
  { "fprod", 3, op_fprod, "Numerics", "lo hi xt -- P", "product of f(i) for integer i from lo to hi", "1 10 @ident fprod" },
  { "tol", 1, op_tol, "Numerics", "x --", "set the convergence tolerance (default 1e-10)", "1e-6 tol" },
  { "nevals", 0, op_nevals, "Numerics", "-- n", "function evaluations used by the last numerical call", "nevals" },
  { "interr", 0, op_interr, "Numerics", "-- e", "error estimate of the last integrate", "interr" },
  /* ---- Registers ---- */
  { "sto", 2, op_sto, "Registers", "x n --", "store x in register n (0-63); any type", "42 0 sto" },
  { "rcl", 1, op_rcl, "Registers", "n -- x", "push a copy of register n", "0 rcl" },
  { "ffr", 0, op_ffr, "Registers", "-- n", "index of the first free register", "ffr" },
  { "pr", 0, op_pr, "Registers", "--", "show which registers are occupied", "pr" },
  { "saveregs", 0, op_saveregs, "Registers", "--", "save all registers to registers.txt in the config directory", "saveregs" },
  { "loadregs", 0, op_loadregs, "Registers", "--", "load the registers saved by saveregs", "loadregs" },
  { "clregs", 0, op_clregs, "Registers", "--", "clear all registers", "clregs" },
  /* ---- Dates ---- */
  { "today", 0, op_today, "Dates", "-- date", "today's date as a string d.m.yyyy", "today" },
  { "now", 0, op_now, "Dates", "-- \"HH:MM\"", "push the current local time as a string, same form as sunrise/sunset", "now" },
  { "tz_offset", 2, op_tz_offset, "Dates", "date zone -- hours", "UTC offset in hours of an IANA zone on the given date, DST included", "\"1.12.2026\" \"America/New_York\" tz_offset" },
  { "ddays", 2, op_ddays, "Dates", "d1 d2 -- n", "days from date d1 to date d2 (d2 - d1)", "\"1.1.2025\" \"31.12.2025\" ddays" },
  { "dateplus", 2, op_dateplus, "Dates", "date n -- date2", "the date n days later (n may be negative)", "\"1.1.2025\" 30 dateplus" },
  { "dow", 1, op_dow, "Dates", "date -- name", "weekday name of a date", "\"1.1.2025\" dow" },
  { "edmy", 1, op_edmy, "Dates", "date -- y m d", "split a date into year, month, day (day on top)", "\"1.1.2025\" edmy" },
  { "num2date", 3, op_num2date, "Dates", "d m y -- date", "build a date string from day, month, year", "1 1 2025 num2date" },
  { "days2eoy", 1, op_days2eoy, "Dates", "date -- n", "days from the date to the end of its year", "\"1.1.2025\" days2eoy" },
  { "sunrise", 4, op_sunrise, "Dates", "date lat lon zone -- time", "local sunrise HH:MM; zone is an IANA name or a UTC offset in hours", "\"21.6.2025\" 38.9 -77.0 \"America/New_York\" sunrise" },
  { "sunset", 4, op_sunset, "Dates", "date lat lon zone -- time", "local sunset HH:MM", "\"21.6.2025\" 38.9 -77.0 -4 sunset" },
  { "dawn", 4, op_dawn, "Dates", "date lat lon zone -- time", "civil dawn HH:MM (sun 6 degrees below the horizon)", "today 38.9 -77.0 \"America/New_York\" dawn" },
  { "dusk", 4, op_dusk, "Dates", "date lat lon zone -- time", "civil dusk HH:MM", "today 38.9 -77.0 \"America/New_York\" dusk" },
  /* ---- Display ---- */
  { "print", 0, op_print, "Display", "x -- x", "print the top of the stack (a matrix in full)", "42 print" },
  { "pm", 1, op_pm, "Display", "A -- A", "print the matrix on top of the stack in full", "[2 2 $ 1 2 3 4] pm" },
  { "ps", 0, op_ps, "Display", "--", "print the whole stack", "ps" },
  { "setprec", 1, op_setprec, "Display", "n --", "digits after the decimal point (fixed) or significant digits (scientific), 0-20", "10 setprec" },
  { "sfs", 0, op_sfs, "Display", "--", "toggle between fixed-point and scientific display", "sfs" },
  /* ---- Words ---- */
  { "listwords", 0, op_listwords, "Words", "--", "list the user-defined words", "listwords" },
  { "listmacros", 0, op_listmacros, "Words", "--", "list the macros loaded from predefined_macros.txt", "listmacros" },
  { "loadwords", 0, op_loadwords, "Words", "--", "load the words saved in user_words.txt (also done at startup)", "loadwords" },
  { "savewords", 0, op_savewords, "Words", "--", "save all user-defined words to user_words.txt in the config directory", "savewords" },
  { "delword", 1, op_delword, "Words", "\"name\" --", "delete a user-defined word", "\"sq\" delword" },
  { "clrwords", 0, op_clrwords, "Words", "--", "delete all user-defined words", "clrwords" },
  /* ---- Programs ---- */
  { "eval", 1, op_eval, "Programs", "s --", "evaluate the string s as a line of input", "\"1 2 +\" eval" },
  { "batch", 1, op_batch, "Programs", "s --", "run the lines of file s as input", "\"calc.txt\" batch" },
  { "run", 1, op_run, "Programs", "s --", "load and run the HP-41 style program in file s (LBL/GOTO/GOSUB/RTN/END)", "\"prog1.prg\" run" },
  /* ---- Session ---- */
  { "help", 0, op_help, "Session", "--", "print the quick-start help screen", "help" },
  { "usage", 1, op_usage, "Session", "\"name\" --", "show the stack picture, description and example of a command, or a word's definition", "\"sin\" usage" },
  { "listfcns", 0, op_listfcns, "Session", "--", "list all commands by category", "listfcns" },
  { "clrhist", 0, op_clrhist, "Session", "--", "clear the readline history", "clrhist" },
  { "fuck", 0, op_fuck, "Session", "--", "an Easter egg", "fuck" },
};
const int n_ops = (int)(sizeof ops / sizeof ops[0]);

int find_op(const char *name) {
  for (int i = 0; i < n_ops; i++)
    if (strcmp(ops[i].name, name) == 0) return i;
  return -1;
}

int call_op(Stack *stack, int idx) {
  const Op *op = &ops[idx];
  if (stack->top + 1 < op->nargs)
    return mm_error("%s: needs %d item%s on the stack (%s)", op->name, op->nargs,
                    op->nargs == 1 ? "" : "s", op->effect);
  return op->fn(stack);
}

static int categories(const char **out, int max) {
  int n = 0;
  for (int i = 0; i < n_ops; i++) {
    int seen = 0;
    for (int k = 0; k < n; k++) if (strcmp(out[k], ops[i].cat) == 0) { seen = 1; break; }
    if (!seen && n < max) out[n++] = ops[i].cat;
  }
  return n;
}

void list_ops_by_category(void) {
  const char *cats[32];
  int nc = categories(cats, 32);
  for (int c = 0; c < nc; c++) {
    printf("  %-18s", cats[c]);
    int col = 20;
    for (int i = 0; i < n_ops; i++) {
      if (strcmp(ops[i].cat, cats[c]) != 0) continue;
      int l = (int)strlen(ops[i].name) + 1;
      if (col + l > 78) { printf("\n  %-18s", ""); col = 20; }
      printf(" %s", ops[i].name);
      col += l;
    }
    printf("\n");
  }
  printf("'usage \"name\"' describes one command; user words: listwords.\n");
}

static void md(const char *s) {
  for (; *s; s++) { if (*s == '|') fputs("\\|", stdout); else putchar(*s); }
}

/* LaTeX: everything goes inside \texttt{} with the special characters escaped. */
static void tex(const char *s) {
  for (; *s; s++) {
    switch (*s) {
    case '\\': fputs("\\textbackslash{}", stdout); break;
    case '{': fputs("\\{", stdout); break;
    case '}': fputs("\\}", stdout); break;
    case '$': case '&': case '%': case '#': case '_': printf("\\%c", *s); break;
    case '^': fputs("\\^{}", stdout); break;
    case '~': fputs("\\~{}", stdout); break;
    case '|': fputs("\\textbar{}", stdout); break;
    case '<': fputs("\\textless{}", stdout); break;
    case '>': fputs("\\textgreater{}", stdout); break;
    case '"': fputs("\\textquotedbl{}", stdout); break;
    case '\'': fputs("\\textquotesingle{}", stdout); break;
    case '-': fputs("-{}", stdout); break;        /* no ligatures: -- stays -- */
    default: putchar(*s);
    }
  }
}

void print_ops_latex(void) {
  const char *cats[32];
  int nc = categories(cats, 32);
  printf("%% Generated by mm_15 --commands-tex from the dispatch table; do not edit by hand.\n");
  for (int c = 0; c < nc; c++) {
    printf("\\subsection*{%s}\n", cats[c]);
    printf("\\begin{longtable}{@{}>{\\raggedright\\arraybackslash}p{0.12\\textwidth}>{\\raggedright\\arraybackslash}p{0.19\\textwidth}>{\\raggedright\\arraybackslash}p{0.36\\textwidth}>{\\raggedright\\arraybackslash}p{0.23\\textwidth}@{}}\n");
    printf("\\toprule \\textbf{Command} & \\textbf{Stack} & \\textbf{Description} & \\textbf{Example}\\\\ \\midrule \\endfirsthead\n");
    printf("\\toprule \\textbf{Command} & \\textbf{Stack} & \\textbf{Description} & \\textbf{Example}\\\\ \\midrule \\endhead\n");
    printf("\\bottomrule \\endlastfoot\n");
    for (int i = 0; i < n_ops; i++) {
      if (strcmp(ops[i].cat, cats[c]) != 0) continue;
      printf("\\texttt{"); tex(ops[i].name); printf("} & ");
      if (ops[i].effect[0]) { printf("\\texttt{"); tex(ops[i].effect); printf("}"); }
      printf(" & "); tex(ops[i].help); printf(" & ");
      if (ops[i].example[0]) { printf("\\texttt{"); tex(ops[i].example); printf("}"); }
      printf("\\\\\n");
    }
    printf("\\end{longtable}\n\n");
  }
}

void print_ops_markdown(void) {
  const char *cats[32];
  int nc = categories(cats, 32);
  printf("# MM-15 command reference\n\n");
  printf("Generated by `mm_15 --commands` from the dispatch table; do not edit by hand.\n");
  printf("Stack pictures read deepest-first: `y x -- y+x` consumes Y and X and leaves the sum in X.\n");
  for (int c = 0; c < nc; c++) {
    printf("\n## %s\n\n| Command | Stack | Description | Example |\n|---|---|---|---|\n", cats[c]);
    for (int i = 0; i < n_ops; i++) {
      if (strcmp(ops[i].cat, cats[c]) != 0) continue;
      printf("| `"); md(ops[i].name); printf("` | ");
      if (ops[i].effect[0]) { printf("`"); md(ops[i].effect); printf("`"); }
      printf(" | "); md(ops[i].help); printf(" | ");
      if (ops[i].example[0]) { printf("`"); md(ops[i].example); printf("`"); }
      printf(" |\n");
    }
  }
}
