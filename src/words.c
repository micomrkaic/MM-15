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
#include "errors.h"
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "eval_fun.h"
#include "globals.h"
#include "lexer.h"
#include "ops.h"
#include "words.h"

user_word *words = NULL;
int word_count = 0;
static int word_cap = 0;
user_word *macros = NULL;
int macro_count = 0;
static int macro_cap = 0;

static int word_depth = 0;     /* current nesting of running words */

static const char *control_words[] = {
  "if", "else", "then", "begin", "until", "while", "repeat",
  "do", "loop", "+loop", "i", "j", NULL
};

bool is_control_word(const char *t) {
  for (int i = 0; control_words[i]; i++)
    if (strcmp(t, control_words[i]) == 0) return true;
  return false;
}

/* ======================= lookup ======================= */

static user_word *find_in(user_word *tab, int n, const char *name) {
  for (int i = 0; i < n; i++)
    if (strcmp(tab[i].name, name) == 0) return &tab[i];
  return NULL;
}

user_word *find_word(const char *name) {
  user_word *w = find_in(words, word_count, name);
  return w ? w : find_in(macros, macro_count, name);
}

static void free_word(user_word *w) {
  free(w->body);
  free(w->code);
  w->body = NULL;
  w->code = NULL;
  w->ncode = 0;
}

/* ======================= compiler ======================= */

typedef enum { C_IF, C_ELSE, C_BEGIN, C_WHILE, C_DO } ctl_kind;
typedef struct { ctl_kind k; int pc; } ctl_entry;

static const char *ctl_name(ctl_kind k) {
  switch (k) {
  case C_IF: return "if"; case C_ELSE: return "else"; case C_BEGIN: return "begin";
  case C_WHILE: return "while"; case C_DO: return "do";
  }
  return "?";
}

/* Compile body into *out. Identifiers must be known words (or self). */
static int compile(const char *self, const char *body, WInstr **out, int *nout) {
  int cap = 32, pc = 0, nctl = 0, ndo = 0, rc = 0;
  WInstr *code = malloc((size_t)cap * sizeof *code);
  if (!code) return mm_error("Out of memory");
  ctl_entry ctl[64];
  Lexer lx = { .input = body, .pos = 0 };

#define FAIL(...) do { mm_error(__VA_ARGS__); rc = -1; goto done; } while (0)
#define EMIT(kind_, target_) do {                                           \
    if (pc >= cap) {                                                        \
      cap *= 2;                                                             \
      WInstr *n_ = realloc(code, (size_t)cap * sizeof *n_);                 \
      if (!n_) { FAIL("Out of memory"); }                                   \
      code = n_;                                                            \
    }                                                                       \
    code[pc].kind = (kind_); code[pc].target = (target_); code[pc].tok = tok; pc++; } while (0)
#define NEED(kind_, name_) do { if (nctl == 0 || ctl[nctl - 1].k != (kind_)) \
    FAIL("%s: '%s' without matching '%s'", self, name_, ctl_name(kind_)); } while (0)
#define PUSHCTL(kind_) do { if (nctl >= 64) FAIL("%s: control structures nested too deeply", self); \
    ctl[nctl].k = (kind_); ctl[nctl].pc = pc; nctl++; } while (0)

  for (;;) {
    Token tok = next_token(&lx);
    if (tok.type == TOK_EOF) break;
    if (tok.type == TOK_UNKNOWN) FAIL("%s: illegal token '%s'", self, tok.text);
    if (tok.type == TOK_COLON || tok.type == TOK_SEMICOLON)
      FAIL("%s: ':' and ';' cannot appear inside a definition", self);
    const char *t = tok.text;
    if (tok.type != TOK_IDENTIFIER && tok.type != TOK_FUNCTION) { EMIT(W_TOKEN, -1); continue; }

    if (!strcmp(t, "if")) {
      PUSHCTL(C_IF); EMIT(W_BRANCH0, -1);
    } else if (!strcmp(t, "else")) {
      NEED(C_IF, t);
      EMIT(W_BRANCH, -1);
      code[ctl[nctl - 1].pc].target = pc;
      ctl[nctl - 1].k = C_ELSE; ctl[nctl - 1].pc = pc - 1;
    } else if (!strcmp(t, "then")) {
      if (nctl == 0 || (ctl[nctl - 1].k != C_IF && ctl[nctl - 1].k != C_ELSE))
        FAIL("%s: 'then' without matching 'if'", self);
      code[ctl[--nctl].pc].target = pc;
    } else if (!strcmp(t, "begin")) {
      PUSHCTL(C_BEGIN);
    } else if (!strcmp(t, "until")) {
      NEED(C_BEGIN, t);
      EMIT(W_BRANCH0, ctl[--nctl].pc);
    } else if (!strcmp(t, "while")) {
      NEED(C_BEGIN, t);
      PUSHCTL(C_WHILE); EMIT(W_BRANCH0, -1);
    } else if (!strcmp(t, "repeat")) {
      NEED(C_WHILE, t);
      int wpc = ctl[--nctl].pc;
      int bpc = ctl[--nctl].pc;
      EMIT(W_BRANCH, bpc);
      code[wpc].target = pc;
    } else if (!strcmp(t, "do")) {
      if (ndo >= MAX_LOOP_NEST) FAIL("%s: do loops nested deeper than %d", self, MAX_LOOP_NEST);
      PUSHCTL(C_DO); EMIT(W_DO, -1); ndo++;
    } else if (!strcmp(t, "loop") || !strcmp(t, "+loop")) {
      NEED(C_DO, t);
      int dpc = ctl[--nctl].pc;
      EMIT(t[0] == '+' ? W_PLOOP : W_LOOP, dpc + 1);
      code[dpc].target = pc;
      ndo--;
    } else if (!strcmp(t, "i")) {
      if (ndo < 1) FAIL("%s: 'i' outside a do loop", self);
      EMIT(W_I, -1);
    } else if (!strcmp(t, "j")) {
      if (ndo < 2) FAIL("%s: 'j' needs two nested do loops", self);
      EMIT(W_J, -1);
    } else if (tok.type == TOK_FUNCTION) {
      EMIT(W_TOKEN, -1);
    } else {
      if (strcmp(t, self) != 0 && !find_word(t))
        FAIL("%s: undefined word '%s'", self, t);
      EMIT(W_TOKEN, -1);
    }
  }
  if (nctl > 0) FAIL("%s: unterminated '%s'", self, ctl_name(ctl[nctl - 1].k));
done:
  if (rc != 0) { free(code); return -1; }
  *out = code; *nout = pc;
  return 0;
#undef FAIL
#undef EMIT
#undef NEED
#undef PUSHCTL
}

/* ======================= definition ======================= */

static int validate_name(const char *name) {
  size_t l = strlen(name);
  if (l == 0) return mm_error("Missing word name");
  if (l >= MAX_WORD_NAME) return mm_error("Word name '%s' longer than %d characters", name, MAX_WORD_NAME - 1);
  for (const char *p = name; *p; p++)
    if (!isalnum((unsigned char)*p) && *p != '_')
      return mm_error("Word name '%s' may only contain letters, digits and '_'", name);
  if (isdigit((unsigned char)name[0])) return mm_error("Word name '%s' cannot start with a digit", name);
  if (find_op(name) >= 0) return mm_error("'%s' is a built-in command", name);
  if (is_control_word(name)) return mm_error("'%s' is a control word", name);
  return 0;
}

static char *normalise(const char *body) {
  /* collapse whitespace runs and trim, so listwords/savewords are tidy */
  size_t n = strlen(body);
  char *out = malloc(n + 1);
  if (!out) return NULL;
  size_t o = 0; bool sp = true;
  for (size_t i = 0; i < n; i++) {
    if (isspace((unsigned char)body[i])) { if (!sp) { out[o++] = ' '; sp = true; } }
    else { out[o++] = body[i]; sp = false; }
  }
  while (o > 0 && out[o - 1] == ' ') o--;
  out[o] = '\0';
  return out;
}

static int grow(user_word **tab, int *cap, int count) {
  if (count < *cap) return 0;
  int nc = *cap ? 2 * *cap : 32;
  user_word *n = realloc(*tab, (size_t)nc * sizeof *n);
  if (!n) return mm_error("Out of memory");
  *tab = n; *cap = nc;
  return 0;
}

int define_word(const char *name, const char *body, bool is_macro) {
  if (word_depth > 0) return mm_error("Cannot define a word while a word is running");
  if (validate_name(name) != 0) return -1;
  WInstr *code; int ncode;
  if (compile(name, body, &code, &ncode) != 0) return -1;
  char *norm = normalise(body);
  if (!norm) { free(code); return mm_error("Out of memory"); }

  user_word **tab = is_macro ? &macros : &words;
  int *count = is_macro ? &macro_count : &word_count;
  int *cap = is_macro ? &macro_cap : &word_cap;
  user_word *w = find_in(*tab, *count, name);
  if (w) {
    free_word(w);
  } else {
    if (grow(tab, cap, *count) != 0) { free(code); free(norm); return -1; }
    w = &(*tab)[(*count)++];
    memset(w, 0, sizeof *w);
    snprintf(w->name, sizeof w->name, "%s", name);
  }
  w->body = norm; w->code = code; w->ncode = ncode; w->is_macro = is_macro;
  if (!is_macro && (!batch_mode || verbose_mode)) printf("New word %s <- %s\n", w->name, w->body);
  return 0;
}

/* ": name body ;" on one line. */
int is_word_definition(const char *s) {
  while (isspace((unsigned char)*s)) s++;
  if (*s != ':') return 0;
  s++;
  while (isspace((unsigned char)*s)) s++;
  const char *ns = s;
  while (*s && !isspace((unsigned char)*s)) s++;
  char name[MAX_WORD_NAME + 32];
  size_t nl = (size_t)(s - ns);
  if (nl >= sizeof name) nl = sizeof name - 1;
  memcpy(name, ns, nl); name[nl] = '\0';
  if (nl == 0) { mm_error("Missing word name after ':'"); return -1; }

  const char *end = s + strlen(s);
  while (end > s && isspace((unsigned char)end[-1])) end--;
  if (end == s || end[-1] != ';') { mm_error("Definition of '%s' must end with ';'", name); return -1; }
  end--;
  char *body = strndup(s, (size_t)(end - s));
  if (!body) { mm_error("Out of memory"); return -1; }
  int rc = define_word(name, body, false);
  free(body);
  return rc == 0 ? 1 : -1;
}

/* ======================= execution ======================= */

int exec_word(Stack *stack, user_word *w) {
  if (word_depth >= MAX_WORD_DEPTH)
    return mm_error("Runaway recursion: word nesting deeper than %d (in '%s')", MAX_WORD_DEPTH, w->name);
  struct { double index, limit; } lp[MAX_LOOP_NEST];
  int nl = 0;
  long errors_before = error_count;
  word_depth++;
  for (int pc = 0; pc < w->ncode && error_count == errors_before; ) {
    WInstr *in = &w->code[pc];
    switch (in->kind) {
    case W_TOKEN:
      evaluate_one_token(stack, in->tok);
      pc++;
      break;
    case W_BRANCH:
      pc = in->target;
      break;
    case W_BRANCH0: {
      if (stack->top < 0 || stack->items[stack->top].type != TYPE_REAL) {
        mm_error("%s: conditional needs a real flag on the stack", w->name);
        break;
      }
      double f = pop(stack).real;
      pc = (f == 0.0) ? in->target : pc + 1;
      break;
    }
    case W_DO: {
      if (stack->top < 1 || stack->items[stack->top].type != TYPE_REAL ||
          stack->items[stack->top - 1].type != TYPE_REAL) {
        mm_error("%s: do needs real limit and start", w->name);
        break;
      }
      double start = pop(stack).real, limit = pop(stack).real;
      if (start == limit) { pc = in->target; break; }
      lp[nl].index = start; lp[nl].limit = limit; nl++;
      pc++;
      break;
    }
    case W_LOOP:
      lp[nl - 1].index += 1.0;
      if (lp[nl - 1].index < lp[nl - 1].limit) pc = in->target; else { nl--; pc++; }
      break;
    case W_PLOOP: {
      if (stack->top < 0 || stack->items[stack->top].type != TYPE_REAL) {
        mm_error("%s: +loop needs a real increment", w->name);
        break;
      }
      double inc = pop(stack).real;
      lp[nl - 1].index += inc;
      bool again = inc >= 0 ? lp[nl - 1].index < lp[nl - 1].limit : lp[nl - 1].index >= lp[nl - 1].limit;
      if (again) pc = in->target; else { nl--; pc++; }
      break;
    }
    case W_I: push_real(stack, lp[nl - 1].index); pc++; break;
    case W_J: push_real(stack, lp[nl - 2].index); pc++; break;
    }
  }
  word_depth--;
  if (error_count != errors_before && word_depth == 0) {
    fflush(stdout);
    fprintf(stderr, "  (in word '%s')\n", w->name);
  }
  return error_count == errors_before ? 0 : -1;
}

/* ======================= listing / deleting ======================= */

static void list_table(user_word *tab, int n, const char *what) {
  if (n == 0) { printf("  (no %s defined)\n", what); return; }
  for (int i = 0; i < n; i++) printf("%2d. %16s: %s\n", i, tab[i].name, tab[i].body);
}

void list_words(void)  { list_table(words, word_count, "words"); }
void list_macros(void) { list_table(macros, macro_count, "macros"); }

void delete_word(Stack *stack) {
  if (word_depth > 0) { mm_error("delword cannot run inside a word"); return; }
  stack_element a = pop(stack);
  if (a.type != TYPE_REAL || !isfinite(a.real) || a.real != (int)a.real) {
    stack_element_free(&a);
    mm_error("delword: needs the word's number from listwords");
    return;
  }
  int index = (int)a.real;
  if (index < 0 || index >= word_count) { mm_error("delword: no word number %d", index); return; }
  printf("Deleted %s\n", words[index].name);
  free_word(&words[index]);
  memmove(&words[index], &words[index + 1], (size_t)(word_count - index - 1) * sizeof *words);
  word_count--;
}

void clear_words(void) {
  if (word_depth > 0) { mm_error("clrwords cannot run inside a word"); return; }
  for (int i = 0; i < word_count; i++) free_word(&words[i]);
  word_count = 0;
}

void free_words(void) {
  clear_words();
  for (int i = 0; i < macro_count; i++) free_word(&macros[i]);
  free(words); free(macros);
  words = macros = NULL;
  word_count = macro_count = word_cap = macro_cap = 0;
}

/* ======================= files ======================= */

int save_words_to_file(void) {
  const char *path = WORDS_PATH;
  FILE *f = fopen(path, "w");
  if (!f) return mm_perror(path);
  fprintf(f, "# MM-15 user words: name followed by the body\n");
  for (int i = 0; i < word_count; i++) fprintf(f, "%s %s\n", words[i].name, words[i].body);
  if (fclose(f) != 0) return mm_perror(path);
  printf("%d word%s saved\n", word_count, word_count == 1 ? "" : "s");
  return 0;
}

/* Two passes so a body may refer to a word defined later in the file:
   register every name with an empty body, then compile each body. Entries
   that fail to compile are dropped with their line number. */
static int load_table(const char *path, bool is_macro, bool quiet_if_missing) {
  if (word_depth > 0) return mm_error("Cannot load words while a word is running");
  FILE *f = fopen(path, "r");
  if (!f) {
    if (quiet_if_missing && errno == ENOENT) return 0;
    return mm_perror(path);
  }
  user_word **tab = is_macro ? &macros : &words;
  int *count = is_macro ? &macro_count : &word_count;
  int *cap = is_macro ? &macro_cap : &word_cap;
  for (int i = 0; i < *count; i++) free_word(&(*tab)[i]);
  *count = 0;

  struct { char *name, *body; int line; } *ent = NULL;
  int nent = 0, ecap = 0, lineno = 0, rc = 0;
  char *line = NULL; size_t lcap = 0; ssize_t n;
  while ((n = getline(&line, &lcap, f)) != -1) {
    lineno++;
    char *p = line;
    while (isspace((unsigned char)*p)) p++;
    if (!*p || *p == '#') continue;
    char *ns = p;
    while (*p && !isspace((unsigned char)*p)) p++;
    char *name = strndup(ns, (size_t)(p - ns));
    while (isspace((unsigned char)*p)) p++;
    char *e = p + strlen(p);
    while (e > p && isspace((unsigned char)e[-1])) e--;
    char *body = strndup(p, (size_t)(e - p));
    if (!name || !body) { free(name); free(body); rc = mm_error("Out of memory"); break; }
    if (validate_name(name) != 0) {
      fprintf(stderr, "  (%s, line %d)\n", path, lineno);
      free(name); free(body); rc = -1; continue;
    }
    if (nent == ecap) {
      ecap = ecap ? 2 * ecap : 32;
      void *nn = realloc(ent, (size_t)ecap * sizeof *ent);
      if (!nn) { free(name); free(body); rc = mm_error("Out of memory"); break; }
      ent = nn;
    }
    ent[nent].name = name; ent[nent].body = body; ent[nent].line = lineno; nent++;
    /* pass 1: register the name with an empty body so references resolve */
    if (grow(tab, cap, *count) == 0) {
      user_word *w = find_in(*tab, *count, name);
      if (!w) {
        w = &(*tab)[(*count)++];
        memset(w, 0, sizeof *w);
        snprintf(w->name, sizeof w->name, "%s", name);
        w->body = strdup("");
        w->is_macro = is_macro;
      }
    }
  }
  free(line);
  fclose(f);

  /* pass 2: compile; drop what fails */
  for (int k = 0; k < nent; k++) {
    user_word *w = find_in(*tab, *count, ent[k].name);
    WInstr *code; int ncode;
    if (w && compile(ent[k].name, ent[k].body, &code, &ncode) == 0) {
      free(w->body); free(w->code);
      w->body = normalise(ent[k].body); w->code = code; w->ncode = ncode;
    } else if (w) {
      fprintf(stderr, "  (%s, line %d)\n", path, ent[k].line);
      rc = -1;
      int idx = (int)(w - *tab);
      free_word(w);
      memmove(&(*tab)[idx], &(*tab)[idx + 1], (size_t)(*count - idx - 1) * sizeof **tab);
      (*count)--;
    }
    free(ent[k].name); free(ent[k].body);
  }
  free(ent);
  return rc;
}

int load_words_from_file(void)  { return load_table(WORDS_PATH, false, false); }
int autoload_words(void)        { return load_table(WORDS_PATH, false, true); }
int load_macros_from_file(void) { return load_table(MACROS_PATH, true, true); }
