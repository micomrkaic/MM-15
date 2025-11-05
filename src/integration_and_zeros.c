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

#define MAX_ROMBERG_ITER 20
#define MAX_BISECTION_ITER 40

#include <math.h>
#include <stdio.h>
#include "words.h"
#include "globals.h"
#include "stack.h"
#include "eval_fun.h"
#include "integration_and_zeros.h"

void find_zero(Stack *stack) {
  if (stack->top < 1) {
    fprintf(stderr, "Error: Stack underflow — need two real numbers.\n");
    return;
  }

  stack_element a = pop(stack);
  stack_element b = pop(stack);

  if (a.type != TYPE_REAL || b.type != TYPE_REAL) {
    fprintf(stderr, "Error: find_zero requires two real scalars.\n");
    return;
  }

  if (b.real >= a.real) {
    fprintf(stderr, "Error: messed up interval bounds.\n");
    return;
  }

  double root;
  if (bisection(stack_helper, b.real, a.real, fsolve_tolerance, &root)) {
    push_real(stack, root);
  } else {
    fprintf(stderr, "Warning: Bisection failed — pushing a.real back.\n");
    push_real(stack, a.real);
  }
}


void integrate(Stack *stack) {
  if (stack->top < 1) {
    fprintf(stderr, "Error: Stack underflow — need two real numbers for integration.\n");
    return;
  }

  stack_element a = pop(stack);
  stack_element b = pop(stack);

  if (a.type != TYPE_REAL || b.type != TYPE_REAL) {
    fprintf(stderr, "Error: integrate requires two real scalars.\n");
    return;
  }

  if (b.real >= a.real) {
    fprintf(stderr, "Error: messed up integration limits.\n");
    return;
  }

  double result = romberg(stack_helper, b.real, a.real, intg_tolerance, MAX_ROMBERG_ITER);
  push_real(stack, result);
}



// Romberg integration
double romberg(double (*f)(double), double a, double b, double tol, int max_iter) {
  double R[MAX_ROMBERG_ITER][MAX_ROMBERG_ITER];
  int i, j;
  double h = b - a;

  // R[0][0] is the trapezoidal rule with 1 interval
  R[0][0] = 0.5 * h * (f(a) + f(b));

  for (i = 1; i < max_iter; i++) {
    h *= 0.5;

    // Compute trapezoidal sum for 2^i intervals
    double sum = 0.0;
    int n = 1 << (i - 1); // 2^(i-1)
    for (int k = 1; k <= n; k++) {
      sum += f(a + (2 * k - 1) * h);
    }
    
    R[i][0] = 0.5 * R[i - 1][0] + h * sum;
    
    // Richardson extrapolation
    for (j = 1; j <= i; j++) {
      R[i][j] = R[i][j - 1] + (R[i][j - 1] - R[i - 1][j - 1]) / (pow(4, j) - 1);
    }
    
    // Check for convergence
    if (i > 1 && fabs(R[i][i] - R[i - 1][i - 1]) < tol) {
      return R[i][i];
    }
  }
  
  fprintf(stderr,"Warning: Romberg integration did not converge after %d iterations\n", max_iter);
  return R[max_iter - 1][max_iter - 1];
}

void set_integration_precision(Stack *stack) {
  stack_element a = pop(stack);
  if ((a.type == TYPE_REAL) && (a.real >= 1.0e-10) && (a.real <= 1.0e-2))
    intg_tolerance = a.real;
  else 
    fprintf(stderr,"Incorrect argument\n");
}

void set_f0_precision(Stack *stack) {
  stack_element a = pop(stack);
  if ((a.type == TYPE_REAL) && (a.real >= 1.0e-10) && (a.real <= 1.0e-2))
    fsolve_tolerance = a.real;
  else 
    fprintf(stderr,"Incorrect argument\n");
}

double stack_helper(double x) {
  Stack integration_stack;

  init_stack(&integration_stack);
  push_real(&integration_stack,x);
  evaluate_line(&integration_stack, words[selected_function].name);
  stack_element a = pop(&integration_stack);
  return a.real;
}

/* void integrate(Stack *stack) { */
/*   stack_element b = pop(stack);  */
/*   stack_element a = pop(stack); */
/*   push_real(stack,romberg(stack_helper,a.real,b.real,intg_tolerance,MAX_ROMBERG_ITER)); */
/* } */

/* void find_zero(Stack *stack) { */
/*   stack_element b = pop(stack);  */
/*   stack_element a = pop(stack); */
/*   double root; */
/*   if (bisection(stack_helper, a.real, b.real, fsolve_tolerance, &root)) */
/*     push_real(stack,root); */
/*   else */
/*     push_real(stack,a.real); */
/* } */

// Bisection function
bool bisection(double (*f)(double), double a, double b, double tol, double* root) {
  double fa = f(a);
  double fb = f(b);

  // Check for a sign change
  if (fa * fb > 0) {
    fprintf(stderr,
	    "Error: f(a) and f(b) do not have opposite signs. No guaranteed root in [%.6f, %.6f]\n",
	    a, b);
    return false;
  }

  for (int i = 0; i < MAX_BISECTION_ITER; ++i) {
    double mid = 0.5 * (a + b);
    double fmid = f(mid);

    if (fabs(fmid) < tol || (b - a) < tol) {
      *root = mid;
      return true;
    }

    // Determine subinterval
    if (fa * fmid < 0) {
      b = mid;
      fb = fmid;
    } else {
      a = mid;
      fa = fmid;
    }
  }
  fprintf(stderr,"Warning: Maximum iterations reached without convergence\n");
  return false;
}
