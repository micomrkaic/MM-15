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
  printf("\n");
  title("MM-15 " MM15_VERSION " -- RPN calculator for real and complex scalars, strings and matrices");
  subtitle("Entering data");
  printf("    Numbers 3  -1.2e-4   complex (1,2)   strings \"text\"   all input is case sensitive\n");
  printf("    Matrices [rows cols $ a11 a12 ...]  e.g. [2 2 $ 1 2 3 4];  from a file [2, 2, \"m.txt\"]\n");
  printf("    Comments start with \\ or #.  undo restores the stack before the previous line.\n");
  subtitle("Stack");
  printf("    dup drop swap over nip tuck  n pick  n roll  depth  clst  ps  print  pm\n");
  subtitle("Arithmetic and functions");
  printf("    + - * / ^  chs inv abs sqrt mod fact ncr npr  %% %%t %%chg   .* ./ .^ (elementwise)\n");
  printf("    exp ln log  sin cos tan asin acos atan  sinh cosh tanh asinh acosh atanh\n");
  printf("    re im arg conj re2c split_c j2r   npdf ncdf nquant gamma ln_gamma beta ln_beta\n");
  printf("    Real arguments outside the real domain promote to complex: -1 sqrt is (0,1).\n");
  subtitle("Matrices");
  printf("    tran ' minv pinv det eig chol svd kron  eye ones zeroes rand randn rrange\n");
  printf("    dim reshape get_aij set_aij split_mat diag to_diag join_v join_h\n");
  printf("    csum rsum cmean rmean cvar rvar cmin rmin cmax rmax cumsum_r cumsum_c  roots pval\n");
  subtitle("Words (user-defined commands)");
  printf("    : name body ;     e.g.  : sq dup * ;     then  5 sq\n");
  printf("    inside a word: if else then   begin until   begin while repeat   do loop +loop  i j\n");
  printf("    listwords  \"name\" usage  \"name\" delword  savewords  listmacros\n");
  subtitle("Numerical methods (@name pushes a function token)");
  printf("    a b @f integrate   a b @f romberg   a b @f fzero   x @f solve   a b @f minimize\n");
  printf("    x @f deriv   lo hi @f fsum   lo hi @f fprod   @f exec   x tol   nevals  interr\n");
  subtitle("Registers, dates, strings, programs");
  printf("    x n sto  n rcl  pr  ffr  saveregs loadregs      today now dow dateplus ddays sunrise ...\n");
  printf("    scon substr s2u s2l slen srev int2str eval      \"file\" run   \"file\" batch   !shell command\n");
  subtitle("More help");
  printf("    listfcns            every command by category, with stack pictures\n");
  printf("    \"name\" usage        one command in detail, or a word's definition\n");
  printf("    man mm_15, and the manual in the share/doc directory\n");
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
