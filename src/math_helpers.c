/*
 * This file is part of Mico's toy RPN Calculator
 *
 * Mico's toy RPN Calculator is free software: 
 * you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Mico's toy RPN Calculator is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Mico's toy RPN Calculator. If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <math.h>
#include <complex.h>
#include <stdbool.h>
#include <gsl/gsl_complex_math.h>
#include "stack.h"
#include "unary_fun.h"
#include "binary_fun.h"
#include "stat_fun.h"
#include "spec_fun.h"
#include "math_helpers.h"


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
    fprintf(stderr,"Division by zero not allowed!\n");
    return 0.0;
  }
}

gsl_complex one_over_complex(gsl_complex x) {
  //  complex double a=to_double_complex(x);
  if (GSL_REAL(x) != 0.0 || GSL_IMAG(x) != 0.0) {
    return gsl_complex_inverse(x);
  } else {
    fprintf(stderr,"Division by zero not allowed!\n");
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
      fprintf(stderr, "Unsupported type in %s\n", #name);	\
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

DEFINE_UNARY_WRAPPER(asin, asin, my_complex_asin)
DEFINE_UNARY_WRAPPER(acos, acos, my_complex_acos)
DEFINE_UNARY_WRAPPER(atan, atan, my_complex_atan)
DEFINE_UNARY_WRAPPER(asinh, asinh, my_complex_asinh)
DEFINE_UNARY_WRAPPER(acosh, acosh, my_complex_acosh)
DEFINE_UNARY_WRAPPER(atanh, atanh, my_complex_atanh)

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
    fprintf(stderr,"im: unsupported type\n");
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
    fprintf(stderr,"re: unsupported type\n");
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
    fprintf(stderr,"abs: unsupported type\n");
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
    fprintf(stderr,"arg: unsupported type\n");
  return;
}

void ln_wrapper(Stack *stack) {
  stack_element a = pop(stack);

  if (a.type == TYPE_REAL) {
    if (a.real >= 0.0) {
      push_real(stack, log(a.real));
    } else {
      push_complex(stack, gsl_complex_log(gsl_complex_rect(a.real, 0.0)));
    }
  }

  else if (a.type == TYPE_COMPLEX) {
    push_complex(stack, gsl_complex_log(a.complex_val));
  }

  else if (a.type == TYPE_MATRIX_REAL) {
    gsl_matrix* m = a.matrix_real;
    size_t rows = m->size1;
    size_t cols = m->size2;

    // Check for any negative entry
    bool has_negative = false;
    for (size_t i = 0; i < rows && !has_negative; ++i) {
      for (size_t j = 0; j < cols; ++j) {
	if (gsl_matrix_get(m, i, j) < 0.0) {
	  has_negative = true;
	  break;
	}
      }
    }

    if (has_negative) {
      // Promote to complex
      gsl_matrix_complex* cm = gsl_matrix_complex_alloc(rows, cols);
      for (size_t i = 0; i < rows; ++i) {
	for (size_t j = 0; j < cols; ++j) {
	  double x = gsl_matrix_get(m, i, j);
	  gsl_complex z = gsl_complex_rect(x, 0.0);
	  gsl_matrix_complex_set(cm, i, j, gsl_complex_log(z));
	}
      }
      push_matrix_complex(stack, cm);
    } else {
      // Stay real
      gsl_matrix* rm = gsl_matrix_alloc(rows, cols);
      for (size_t i = 0; i < rows; ++i) {
	for (size_t j = 0; j < cols; ++j) {
	  double x = gsl_matrix_get(m, i, j);
	  gsl_matrix_set(rm, i, j, log(x));
	}
      }
      push_matrix_real(stack, rm);
    }
  }

  else if (a.type == TYPE_MATRIX_COMPLEX) {
    gsl_matrix_complex* m = a.matrix_complex;
    size_t rows = m->size1;
    size_t cols = m->size2;

    gsl_matrix_complex* result = gsl_matrix_complex_alloc(rows, cols);
    for (size_t i = 0; i < rows; ++i) {
      for (size_t j = 0; j < cols; ++j) {
	gsl_complex z = gsl_matrix_complex_get(m, i, j);
	gsl_matrix_complex_set(result, i, j, gsl_complex_log(z));
      }
    }
    push_matrix_complex(stack, result);
  }
  else {
    fprintf(stderr, "ln: unsupported type\n");
  }
}

void log_wrapper(Stack *stack) {
  stack_element a = pop(stack);

  gsl_complex log10_const = gsl_complex_rect(log(10.0), 0.0);

  if (a.type == TYPE_REAL) {
    if (a.real >= 0.0) {
      push_real(stack, log10(a.real));
    } else {
      gsl_complex z = gsl_complex_rect(a.real, 0.0);
      gsl_complex ln_z = gsl_complex_log(z);
      gsl_complex result = gsl_complex_div(ln_z, log10_const);
      push_complex(stack, result);
    }
  }

  else if (a.type == TYPE_COMPLEX) {
    gsl_complex ln_z = gsl_complex_log(a.complex_val);
    gsl_complex result = gsl_complex_div(ln_z, log10_const);
    push_complex(stack, result);
  }

  else if (a.type == TYPE_MATRIX_REAL) {
    gsl_matrix* m = a.matrix_real;
    size_t rows = m->size1;
    size_t cols = m->size2;

    bool has_negative = false;
    for (size_t i = 0; i < rows && !has_negative; ++i) {
      for (size_t j = 0; j < cols; ++j) {
	if (gsl_matrix_get(m, i, j) < 0.0) {
	  has_negative = true;
	  break;
	}
      }
    }

    if (has_negative) {
      // Promote to complex
      gsl_matrix_complex* cm = gsl_matrix_complex_alloc(rows, cols);
      for (size_t i = 0; i < rows; ++i) {
	for (size_t j = 0; j < cols; ++j) {
	  double x = gsl_matrix_get(m, i, j);
	  gsl_complex z = gsl_complex_rect(x, 0.0);
	  gsl_complex ln_z = gsl_complex_log(z);
	  gsl_complex res = gsl_complex_div(ln_z, log10_const);
	  gsl_matrix_complex_set(cm, i, j, res);
	}
      }
      push_matrix_complex(stack, cm);
    } else {
      // Stay real
      gsl_matrix* rm = gsl_matrix_alloc(rows, cols);
      for (size_t i = 0; i < rows; ++i) {
	for (size_t j = 0; j < cols; ++j) {
	  double x = gsl_matrix_get(m, i, j);
	  gsl_matrix_set(rm, i, j, log10(x));
	}
      }
      push_matrix_real(stack, rm);
    }
  }

  else if (a.type == TYPE_MATRIX_COMPLEX) {
    gsl_matrix_complex* m = a.matrix_complex;
    size_t rows = m->size1;
    size_t cols = m->size2;

    gsl_matrix_complex* result = gsl_matrix_complex_alloc(rows, cols);
    for (size_t i = 0; i < rows; ++i) {
      for (size_t j = 0; j < cols; ++j) {
	gsl_complex z = gsl_matrix_complex_get(m, i, j);
	gsl_complex ln_z = gsl_complex_log(z);
	gsl_matrix_complex_set(result, i, j, gsl_complex_div(ln_z, log10_const));
      }
    }
    push_matrix_complex(stack, result);
  }
  else {
    fprintf(stderr, "log: unsupported type\n");
  }
}

void sqrt_wrapper(Stack *stack) {
  stack_element a = pop(stack);

  if (a.type == TYPE_REAL) {
    if (a.real >= 0.0) {
      push_real(stack, sqrt(a.real));
    } else {
      gsl_complex z = gsl_complex_rect(a.real, 0.0);
      push_complex(stack, gsl_complex_sqrt(z));
    }
  }

  else if (a.type == TYPE_COMPLEX) {
    push_complex(stack, gsl_complex_sqrt(a.complex_val));
  }

  else if (a.type == TYPE_MATRIX_REAL) {
    gsl_matrix* m = a.matrix_real;
    size_t rows = m->size1;
    size_t cols = m->size2;

    bool has_negative = false;
    for (size_t i = 0; i < rows && !has_negative; ++i) {
      for (size_t j = 0; j < cols; ++j) {
	if (gsl_matrix_get(m, i, j) < 0.0) {
	  has_negative = true;
	  break;
	}
      }
    }

    if (has_negative) {
      // Promote to complex
      gsl_matrix_complex* cm = gsl_matrix_complex_alloc(rows, cols);
      for (size_t i = 0; i < rows; ++i) {
	for (size_t j = 0; j < cols; ++j) {
	  double x = gsl_matrix_get(m, i, j);
	  gsl_complex z = gsl_complex_rect(x, 0.0);
	  gsl_matrix_complex_set(cm, i, j, gsl_complex_sqrt(z));
	}
      }
      push_matrix_complex(stack, cm);
    } else {
      gsl_matrix* rm = gsl_matrix_alloc(rows, cols);
      for (size_t i = 0; i < rows; ++i) {
	for (size_t j = 0; j < cols; ++j) {
	  double x = gsl_matrix_get(m, i, j);
	  gsl_matrix_set(rm, i, j, sqrt(x));
	}
      }
      push_matrix_real(stack, rm);
    }
  }

  else if (a.type == TYPE_MATRIX_COMPLEX) {
    gsl_matrix_complex* m = a.matrix_complex;
    size_t rows = m->size1;
    size_t cols = m->size2;

    gsl_matrix_complex* result = gsl_matrix_complex_alloc(rows, cols);
    for (size_t i = 0; i < rows; ++i) {
      for (size_t j = 0; j < cols; ++j) {
	gsl_complex z = gsl_matrix_complex_get(m, i, j);
	gsl_matrix_complex_set(result, i, j, gsl_complex_sqrt(z));
      }
    }
    push_matrix_complex(stack, result);
  }

  else {
    fprintf(stderr, "sqrt: unsupported type\n");
  }
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
    fprintf(stderr,"npdf: unsupported type\n");
  return;
}

void ncdf_wrapper(Stack *stack) {
  value_type a = stack_top_type(stack);
  if (a == TYPE_REAL) {
    stack_element b = pop(stack);
    push_real(stack, standard_normal_cdf(b.real));
    return;
  }  else if (a == TYPE_COMPLEX) {
    fprintf(stderr,"ncdf: unsupported type\n");
  } else if (a == TYPE_MATRIX_REAL) {
    apply_real_matrix_unary_inplace(stack, standard_normal_cdf);
    return;
  } else if (a == TYPE_MATRIX_COMPLEX)
    fprintf(stderr,"ncdf: unsupported type\n");
  return;
}

void nquant_wrapper(Stack *stack) {
  value_type a = stack_top_type(stack);
  if (a == TYPE_REAL) {
    stack_element b = pop(stack);
    push_real(stack, standard_normal_quantile(b.real));
    return;
  }  else if (a == TYPE_COMPLEX) {
    fprintf(stderr,"nquant: unsupported type\n");
  } else if (a == TYPE_MATRIX_REAL) {
    apply_real_matrix_unary_inplace(stack, standard_normal_quantile);
    return;
  } else if (a == TYPE_MATRIX_COMPLEX)
    fprintf(stderr,"nquant: unsupported type\n");
  return;
}

void gamma_wrapper(Stack *stack) {
  value_type a = stack_top_type(stack);
  if (a == TYPE_REAL) {
    stack_element b = pop(stack);
    push_real(stack, gamma_function(b.real));
    return;
  }  else if (a == TYPE_COMPLEX) {
    fprintf(stderr,"nquant: unsupported type\n");
  } else if (a == TYPE_MATRIX_REAL) {
    apply_real_matrix_unary_inplace(stack, gamma_function);
    return;
  } else if (a == TYPE_MATRIX_COMPLEX)
    fprintf(stderr,"nquant: unsupported type\n");
  return;
}

void ln_gamma_wrapper(Stack *stack) {
  value_type a = stack_top_type(stack);
  if (a == TYPE_REAL) {
    stack_element b = pop(stack);
    push_real(stack, ln_gamma_function(b.real));
    return;
  }  else if (a == TYPE_COMPLEX) {
    fprintf(stderr,"nquant: unsupported type\n");
  } else if (a == TYPE_MATRIX_REAL) {
    apply_real_matrix_unary_inplace(stack, ln_gamma_function);
    return;
  } else if (a == TYPE_MATRIX_COMPLEX)
    fprintf(stderr,"nquant: unsupported type\n");
  return;
}

void beta_wrapper(Stack *stack) {
  if (stack_size(stack) < 2) {
    fprintf(stderr,"Not enough elements on the stack\n");
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
    fprintf(stderr,"beta: unsupported type\n");
    return;
  }
}

void ln_beta_wrapper(Stack *stack) {
  if (stack_size(stack) < 2) {
    fprintf(stderr,"Not enough elements on the stack\n");
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
    fprintf(stderr,"ln_beta: unsupported type\n");
    return;
  }
}
