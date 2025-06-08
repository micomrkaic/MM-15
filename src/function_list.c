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

#include <string.h>
#include <stdbool.h>
#include "function_list.h"

const char* const function_names[] = {
  "sin", "cos", "tan", "asin", "acos", "atan",
  "sinh", "cosh", "tanh", "asinh", "acosh", "atanh",
  "ln", "log", "exp", "sqrt", "pow", "re", "im", "abs", "arg", "conj",
  "npdf", "ncdf", "nquant","gamma", "ln_gamma","beta","ln_beta",
  "re2c", "split_c", "j2r","frac","intg",
  "chs", "inv",
  "fuck", "help", "listfcns",
  "gravity", "pi", "e", "inf", "nan",
  "drop", "clst", "swap", "dup", "nip", "tuck", "roll", "over",
  "scon", "s2l", "s2u", "slen", "srev", "int2str",
  "minv", "pinv", "det", "eig", "tran", "reshape", "get_aij", "set_aij","split_mat","'",
  "kron", "diag", "to_diag", "chol", "svd", "dim", "eye",
  "join_v", "join_h", "cumsum_r", "cumsum_c",
  "ones", "zeroes", "rand", "randn", "rrange",
  "cmean", "rmean", "csum", "rsum", "cvar", "rvar",
  "cmin", "cmax", "rmin", "rmax",
  "roots", "pval", "integrate", "fzero", "set_intg_tol", "set_f0_tol",
  "rcl", "sto","pr","saveregs","loadregs","clregs","ffr",
  "print", "pm", "ps", "setprec","sfs","undo",
  ".*", "./", ".^",
  "eq","leq","lt","gt","geq","neq","and","or","not",
  "ddays","today","dateplus","dow","edmy",
  "listwords",  "loadwords", "savewords", "delword", "selword","clrwords", "listmacros",
  "clrhist",
  "top_eq0?", "top_ge0?",  "top_gt0?", "top_le0?",  "top_lt0?",
  "top_eg?", "top_ge?",  "top_gt?", "top_le?",  "top_lt?",
  "ctr_eq0?", "ctr_ge0?",  "ctr_gt0?", "ctr_le0?",  "ctr_lt0?",
  "set_ctr", "clr_ctr", "ctr_inc", "ctr_dec",
  "goto", "xeq", "rtn", "end", "lbl",
  "eval", "batch", "run", 
  NULL
};
