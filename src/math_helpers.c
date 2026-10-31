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
#include <complex.h>                        // for complex, cimag, cacos
#include <gsl/gsl_complex.h>                // for gsl_complex, GSL_IMAG
#include <gsl/gsl_complex_math.h>           // for gsl_complex_rect, gsl_com...
#include <gsl/gsl_matrix_complex_double.h>  // for gsl_matrix_complex_alloc
#include <gsl/gsl_matrix_double.h>          // for gsl_matrix_get, gsl_matrix
#include <math.h>                           // for log, modf, log10, sqrt, fabs
#include <stdbool.h>                        // for bool, false, true
#include <stdio.h>                          // for fprintf, stderr, size_t
#include "math_helpers.h"                   // for abs_wrapper, acos_wrapper
#include "spec_fun.h"                       // for gamma_function, ln_gamma_...
#include "stack.h"                          // for stack_top_type, pop, (ano...
#include "stat_fun.h"                       // for standard_normal_cdf, stan...
#include "unary_fun.h"                      // for apply_real_matrix_unary_i...

gsl_complex my_complex_asin(gsl_complex z) {
  double complex x = to_double_complex(z);
  return to_gsl_complex(casin(x));
}

gsl_complex my_complex_acos(gsl_complex z) {
  double complex x = to_double_complex(z);
  return to_gsl_complex(cacos(x));
}

gsl_complex my_complex_atan(gsl_complex z) {
  double complex x = to_double_complex(z);
  return to_gsl_complex(catan(x));
}

gsl_complex my_complex_asinh(gsl_complex z) {
  double complex x = to_double_complex(z);
  return to_gsl_complex(casinh(x));
}

gsl_complex my_complex_acosh(gsl_complex z) {
  double complex x = to_double_complex(z);
  return to_gsl_complex(cacosh(x));
}

gsl_complex my_complex_atanh(gsl_complex z) {
  double complex x = to_double_complex(z);
  return to_gsl_complex(catanh(x));
}


double safe_frac(double a)
{
  double intpart;
  return modf(a, &intpart);
}

double safe_int(double a)
{
  double intpart;
  modf(a, &intpart);
  return intpart;
}

#include <complex.h>
#include <math.h>

gsl_complex safe_frac_complex(gsl_complex z) {
  double r_int, i_int;
  double real_frac = modf(GSL_REAL(z), &r_int);
  double imag_frac = modf(GSL_IMAG(z), &i_int);
  return gsl_complex_rect(real_frac, imag_frac);
  //  return real_frac + imag_frac * I;
}

gsl_complex safe_int_complex(gsl_complex z) {
  double r_int, i_int;
  modf(GSL_REAL(z), &r_int);
  modf(GSL_IMAG(z), &i_int);
  return gsl_complex_rect(r_int, i_int);
  //  return r_int + i_int * I;
}

double negate_real(double x) {
  return -x;
}

double log10_real(double x) {
  return log(x) / log(10.0);
}

gsl_complex log10_complex(gsl_complex z) {
  gsl_complex ln_z = gsl_complex_log(z);
  return gsl_complex_div_real(ln_z, log(10.0));
}

gsl_complex negate_complex(gsl_complex x) {
  gsl_complex result;
  GSL_SET_COMPLEX(&result, -GSL_REAL(x), -GSL_IMAG(x));
  return result;
}

double one_over_real(double x) {
  if (x != 0.0) {
    return 1.0/x;
  } else {
    mm_error("Division by zero not allowed!\n");
    return 0.0;
  }
}

gsl_complex one_over_complex(gsl_complex x) {
  //  complex double a=to_double_complex(x);
  if (GSL_REAL(x) != 0.0 || GSL_IMAG(x) != 0.0) {
    return gsl_complex_inverse(x);
  } else {
    mm_error("Division by zero not allowed!\n");
    return gsl_complex_rect(0.0, 0.0);
  }
}

gsl_complex log10_complex_not_gsl(gsl_complex z) {
  gsl_complex lnz = gsl_complex_log(z);               // natural log
  gsl_complex log10_const = gsl_complex_rect(log(10.0), 0.0);
  return gsl_complex_div(lnz, log10_const);           // ln(z)/ln(10)
}

gsl_complex to_gsl_complex(double complex z) {
  return gsl_complex_rect(creal(z), cimag(z));
  //  return z;
}

double complex to_double_complex(gsl_complex z) {
  return GSL_REAL(z) + GSL_IMAG(z) * I;
}

bool is_zero_complex(gsl_complex z) {
  return (GSL_REAL(z) == 0.0) && (GSL_IMAG(z) == 0.0);
}

// ****************************************************************
// ************************* Wrappers *****************************
// ****************************************************************

#define DEFINE_UNARY_WRAPPER(name, real_fn, complex_fn)		\
  void name##_wrapper(Stack* stack) {				\
    value_type top_type = stack_top_type(stack);			\
    switch (top_type) {						\
    case TYPE_REAL:						\
      apply_real_unary(stack, real_fn);				\
      return;							\
    case TYPE_COMPLEX:						\
      apply_complex_unary(stack, complex_fn);			\
      return;							\
    case TYPE_MATRIX_REAL:					\
      apply_real_matrix_unary_inplace(stack, real_fn);		\
      return;							\
    case TYPE_MATRIX_COMPLEX:					\
      apply_complex_matrix_unary_inplace(stack, complex_fn);	\
      return;							\
    default:							\
      mm_error("Unsupported type in %s\n", #name);	\
      return;							\
    }								\
  }

// These functions do not mutate types
DEFINE_UNARY_WRAPPER(sin, sin, gsl_complex_sin)
  DEFINE_UNARY_WRAPPER(cos, cos, gsl_complex_cos)
  DEFINE_UNARY_WRAPPER(tan, tan, gsl_complex_tan)
  DEFINE_UNARY_WRAPPER(sinh, sinh, gsl_complex_sinh)
  DEFINE_UNARY_WRAPPER(cosh, cosh, gsl_complex_cosh)
  DEFINE_UNARY_WRAPPER(tanh, tanh, gsl_complex_tanh)
  DEFINE_UNARY_WRAPPER(exp, exp, gsl_complex_exp)
  DEFINE_UNARY_WRAPPER(chs, negate_real, negate_complex)
  DEFINE_UNARY_WRAPPER(inv, one_over_real, one_over_complex)
  DEFINE_UNARY_WRAPPER(frac, safe_frac, safe_frac_complex)
DEFINE_UNARY_WRAPPER(intg, safe_int, safe_int_complex)

DEFINE_UNARY_WRAPPER(atan, atan, my_complex_atan)
DEFINE_UNARY_WRAPPER(asinh, asinh, my_complex_asinh)

/* Functions whose real version is only defined on part of the real line.
   Outside that domain the argument is promoted to complex and the complex
   function is used, so -1 sqrt gives (0,1), 2 asin gives (1.5708,-1.3170),
   and a real matrix with one out-of-domain entry becomes a complex matrix.
   This is the one rule for every such function. */
static void promoting_unary(Stack *stack, const char *name, double (*fr)(double),
                            gsl_complex (*fc)(gsl_complex), bool (*in_domain)(double)) {
  switch (stack_top_type(stack)) {
  case TYPE_REAL: {
    double x = stack->items[stack->top].real;
    if (in_domain(x)) stack->items[stack->top].real = fr(x);
    else { pop(stack); push_complex(stack, fc(gsl_complex_rect(x, 0.0))); }
    return;
  }
  case TYPE_COMPLEX:
    apply_complex_unary(stack, fc);
    return;
  case TYPE_MATRIX_REAL: {
    gsl_matrix *m = stack->items[stack->top].matrix_real;
    bool all_ok = true;
    for (size_t i = 0; i < m->size1 && all_ok; i++)
      for (size_t j = 0; j < m->size2; j++)
        if (!in_domain(gsl_matrix_get(m, i, j))) { all_ok = false; break; }
    if (all_ok) { apply_real_matrix_unary_inplace(stack, fr); return; }
    gsl_matrix_complex *cm = gsl_matrix_complex_alloc(m->size1, m->size2);
    if (!cm) { mm_error("%s: out of memory", name); return; }
    for (size_t i = 0; i < m->size1; i++)
      for (size_t j = 0; j < m->size2; j++)
        gsl_matrix_complex_set(cm, i, j, fc(gsl_complex_rect(gsl_matrix_get(m, i, j), 0.0)));
    stack_element old = pop(stack);
    stack_element_free(&old);
    push_matrix_complex(stack, cm);
    return;
  }
  case TYPE_MATRIX_COMPLEX:
    apply_complex_matrix_unary_inplace(stack, fc);
    return;
  default:
    mm_error("Unsupported type in %s", name);
  }
}

static bool dom_nonneg(double x)  { return x >= 0.0; }
static bool dom_unit(double x)    { return fabs(x) <= 1.0; }
static bool dom_ge1(double x)     { return x >= 1.0; }
static bool dom_lt1(double x)     { return fabs(x) <= 1.0; }

#define DEFINE_PROMOTING_WRAPPER(name, real_fn, complex_fn, domain)  \
  void name##_wrapper(Stack *stack) { promoting_unary(stack, #name, real_fn, complex_fn, domain); }

DEFINE_PROMOTING_WRAPPER(asin,  asin,  my_complex_asin,  dom_unit)
DEFINE_PROMOTING_WRAPPER(acos,  acos,  my_complex_acos,  dom_unit)
DEFINE_PROMOTING_WRAPPER(acosh, acosh, my_complex_acosh, dom_ge1)
DEFINE_PROMOTING_WRAPPER(atanh, atanh, my_complex_atanh, dom_lt1)
DEFINE_PROMOTING_WRAPPER(sqrt,  sqrt,  gsl_complex_sqrt, dom_nonneg)
DEFINE_PROMOTING_WRAPPER(ln,    log,   gsl_complex_log,  dom_nonneg)
DEFINE_PROMOTING_WRAPPER(log,   log10, log10_complex,    dom_nonneg)

// **** Start: Define helpers to evaluate logical not **** 
static inline double real_not(double x) {
  return !x;
}

static inline gsl_complex complex_not(gsl_complex z) {
  //  return (GSL_REAL(z) == 0.0) + I*(GSL_IMAG(z) == 0.0);
  double re_part = (GSL_REAL(z) == 0.0) ? 1.0 : 0.0;
  double im_part = (GSL_IMAG(z) == 0.0) ? 1.0 : 0.0;
  return gsl_complex_rect(re_part, im_part);
}

DEFINE_UNARY_WRAPPER(logical_not, real_not, complex_not)
// **** End: Define helpers to evaluate logical not **** 


// These functions do mutate types
     void im_wrapper(Stack *stack) {
  value_type a = stack_top_type(stack);
  if (a == TYPE_REAL) {
    stack_element b = pop(stack);
    push_real(stack, cimag(b.real));
  }  else if (a == TYPE_COMPLEX) {
    stack_element b = pop(stack);	
    push_real(stack, GSL_IMAG(b.complex_val));}
  else if (a == TYPE_MATRIX_REAL)
    {
      // To Do: push a matrix of zeroes of the right size
    }
  else if (a == TYPE_MATRIX_COMPLEX)
    complex_matrix_imag_part(stack);
  else
    mm_error("im: unsupported type\n");
  return;
}

void re_wrapper(Stack *stack) {
  value_type a = stack_top_type(stack);
  if (a == TYPE_REAL) {
    return; // nothing needs to be done
  }  else if (a == TYPE_COMPLEX) {
    stack_element b = pop(stack);	
    push_real(stack, GSL_REAL(b.complex_val));}
  else if (a == TYPE_MATRIX_REAL)   {
    return; // nothing needs to be done
  } else if (a == TYPE_MATRIX_COMPLEX)
    complex_matrix_real_part(stack);
  else
    mm_error("re: unsupported type\n");
  return;
}

void abs_wrapper(Stack *stack) {
  value_type a = stack_top_type(stack);
  if (a == TYPE_REAL) {
    stack_element b = pop(stack);	
    push_real(stack,fabs(b.real));
    return;
  }  else if (a == TYPE_COMPLEX) {
    stack_element b = pop(stack);	
    push_real(stack, gsl_complex_abs(b.complex_val));}
  else if (a == TYPE_MATRIX_REAL)   {
    apply_real_matrix_unary_inplace(stack, fabs);
    return; 
  } else if (a == TYPE_MATRIX_COMPLEX)
    // to do complex abs element by element
    return;
  else
    mm_error("abs: unsupported type\n");
  return;
}

void arg_wrapper(Stack *stack) {
  value_type a = stack_top_type(stack);
  if (a == TYPE_REAL) {
    stack_element b = pop(stack);	
    push_real(stack,carg(b.real));
    return;
  }  else if (a == TYPE_COMPLEX) {
    stack_element b = pop(stack);	
    push_real(stack, gsl_complex_arg(b.complex_val));}
  else if (a == TYPE_MATRIX_REAL)   {
    // To do: push a matrix of zeroes
    return; 
  } else if (a == TYPE_MATRIX_COMPLEX)
    // to do complex arg element by element
    return;
  else
    mm_error("arg: unsupported type\n");
  return;
}




void npdf_wrapper(Stack *stack) {
  value_type a = stack_top_type(stack);
  if (a == TYPE_REAL) {
    stack_element b = pop(stack);
    push_real(stack, standard_normal_pdf(b.real));
    return;
  }  else if (a == TYPE_COMPLEX) {
    printf("npdf: unsupported type\n");
  } else if (a == TYPE_MATRIX_REAL) {
    apply_real_matrix_unary_inplace(stack, standard_normal_pdf);
    return;
  } else if (a == TYPE_MATRIX_COMPLEX)
    mm_error("npdf: unsupported type\n");
  return;
}

void ncdf_wrapper(Stack *stack) {
  value_type a = stack_top_type(stack);
  if (a == TYPE_REAL) {
    stack_element b = pop(stack);
    push_real(stack, standard_normal_cdf(b.real));
    return;
  }  else if (a == TYPE_COMPLEX) {
    mm_error("ncdf: unsupported type\n");
  } else if (a == TYPE_MATRIX_REAL) {
    apply_real_matrix_unary_inplace(stack, standard_normal_cdf);
    return;
  } else if (a == TYPE_MATRIX_COMPLEX)
    mm_error("ncdf: unsupported type\n");
  return;
}

void nquant_wrapper(Stack *stack) {
  value_type a = stack_top_type(stack);
  if (a == TYPE_REAL) {
    stack_element b = pop(stack);
    push_real(stack, standard_normal_quantile(b.real));
    return;
  }  else if (a == TYPE_COMPLEX) {
    mm_error("nquant: unsupported type\n");
  } else if (a == TYPE_MATRIX_REAL) {
    apply_real_matrix_unary_inplace(stack, standard_normal_quantile);
    return;
  } else if (a == TYPE_MATRIX_COMPLEX)
    mm_error("nquant: unsupported type\n");
  return;
}

void gamma_wrapper(Stack *stack) {
  value_type a = stack_top_type(stack);
  if (a == TYPE_REAL) {
    stack_element b = pop(stack);
    push_real(stack, gamma_function(b.real));
    return;
  }  else if (a == TYPE_COMPLEX) {
    mm_error("nquant: unsupported type\n");
  } else if (a == TYPE_MATRIX_REAL) {
    apply_real_matrix_unary_inplace(stack, gamma_function);
    return;
  } else if (a == TYPE_MATRIX_COMPLEX)
    mm_error("nquant: unsupported type\n");
  return;
}

void ln_gamma_wrapper(Stack *stack) {
  value_type a = stack_top_type(stack);
  if (a == TYPE_REAL) {
    stack_element b = pop(stack);
    push_real(stack, ln_gamma_function(b.real));
    return;
  }  else if (a == TYPE_COMPLEX) {
    mm_error("nquant: unsupported type\n");
  } else if (a == TYPE_MATRIX_REAL) {
    apply_real_matrix_unary_inplace(stack, ln_gamma_function);
    return;
  } else if (a == TYPE_MATRIX_COMPLEX)
    mm_error("nquant: unsupported type\n");
  return;
}

void beta_wrapper(Stack *stack) {
  if (stack_size(stack) < 2) {
    mm_error("Not enough elements on the stack\n");
    return;
  }
  value_type a = stack_top_type(stack);
  value_type b = stack_next2_top_type(stack);
  if ((a == TYPE_REAL) && (b == TYPE_REAL)) {
    stack_element x = pop(stack);
    stack_element y = pop(stack);
    push_real(stack, beta_function(x.real,y.real));
    return;
  }  else {
    mm_error("beta: unsupported type\n");
    return;
  }
}

void ln_beta_wrapper(Stack *stack) {
  if (stack_size(stack) < 2) {
    mm_error("Not enough elements on the stack\n");
    return;
  }
  value_type a = stack_top_type(stack);
  value_type b = stack_next2_top_type(stack);
  if ((a == TYPE_REAL) && (b == TYPE_REAL)) {
    stack_element x = pop(stack);
    stack_element y = pop(stack);
    push_real(stack, ln_beta_function(x.real,y.real));
    return;
  }  else {
    mm_error("ln_beta: unsupported type\n");
    return;
  }
}
