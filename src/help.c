#define _POSIX_C_SOURCE 200112L
#ifdef __APPLE__
#define _DARWIN_C_SOURCE
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

#include "errors.h"
#include <stdbool.h>        // for true
#include <stdio.h>          // for printf, fprintf, NULL, stderr
#include <stdlib.h>         // for free, malloc, qsort
#include <string.h>         // for strcmp
#include "ops.h"            // for ops, find_op
#include "words.h"          // for find_word
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
  printf("    npv, irr, ddays, dateplus, today, now, tz_offset, days2eoy, dow, edmy, num2date \n");
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

void list_all_functions_sorted(void) {
  list_ops_by_category();
}

/* Public function: print "name" usage: ... */
void usage(const char *name) {
    if (!name || !*name) {
        mm_error("usage: expected non-empty word name\n");
        skip_stack_printing = true;
        return;
    }

    int idx = find_op(name);
    if (idx < 0) {
        user_word *w = find_word(name);
        if (w) { printf(": %s %s ;\n", w->name, w->body); skip_stack_printing = true; return; }
        mm_error("No usage information available for \"%s\".\n", name);
        skip_stack_printing = true;
        return;
    }
    const Op *e = &ops[idx];

    /* First line: "sin" usage: x -- sin(x) */
    printf(BOLD "\"%s\" usage" RESET ": %s   [%s]\n", e->name, e->effect, e->cat);
    if (e->help[0])    printf("    %s\n", e->help);
    if (e->example[0]) printf("    Example: %s\n", e->example);

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
        mm_error("usage: expected a string with a word name on the stack.\n");

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
        mm_error("usage: empty string is not a valid word name.\n");
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
