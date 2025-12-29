#define _POSIX_C_SOURCE 200112L
#ifdef __APPLE__
#define _DARWIN_C_SOURCE
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

#include <stdbool.h>        // for true
#include <stdio.h>          // for printf, fprintf, NULL, stderr
#include <stdlib.h>         // for free, malloc, qsort
#include <string.h>         // for strcmp
#include "function_list.h"  // for function_names
#include "globals.h"        // for skip_stack_printing
#include "help.h"           // for HelpEntry, help_menu, list_all_functions
#include "stack.h"          // for (anonymous struct)::(anonymous), TYPE_STRING

#define BOLD      "\033[1m"
#define ITALIC    "\033[3m"
#define UNDERLINE "\033[4m"
#define RESET     "\033[0m"

#define title(s) printf(BOLD s RESET "\n")
#define subtitle(s) printf(UNDERLINE s RESET "\n")

void whose_place(void) {
  printf("Your place or mine?\n");
  return;
}

void help_menu(void) {
  //  printf("\n\n_mico's toy Matrix and Scalar RPN Calculator\n");
  printf("\n");
  title("RPN Calculator for real and complex scalars and matrices");
  subtitle("Quick Start and Entering data + Commands ");
  printf("    All inputs are case sensitive. Enter strings as \"string\".\n");
  printf("    Enter complex numbers as in: (1,3) or (-1.2e-4, 0.7e2).\n");
  printf("    Enter inline matrices as in J language [#rows #cols $ values]. \n");
  printf("    Example: [2 2 $ -1 2 5 1]. Matrix entries can be real or complex.\n");
  printf("    Read matrix from file as [#rows, #cols, \"filename\"].\n");
  printf("    You can undo the last line entry with undo.\n");
  subtitle("Stack manipulations");
  printf("    drop, dup, swap, clst, nip, tuck, roll, over, savestack, loadstack\n");
  subtitle("Math functions");
  printf("    Math functions work on scalars and matrices wherever possible. \n");
  printf("    Basic stuff: +, -, *, /, ^,  ln, exp, log, chs, inv, pct, pctchg \n");
  printf("    Trigonometry: sin, cos, tan, asin, acos, atan\n");
  printf("    Hyperbolic: sinh, cosh, tanh, asinh, acosh, atanh\n");
  printf("    Polynomials: evaluation and zeros\n");
  printf("    Normal distribution: npdf, ncdf, nquant {quantiles}\n");
  printf("    Special functions: gamma, ln_gamma, beta, ln_beta\n");
  subtitle("Comparison and logic functions");
  printf("    eq, leq, lt, gt, geq, neq, and,  or, not\n");
  subtitle("Complex numbers");
  printf("    re, im, abs, arg, re2c, split_c, j2r {join 2 reals into complex}\n");
  subtitle("Constants");
  printf("    pi, e, gravity, inf, nan\n");
  subtitle("Matrix functions");
  printf("    Get individual matrix elements with get_aij; set them with set_aij.\n");
  printf("    Print the matrix on top of the stack with pm \n");  
  printf("    Special matrices: eye, ones, rand, randn, rrange.\n");  
  printf("    Manipulation: reshape, diag, to_diag, split_mat, join_h, join_v \n");
  printf("    Cummulative sums and products: cumsum_r, cumsum_c, cumprod_r, cumprod_c \n");  
  printf("    Basic matrix statistics: csum, rsum, cmean, rmean, cvar, rvar\n");  
  printf("    Matrix min and max: cmin, rmin, cmax, rmax\n");  
  printf("    Linear algebra: tran, {also '}, det, minv, pinv, chol, eig, svd\n");  
  subtitle("Register functions");
  printf("    sto, rcl, pr {print registers}, saveregs, load, ffr {1st free register} \n");
  subtitle("String functions");
  printf("    scon, substr, s2u, s2l, slen, srev, int2str, eval {evaluate string}\n");
  subtitle("Financial and date functions");
  printf("    npv, irr, ddays, dateplus, today, days2eoy, dow, edmy, num2date \n");
  subtitle("Output format options");
  printf("    setprec {set print precision}, sfs {fix<->sci}\n");
  subtitle("Help and utilities");
  printf("    listfcns {list built in functions}\n");
  printf("    listmacros {list predefined macros}\n");
  printf("    listwords {list user-defined words}\n");
  printf("    new words start with : end with ;\n");
  printf("    Example to compute square : sq dup * ;\n");
  printf("\n");
  skip_stack_printing = true;
}

void list_all_functions(void) {
  printf("Built-in functions:\n\n");
  int count = 0;
  for (int i = 0; function_names[i] != NULL; ++i) {
    printf("%-16s", function_names[i]);  // left-align in 16-char width
    count++;
    if (count % 4 == 0)
      printf("\n");
  }
  if (count % 4 != 0)
    printf("\n");  // final newline if last line wasn't complete
}

// Compare function for qsort
static int compare_strings(const void *a, const void *b)
{
  const char * const *sa = a;
  const char * const *sb = b;
  return strcmp(*sa, *sb);
}

void list_all_functions_sorted(void) {
  // Step 1: Count functions
  int count = 0;
  while (function_names[count] != NULL) count++;

  // Step 2: Copy to a temporary array
  const char** sorted = malloc(count * sizeof(char*));
  if (!sorted) {
    fprintf(stderr, "Memory allocation failed\n");
    return;
  }
  for (int i = 0; i < count; ++i)
    sorted[i] = function_names[i];

  // Step 3: Sort
  qsort(sorted, count, sizeof(char*), compare_strings);

  // Step 4: Print 6 per row
  printf("Built-in functions:\n\n");
  for (int i = 0; i < count; ++i) {
    printf("%-16s", sorted[i]);
    if ((i + 1) % 6 == 0)
      printf("\n");
  }
  if (count % 6 != 0)  printf("\n");
  printf("\n");

  // Cleanup
  free(sorted);
}

const HelpEntry help_table[] = {
    /* --- Trig functions (real/complex) --- */
    { "sin",    "z -- sin(z)",
      "Sine of real or complex argument.",
      "0.5 sin      (pushes sin(0.5))" },

    { "cos",    "z -- cos(z)",
      "Cosine of real or complex argument.",
      "1 cos       (pushes cos(1))" },

    { "tan",    "z -- tan(z)",
      "Tangent of real or complex argument.",
      "0.3 tan     (pushes tan(0.3))" },

    { "asin",   "z -- asin(z)",
      "Inverse sine (principal value).",
      "0.5 asin    (pushes arcsin(0.5))" },

    { "acos",   "z -- acos(z)",
      "Inverse cosine (principal value).",
      "0.5 acos" },

    { "atan",   "z -- atan(z)",
      "Inverse tangent (principal value).",
      "1 atan      (≈ 0.785398...)" },

    { "sinh",   "z -- sinh(z)",
      "Hyperbolic sine.",
      "1 sinh" },

    { "cosh",   "z -- cosh(z)",
      "Hyperbolic cosine.",
      "1 cosh" },

    { "tanh",   "z -- tanh(z)",
      "Hyperbolic tangent.",
      "1 tanh" },

    { "asinh",  "z -- asinh(z)",
      "Inverse hyperbolic sine.",
      "1 asinh" },

    { "acosh",  "z -- acosh(z)",
      "Inverse hyperbolic cosine.",
      "2 acosh" },

    { "atanh",  "z -- atanh(z)",
      "Inverse hyperbolic tangent.",
      "0.5 atanh" },

    /* --- Logs, exp, powers --- */
    { "ln",     "x -- ln(x)",
      "Natural logarithm.",
      "10 ln      (≈ 2.302585...)" },

    { "log",    "x -- log10(x)",
      "Base-10 logarithm.",
      "100 log    (→ 2)" },

    { "exp",    "x -- e^x",
      "Exponential function.",
      "1 exp      (≈ 2.71828...)" },

    { "sqrt",   "x -- sqrt(x)",
      "Square root; supports complex for x < 0.",
      "9 sqrt     (→ 3)" },

    { "pow",    "x y -- x^y",
      "Raise x to the power y.",
      "2 10 pow   (→ 1024)" },

    /* --- Complex components --- */
    { "re",     "z -- Re(z)",
      "Real part of a complex number.",
      "(1,2) re   (→ 1)" },

    { "im",     "z -- Im(z)",
      "Imaginary part of a complex number.",
      "(1,2) im   (→ 2)" },

    { "abs",    "z -- |z|",
      "Absolute value (magnitude for complex).",
      "3 abs      (→ 3)" },

    { "arg",    "z -- arg(z)",
      "Complex argument (phase) in radians.",
      "(0,1) arg  (→ pi/2)" },

    { "conj",   "z -- conj(z)",
      "Complex conjugate.",
      "(1,2) conj (→ 1-2i)" },

    /* --- Probability / statistics --- */
    { "npdf",   "x -- φ(x)",
      "Standard normal (mean 0, sd 1) PDF.",
      "0 npdf     (≈ 0.3989)" },

    { "ncdf",   "x -- Φ(x)",
      "Standard normal CDF.",
      "1.96 ncdf  (≈ 0.975)" },

    { "nquant", "p -- x",
      "Inverse standard normal CDF (quantile).",
      "0.975 nquant (≈ 1.96)" },

    /* --- Special functions --- */
    { "gamma",  "x -- Γ(x)",
      "Gamma function.",
      "5 gamma    (→ 24)" },

    { "ln_gamma","x -- ln Γ(x)",
      "Log gamma function.",
      "10 ln_gamma" },

    { "beta",   "a b -- B(a,b)",
      "Beta function.",
      "2 3 beta   (→ 1/12)" },

    { "ln_beta","a b -- ln B(a,b)",
      "Log beta function.",
      "2 3 ln_beta" },

    /* --- Real/complex conversion helpers (speculative) --- */
    { "re2c",   "x -- x+0i",
      "Promote real number to complex.",
      "5 re2c     (→ 5+0i)" },

    { "split_c","z -- Re(z) Im(z)",
      "Split complex into real and imaginary parts.",
      "(1,2) split_c  (→ 1 2)" },

    { "j2r",    "Im(z) Re(z) -- z",
      "Join two reals into complex (imaginary on top).",
      "1 2 j2r   (→ 2+1i)   ; adjust to your actual convention" },

    /* --- Integer / fractional parts, sign, inverse --- */
    { "frac",   "x -- frac(x)",
      "Fractional part of x (x - floor(x)).",
      "3.7 frac   (→ 0.7)" },

    { "intg",   "x -- floor(x)",
      "Integer part (floor).",
      "3.7 intg   (→ 3)" },

    { "chs",    "x -- -x",
      "Change sign.",
      "5 chs      (→ -5)" },

    { "inv",    "x -- 1/x",
      "Multiplicative inverse.",
      "4 inv      (→ 0.25)" },

    /* --- Meta / help --- */
    { "fuck",   "--",
      "User-defined diagnostic/joke word (see source).",
      "fuck       (behavior depends on your implementation)" },

    { "help",   "[name?] --",
      "Print help for a word, or general help if stack empty or top not a string.",
      "\"sin\" help    (display info about sin)" },

    { "listfcns","--",
      "List all built-in function names.",
      "listfcns" },

    /* --- Constants --- */
    { "gravity","-- g",
      "Push standard gravitational acceleration (m/s^2).",
      "gravity    (→ 9.80665...)" },

    { "pi",     "-- π",
      "Push π.",
      "pi" },

    { "e",      "-- e",
      "Push Euler's number.",
      "e" },

    { "inf",    "-- +∞",
      "Push positive infinity.",
      "inf" },

    { "nan",    "-- NaN",
      "Push a NaN (not-a-number).",
      "nan" },

    /* --- Stack manipulation --- */
    { "drop",   "x --",
      "Drop the top stack element.",
      "1 2 drop   (→ 1)" },

    { "clst",   "--",
      "Clear the entire stack.",
      "1 2 3 clst (stack empty)" },

    { "swap",   "a b -- b a",
      "Swap top two elements.",
      "1 2 swap   (→ 2 1)" },

    { "dup",    "x -- x x",
      "Duplicate top of stack.",
      "5 dup      (→ 5 5)" },

    { "nip",    "a b -- b",
      "Drop second-from-top element (keep top).",
      "1 2 nip    (→ 2)" },

    { "tuck",   "a b -- b a b",
      "Duplicate second item and tuck below top.",
      "1 2 tuck   (→ 2 1 2)" },

    { "roll",   "… x_n … x_0  n -- … x_0 x_n … x_1",
      "Roll nth item (0=top) to the top.",
      "1 2 3 2 roll   (rolls 3rd-from-top)" },

    { "over",   "a b -- a b a",
      "Copy second item to top.",
      "1 2 over   (→ 1 2 1)" },

    /* --- String operations --- */
    { "scon",   "s1 s2 -- s3",
      "String concatenation.",
      "\"foo\" \"bar\" scon   (→ \"foobar\")" },

    { "s2l",    "s -- s_lower",
      "Convert string to lowercase.",
      "\"Hello\" s2l  (→ \"hello\")" },

    { "s2u",    "s -- s_upper",
      "Convert string to uppercase.",
      "\"Hello\" s2u  (→ \"HELLO\")" },

    { "slen",   "s -- n",
      "Length of string in characters.",
      "\"hello\" slen (→ 5)" },

    { "srev",   "s -- s_rev",
      "Reverse string.",
      "\"abc\" srev   (→ \"cba\")" },

    { "int2str","n -- s",
      "Convert integer to string.",
      "123 int2str   (→ \"123\")" },

    { "substr", "s start len -- s_sub",
      "Substring: 0-based index, length len.",
      "\"abcdef\" 1 3 substr   (→ \"bcd\")" },

    /* --- Matrix ops: linear algebra --- */
    { "minv",   "A -- A^{-1}",
      "Matrix inverse (real).",
      "2 2 eye minv (→ identity again)" },

    { "pinv",   "A -- A^+",
      "Moore–Penrose pseudoinverse.",
      "A pinv" },

    { "det",    "A -- det(A)",
      "Determinant of a square matrix.",
      "2 2 eye det (→ 1)" },

    { "eig",    "A -- V Λ",
      "Eigen-decomposition (matrix of eigenvectors and eigenvalues).",
      "A eig      (→ V Λ)" },

    { "tran",   "A -- A^T",
      "Matrix transpose.",
      "A tran" },

    { "reshape","A rows cols -- B",
      "Reshape matrix to given dimensions (row-major).",
      "A 2 3 reshape" },

    { "get_aij","A i j -- a_ij",
      "Get matrix element (0- or 1-based depending on your convention).",
      "A 1 2 get_aij" },

    { "set_aij","A i j x -- A'",
      "Set matrix element to x.",
      "A 1 2 5 set_aij" },

    { "split_mat","A -- v1 v2 ...",
      "Split matrix into row or column vectors (depending on implementation).",
      "A split_mat" },

    { "'",      "A -- A^T",
      "Matrix transpose (short form).",
      "A '" },

    { "kron",   "A B -- A ⊗ B",
      "Kronecker product of two matrices.",
      "A B kron" },

    { "diag",   "v -- D   or   A -- diag(v)",
      "Diagonal matrix from vector or extract main diagonal.",
      "1 2 3 join_v diag" },

    { "to_diag","A -- D",
      "Zero out off-diagonal terms (keep diagonal).",
      "A to_diag" },

    { "chol",   "A -- L",
      "Cholesky factorization (A = L L^T).",
      "A chol" },

    { "svd",    "A -- U S Vt",
      "Singular value decomposition.",
      "A svd" },

    { "dim",    "A -- rows cols",
      "Matrix dimensions.",
      "A dim" },

    { "eye",    "n -- I_n",
      "Identity matrix of size n×n.",
      "3 eye" },

    { "join_v", "v1 v2 -- [v1; v2]",
      "Stack vectors vertically.",
      "v1 v2 join_v" },

    { "join_h", "v1 v2 -- [v1 v2]",
      "Concatenate vectors horizontally.",
      "v1 v2 join_h" },

    { "cumsum_r","A -- B",
      "Row-wise cumulative sum.",
      "A cumsum_r" },

    { "cumsum_c","A -- B",
      "Column-wise cumulative sum.",
      "A cumsum_c" },

    { "ones",   "rows cols -- A",
      "Matrix filled with ones.",
      "2 3 ones" },

    { "zeroes", "rows cols -- A",
      "Matrix filled with zeros.",
      "2 3 zeroes" },

    { "rand",   "rows cols -- A",
      "Matrix of uniform(0,1) randoms.",
      "2 2 rand" },

    { "randn",  "rows cols -- A",
      "Matrix of standard normal randoms.",
      "2 2 randn" },

    { "rrange", "start step end -- v",
      "Range vector (start:step:end).",
      "0 0.1 1 rrange" },

    { "cmean",  "A -- row_mean",
      "Mean of each column.",
      "A cmean" },

    { "rmean",  "A -- col_mean",
      "Mean of each row.",
      "A rmean" },

    { "csum",   "A -- row_sum",
      "Sum of each column.",
      "A csum" },

    { "rsum",   "A -- col_sum",
      "Sum of each row.",
      "A rsum" },

    { "cvar",   "A -- col_variances",
      "Variance of each column.",
      "A cvar" },

    { "rvar",   "A -- row_variances",
      "Variance of each row.",
      "A rvar" },

    { "cmin",   "A -- col_mins",
      "Minimum of each column.",
      "A cmin" },

    { "cmax",   "A -- col_maxs",
      "Maximum of each column.",
      "A cmax" },

    { "rmin",   "A -- row_mins",
      "Minimum of each row.",
      "A rmin" },

    { "rmax",   "A -- row_maxs",
      "Maximum of each row.",
      "A rmax" },

    /* --- Polynomials / integration / roots --- */
    { "roots",  "coeffs -- r1 r2 ...",
      "Roots of polynomial with given coefficients.",
      "[1 0 -1] roots   (x^2 - 1 = 0 → ±1)" },

    { "pval",   "x coeffs -- y",
      "Evaluate polynomial at x.",
      "2 [1 0 -1] pval   (2^2 - 1 = 3)" },

    { "integrate","a b f -- ∫_a^b f(x) dx",
      "Numerical integration over [a,b] of function/macro f.",
      "0 1 \"f\" integrate" },

    { "fzero",  "x0 f -- x*",
      "Find root of function f near initial guess x0.",
      "0 \"f\" fzero" },

    { "set_intg_tol","tol --",
      "Set tolerance for numerical integration.",
      "1e-8 set_intg_tol" },

    { "set_f0_tol","tol --",
      "Set tolerance for root finding.",
      "1e-8 set_f0_tol" },

    /* --- Registers / memory --- */
    { "rcl",    "n -- value",
      "Recall value from register n.",
      "0 rcl" },

    { "sto",    "value n --",
      "Store value into register n.",
      "42 0 sto" },

    { "pr",     "n --",
      "Print contents of register n.",
      "0 pr" },

    { "saveregs","filename --",
      "Save all registers to file.",
      "\"regs.dat\" saveregs" },

    { "loadregs","filename --",
      "Load registers from file.",
      "\"regs.dat\" loadregs" },

    { "clregs", "--",
      "Clear all registers.",
      "clregs" },

    { "ffr",    "--",
      "Free/flush register-related resources (exact semantics: see source).",
      "ffr" },

    /* --- Printing / precision / undo --- */
    { "print",  "x -- x",
      "Print top of stack in current format (leaving it on stack).",
      "42 print" },

    { "pm",     "A -- A",
      "Print matrix in human-readable form.",
      "A pm" },

    { "ps",     "--",
      "Print entire stack.",
      "ps" },

    { "setprec","n --",
      "Set number of digits for printing.",
      "10 setprec" },

    { "sfs",    "n --",
      "Set field width/significant figures (see your printing code).",
      "15 sfs" },

    { "undo",   "--",
      "Undo last stack operation (if history enabled).",
      "undo" },

    /* --- Element-wise array ops --- */
    { ".*",     "A B -- C",
      "Element-wise (Hadamard) product of matrices/vectors.",
      "A B .*" },

    { "./",     "A B -- C",
      "Element-wise division.",
      "A B ./" },

    { ".^",     "A B -- C",
      "Element-wise exponentiation.",
      "A B .^" },

    /* --- Comparisons / logical --- */
    { "eq",     "a b -- bool",
      "Equality test (numeric or string).",
      "2 2 eq     (→ 1/true)" },

    { "leq",    "a b -- bool",
      "Less than or equal.",
      "2 3 leq    (→ 1)" },

    { "lt",     "a b -- bool",
      "Less than.",
      "2 3 lt     (→ 1)" },

    { "gt",     "a b -- bool",
      "Greater than.",
      "3 2 gt     (→ 1)" },

    { "geq",    "a b -- bool",
      "Greater than or equal.",
      "3 2 geq    (→ 1)" },

    { "neq",    "a b -- bool",
      "Not equal.",
      "2 3 neq    (→ 1)" },

    { "and",    "a b -- bool",
      "Logical AND on boolean-like values.",
      "1 0 and    (→ 0)" },

    { "or",     "a b -- bool",
      "Logical OR.",
      "1 0 or     (→ 1)" },

    { "not",    "a -- bool",
      "Logical NOT.",
      "0 not      (→ 1)" },

    /* --- Date / time functions --- */
    { "ddays",  "date2 date1 -- n_days",
      "Difference in days between two dates (date2 - date1).",
      "\"2025-12-31\" \"2025-01-01\" ddays" },

    { "today",  "-- date",
      "Push today's date as a string (YYYY-MM-DD).",
      "today" },

    { "dateplus","date n -- date2",
      "Add n days to a date.",
      "\"2025-01-01\" 30 dateplus" },

    { "dow",    "date -- n",
      "Day of week (e.g. 0=Sunday..6=Saturday; see implementation).",
      "\"2025-01-01\" dow" },

    { "edmy",   "date -- d m y",
      "Extract day, month, year from date.",
      "\"2025-01-01\" edmy" },

    { "num2date","n -- date",
      "Convert integer (days since some epoch) to date string.",
      "10000 num2date" },

    { "days2eoy","date -- n",
      "Number of days from given date to end of year.",
      "\"2025-01-01\" days2eoy" },

    /* --- User words / macros / history --- */
    { "listwords","--",
      "List stored user-defined words.",
      "listwords" },

    { "loadwords","filename --",
      "Load user words from file.",
      "\"words.mm\" loadwords" },

    { "savewords","filename --",
      "Save user words to file.",
      "\"words.mm\" savewords" },

    { "delword","name --",
      "Delete a user-defined word.",
      "\"FOO\" delword" },

    { "selword","name --",
      "Select and show definition of a user word.",
      "\"FOO\" selword" },

    { "clrwords","--",
      "Clear all user-defined words.",
      "clrwords" },

    { "listmacros","--",
      "List available macros/programs.",
      "listmacros" },

    { "clrhist","--",
      "Clear command/history buffer (and/or undo history).",
      "clrhist" },

    /* --- Top / counter tests (conditionals) --- */
    { "top_eq0?","x -- x",
      "Set condition flag if top == 0 (or branch in macro; see your control logic).",
      "0 top_eq0?" },

    { "top_ge0?","x -- x",
      "Condition: top >= 0.",
      "1 top_ge0?" },

    { "top_gt0?","x -- x",
      "Condition: top > 0.",
      "1 top_gt0?" },

    { "top_le0?","x -- x",
      "Condition: top <= 0.",
      "-1 top_le0?" },

    { "top_lt0?","x -- x",
      "Condition: top < 0.",
      "-1 top_lt0?" },

    { "top_eg?","x y -- x y",
      "Condition based on equality of two top items (exact semantics per code).",
      "a b top_eg?" },

    { "top_ge?","x y -- x y",
      "Condition: x >= y.",
      "a b top_ge?" },

    { "top_gt?","x y -- x y",
      "Condition: x > y.",
      "a b top_gt?" },

    { "top_le?","x y -- x y",
      "Condition: x <= y.",
      "a b top_le?" },

    { "top_lt?","x y -- x y",
      "Condition: x < y.",
      "a b top_lt?" },

    { "ctr_eq0?","--",
      "Condition: counter == 0.",
      "ctr_eq0?" },

    { "ctr_ge0?","--",
      "Condition: counter >= 0.",
      "ctr_ge0?" },

    { "ctr_gt0?","--",
      "Condition: counter > 0.",
      "ctr_gt0?" },

    { "ctr_le0?","--",
      "Condition: counter <= 0.",
      "ctr_le0?" },

    { "ctr_lt0?","--",
      "Condition: counter < 0.",
      "ctr_lt0?" },

    { "set_ctr","n --",
      "Set loop/condition counter to n.",
      "10 set_ctr" },

    { "clr_ctr","--",
      "Clear counter (set to zero).",
      "clr_ctr" },

    { "ctr_inc","--",
      "Increment counter by 1.",
      "ctr_inc" },

    { "ctr_dec","--",
      "Decrement counter by 1.",
      "ctr_dec" },

    /* --- Control flow / program structure --- */
    { "goto",   "label --",
      "Jump to label inside current macro/program.",
      "\"LOOP\" goto" },

    { "xeq",    "name --",
      "Execute named word/macro.",
      "\"FOO\" xeq" },

    { "rtn",    "--",
      "Return from current macro/program.",
      "rtn" },

    { "end",    "--",
      "Mark end of macro/program definition.",
      "end" },

    { "lbl",    "name --",
      "Define a label in a macro/program.",
      "\"LOOP\" lbl" },

    /* --- Evaluation / batch --- */
    { "eval",   "s --",
      "Evaluate string as mm_15 code.",
      "\"1 2 +\" eval" },

    { "batch",  "filename --",
      "Run commands from file.",
      "\"script.mm\" batch" },

    { "run",    "name --",
      "Run stored macro/program by name.",
      "\"MYMAC\" run" },

    /* --- Astronomy --- */
    { "sunrise","date lat lon utc_offset -- time_str",
      "Local sunrise time (HH:MM) for given date and location.",
      "\"2025-06-21\" 38.9 -77.0 -4 sunrise" },

    { "sunset", "date lat lon utc_offset -- time_str",
      "Local sunset time (HH:MM) for given date and location.",
      "\"2025-06-21\" 38.9 -77.0 -4 sunset" },

    /* Terminator */
    { NULL, NULL, NULL, NULL }
};

/* ----------------------------------------------------------
 *  "sin" usage support
 * ---------------------------------------------------------- */

/* Internal helper: find a help entry by name */
static const HelpEntry *find_help_entry(const char *name) {
    if (!name) return NULL;

    for (const HelpEntry *e = help_table; e->name != NULL; ++e) {
        if (strcmp(e->name, name) == 0) {
            return e;
        }
    }
    return NULL;
}

/* Public function: print "name" usage: ... */
void usage(const char *name) {
    if (!name || !*name) {
        fprintf(stderr, "usage: expected non-empty word name\n");
        skip_stack_printing = true;
        return;
    }

    const HelpEntry *e = find_help_entry(name);
    if (!e) {
        fprintf(stderr, "No usage information available for \"%s\".\n", name);
        skip_stack_printing = true;
        return;
    }

    /* First line: "sin" usage: x -- sin(x) */
    printf(BOLD "\"%s\" usage" RESET ": %s\n",
           e->name,
           e->stack_effect ? e->stack_effect : "");

    /* Optional extra lines */
    if (e->description && *e->description) {
        printf("    %s\n", e->description);
    }
    if (e->example && *e->example) {
        printf("    Example: %s\n", e->example);
    }

    /* Suppress automatic stack dump in main loop, like help_menu does */
    skip_stack_printing = true;
}

/*
 * RPN word: usage
 *
 * Stack effect (logical):  name -- 
 * where `name` is a TYPE_STRING containing the word name, e.g. "sin".
 *
 * Behavior:
 *   - Pops a string from the stack
 *   - Calls usage(name) to print `"name" usage: ...`
 *   - Frees the popped string
 *   - On error (wrong type / empty string), prints a message,
 *     restores the stack element, and returns 0.
 *
 * Return value:
 *   1 on success, 0 on error.
 */

int op_usage(Stack *stack)
{
    /* Need at least one item on the stack */
    GUARANTEE_STACK(stack, 1);

    stack_element e = pop(stack);

    if (e.type != TYPE_STRING) {
        /* Wrong type: restore element and complain */
        fprintf(stderr, "usage: expected a string with a word name on the stack.\n");

        /* Push it back based on its type so we don't silently lose data */
        switch (e.type) {
        case TYPE_REAL:
            push_real(stack, e.real);
            break;
        case TYPE_COMPLEX:
            push_complex(stack, e.complex_val);
            break;
        case TYPE_STRING:
            /* not possible here, but keep for completeness */
            push_string(stack, e.string);
            break;
        case TYPE_MATRIX_REAL:
            push_matrix_real(stack, e.matrix_real);
            break;
        case TYPE_MATRIX_COMPLEX:
            push_matrix_complex(stack, e.matrix_complex);
            break;
        default:
            /* Unknown type: nothing we can safely do */
            break;
        }

        return 0;
    }

    /* We have a string with the word name */
    const char *name = e.string ? e.string : "";

    if (name[0] == '\0') {
        fprintf(stderr, "usage: empty string is not a valid word name.\n");
        free(e.string);
        return 0;
    }

    /* Delegate to the help system */
    usage(name);

    /* The usage() function already sets skip_stack_printing,
       but it's harmless to enforce it here too if you want: */
    skip_stack_printing = true;

    /* We own the string memory, so free it after use */
    free(e.string);

    return 1;
}
