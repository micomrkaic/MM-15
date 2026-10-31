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

#ifndef NUMERICS_H
#define NUMERICS_H

#include <stdbool.h>
#include "stack.h"

/* Numerical methods over user functions. A function is an execution token,
   pushed by @name, for a built-in or a word that maps x on the stack to
   f(x) on the stack. The function runs on the live stack, so words may use
   registers and other words; an error inside it aborts the call and the
   top-level rollback restores the stack. Convergence uses num_tol. */

#define DEFAULT_TOL   1e-10
#define MAX_FN_EVALS  2000000

extern double num_tol;        /* set by 'tol' */
extern long   num_evals;      /* function evaluations in the last call */
extern double num_last_err;   /* error estimate of the last integrate */

/* execution tokens */
double      xt_of(const char *name);     /* -1 if unknown */
const char *xt_name(double xt);
int         xt_check(double xt);         /* error unless valid */
int         xt_exec(Stack *stack, double xt);

int num_eval(Stack *stack, double xt, double x, double *fx);
int num_integrate(Stack *stack, double xt, double a, double b, double *result, double *err);
int num_romberg(Stack *stack, double xt, double a, double b, double *result);
int num_fzero(Stack *stack, double xt, double a, double b, double *root);
int num_solve(Stack *stack, double xt, double guess, double *root);
int num_minimize(Stack *stack, double xt, double a, double b, double *xmin);
int num_deriv(Stack *stack, double xt, double x, double *dfdx);
int num_fsum(Stack *stack, double xt, double lo, double hi, bool product, double *result);

#endif
