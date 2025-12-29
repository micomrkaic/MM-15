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

#include <ctype.h>          // for isdigit, isspace, isalnum, isalpha
#include <stdbool.h>        // for true, bool, false
#include <stdio.h>          // for size_t, snprintf, fprintf, stderr
#include <stdlib.h>         // for free, calloc, realloc
#include <string.h>         // for strncpy, memcpy, strcmp
#include "function_list.h"  // for function_names
#include "lexer.h"          // for Token, MAX_TOKEN_LEN, TOK_UNKNOWN, Lexer

#define TEMP_BUF_SIZE (MAX_TOKEN_LEN * 4 - 1)

void skip_whitespace(Lexer* lexer) {
  while (isspace((unsigned char)lexer->input[lexer->pos])) lexer->pos++;
}

char peek(Lexer* lexer) {
  return lexer->input[lexer->pos];
}

char advance(Lexer* lexer) {
  return lexer->input[lexer->pos++];
}

static inline char safe_peek_ahead(const Lexer *lx, size_t k)
{
  const char *p = lx->input + lx->pos;
  while (k-- && *p) {
    p++;
  }
  return *p;  // returns '\0' if we ran out
}

bool match(Lexer* lexer, char expected) {
  if (peek(lexer) == expected) {
    lexer->pos++;
    return true;
  }
  return false;
}

/* Accept:
 *   12, -12
 *   12.34, -12.34
 *   .9, -.3
 *   1e3, 1.2e-3, -.3E+2, .9e3
 */

static bool starts_number(const Lexer *lx)
{
  char c0 = safe_peek_ahead(lx, 0);
  if (c0 == '\0') return false;

  // digit => number
  if (isdigit((unsigned char)c0)) return true;

  // .digit => number
  if (c0 == '.') {
    char c1 = safe_peek_ahead(lx, 1);
    return isdigit((unsigned char)c1);
  }

  // +digit, -digit, +.digit, -.digit => number
  if (c0 == '+' || c0 == '-') {
    char c1 = safe_peek_ahead(lx, 1);
    if (isdigit((unsigned char)c1)) return true;
    if (c1 == '.') {
      char c2 = safe_peek_ahead(lx, 2);
      return isdigit((unsigned char)c2);
    }
  }

  return false;
}

Token make_token(token_type type, const char* text) {
  Token token;
  strncpy(token.text, text, MAX_TOKEN_LEN - 1);
  token.text[MAX_TOKEN_LEN - 1] = '\0';
  token.type = type;
  return token;
}

bool is_function_name(const char* name) {
  for (int i = 0; function_names[i]; i++) {
    if (strcmp(name, function_names[i]) == 0) return true;
  }
  return false;
}

Token lex_number(Lexer* lexer) {
  size_t start = lexer->pos;

  //  if (peek(lexer) == '-') advance(lexer);
  if (peek(lexer) == '-' || peek(lexer) == '+') advance(lexer);

  while (isdigit((unsigned char)peek(lexer))) advance(lexer);

  if (peek(lexer) == '.') {
    advance(lexer);
    while (isdigit((unsigned char)peek(lexer))) advance(lexer);
  }

  if (peek(lexer) == 'e' || peek(lexer) == 'E') {
    advance(lexer);
    if (peek(lexer) == '+' || peek(lexer) == '-') advance(lexer);
    while (isdigit((unsigned char)peek(lexer))) advance(lexer);
  }

  size_t len = lexer->pos - start;

  char buf[MAX_TOKEN_LEN] = {0};
  size_t copy_len = (len >= (MAX_TOKEN_LEN - 1)) ? (MAX_TOKEN_LEN - 1) : len;
  strncpy(buf, &lexer->input[start], copy_len);
  buf[copy_len] = '\0';

  return make_token(TOK_NUMBER, buf);
}

Token lex_identifier(Lexer* lexer) {
  size_t start = lexer->pos;
  while (isalnum((unsigned char)peek(lexer)) || peek(lexer) == '_') advance(lexer);

  size_t len = lexer->pos - start;

  char buf[MAX_TOKEN_LEN] = {0};
  size_t copy_len = (len >= (MAX_TOKEN_LEN - 1)) ? (MAX_TOKEN_LEN - 1) : len;
  strncpy(buf, &lexer->input[start], copy_len);
  buf[copy_len] = '\0';

  return make_token(is_function_name(buf) ? TOK_FUNCTION : TOK_IDENTIFIER, buf);
}

Token lex_string(Lexer* lexer) {
  advance(lexer);
  size_t start = lexer->pos;
  while (peek(lexer) != '"' && peek(lexer) != '\0') advance(lexer);

  size_t len = lexer->pos - start;

  char buf[MAX_TOKEN_LEN] = {0};
  size_t copy_len = (len >= (MAX_TOKEN_LEN - 1)) ? (MAX_TOKEN_LEN - 1) : len;
  strncpy(buf, &lexer->input[start], copy_len);
  buf[copy_len] = '\0';

  match(lexer, '"');
  return make_token(TOK_STRING, buf);
}

Token lex_complex(Lexer* lexer) {
  size_t start_pos = lexer->pos;

  if (!match(lexer, '(')) return make_token(TOK_UNKNOWN, "(");

  Token real = lex_number(lexer);

  if (!match(lexer, ',')) {
    lexer->pos = start_pos;
    return make_token(TOK_UNKNOWN, "(");
  }

  Token imag = lex_number(lexer);

  if (!match(lexer, ')')) {
    lexer->pos = start_pos;
    return make_token(TOK_UNKNOWN, "(");
  }

  char temp_buf[TEMP_BUF_SIZE];
  snprintf(temp_buf, sizeof(temp_buf), "(%s,%s)", real.text, imag.text);

  return make_token(TOK_COMPLEX, temp_buf);
}

Token lex_matrix_file(Lexer* lexer) {
  size_t start_pos = lexer->pos;

  Token row = lex_number(lexer);
  if (!match(lexer, ',')) { lexer->pos = start_pos; return make_token(TOK_UNKNOWN, "["); }

  Token col = lex_number(lexer);
  if (!match(lexer, ',')) { lexer->pos = start_pos; return make_token(TOK_UNKNOWN, "["); }

  Token str = lex_string(lexer);
  if (!match(lexer, ']')) { lexer->pos = start_pos; return make_token(TOK_UNKNOWN, "["); }

  char temp_buf[TEMP_BUF_SIZE];
  snprintf(temp_buf, sizeof(temp_buf), "[%s,%s,\"%s\"]", row.text, col.text, str.text);

  return make_token(TOK_MATRIX_FILE, temp_buf);
}

static Token lex_matrix_inline_j(Lexer* lexer) {
  size_t start_pos = lexer->pos;

  // Parse row and column counts
  Token rows = lex_number(lexer);
  if (rows.type != TOK_NUMBER) {
    lexer->pos = start_pos;
    return make_token(TOK_UNKNOWN, "[");
  }

  skip_whitespace(lexer);

  Token cols = lex_number(lexer);
  if (cols.type != TOK_NUMBER) {
    lexer->pos = start_pos;
    return make_token(TOK_UNKNOWN, "[");
  }

  skip_whitespace(lexer);

  if (!match(lexer, '$')) {
    lexer->pos = start_pos;
    return make_token(TOK_UNKNOWN, "[");
  }

  skip_whitespace(lexer);

  // Dynamic buffer for accumulating matrix content
  size_t cap = 256;
  size_t len = 0;
  char* buf = calloc(cap, 1);
  if (!buf) return make_token(TOK_UNKNOWN, "["); // Allocation failed

  // Prepend "rows cols $"
  int written = snprintf(buf, cap, "%s %s $", rows.text, cols.text);
  if (written < 0 || (size_t)written >= cap) {
    free(buf);
    return make_token(TOK_UNKNOWN, "[");
  }
  len = (size_t)written;

  bool has_real = false;
  bool has_complex = false;
  char temp[MAX_TOKEN_LEN * 2];

  while (peek(lexer) != '\0' && peek(lexer) != ']') {
    skip_whitespace(lexer);

    Token t;
    if (peek(lexer) == '(') {
      t = lex_complex(lexer);
      has_complex = true;
    } else if (starts_number(lexer)) {
      t = lex_number(lexer);
      has_real = true;
    } else {
      break;
    }

    written = snprintf(temp, sizeof(temp), " %s", t.text);
    if (written < 0 || (size_t)written >= sizeof(temp)) {
      fprintf(stderr, "token too long to format\n");
      continue;
    }

    if (len + (size_t)written + 1 >= cap) {
      cap *= 2;
      char* new_buf = realloc(buf, cap);
      if (!new_buf) {
        free(buf);
        return make_token(TOK_UNKNOWN, "[");
      }
      buf = new_buf;
    }

    memcpy(buf + len, temp, (size_t)written + 1);
    len += (size_t)written;

    skip_whitespace(lexer);
  }

  if (!match(lexer, ']')) {
    free(buf);
    lexer->pos = start_pos;
    return make_token(TOK_UNKNOWN, "[");
  }

  token_type type = has_complex && has_real ? TOK_MATRIX_INLINE_MIXED
    : has_complex          ? TOK_MATRIX_INLINE_COMPLEX
    :                        TOK_MATRIX_INLINE_REAL;

  Token result = make_token(type, buf);
  free(buf);
  return result;
}

Token next_token(Lexer* lexer) {
  skip_whitespace(lexer);

  char c = peek(lexer);
  if (c == '\0') return make_token(TOK_EOF, "<EOF>");

  if (starts_number(lexer)) return lex_number(lexer);
  if (c == '(') return lex_complex(lexer);

  if (c == '[') {
    size_t start_pos = lexer->pos;
    size_t temp_pos = lexer->pos + 1; // after '['

    // local whitespace skip for lookahead
    while (isspace((unsigned char)lexer->input[temp_pos])) temp_pos++;

    // If it looks like "[<number>,", treat as matrix-file form.
    // Accept numbers that can start with digit, '.', '-', or '-.'
    Lexer look = { .input = lexer->input, .pos = temp_pos };
    if (starts_number(&look)) {
      // Consume a number-like span (coarse, but adequate lookahead)
      while (isdigit((unsigned char)lexer->input[temp_pos]) ||
             lexer->input[temp_pos] == '.' ||
             lexer->input[temp_pos] == '-' ||
             lexer->input[temp_pos] == 'e' ||
             lexer->input[temp_pos] == 'E' ||
             lexer->input[temp_pos] == '+') {
        temp_pos++;
      }
      while (isspace((unsigned char)lexer->input[temp_pos])) temp_pos++;

      if (lexer->input[temp_pos] == ',') {
        advance(lexer); // Eat '['
        return lex_matrix_file(lexer);
      }
    }

    // otherwise inline
    (void)start_pos;
    advance(lexer); // Eat '['
    return lex_matrix_inline_j(lexer);
  }

  if (isalpha((unsigned char)c) || c == '_') return lex_identifier(lexer);
  if (c == '"') return lex_string(lexer);

  // Handle multi-character operators
  if (c == '.' && lexer->input[lexer->pos + 1] == '*') {
    lexer->pos += 2;
    return make_token(TOK_DOT_STAR, ".*");
  }
  if (c == '.' && lexer->input[lexer->pos + 1] == '/') {
    lexer->pos += 2;
    return make_token(TOK_DOT_SLASH, "./");
  }
  if (c == '.' && lexer->input[lexer->pos + 1] == '^') {
    lexer->pos += 2;
    return make_token(TOK_DOT_CARET, ".^");
  }

  switch (c) {
  case '+': advance(lexer); return make_token(TOK_PLUS, "+");
  case '-': advance(lexer); return make_token(TOK_MINUS, "-");
  case '*': advance(lexer); return make_token(TOK_STAR, "*");
  case '/': advance(lexer); return make_token(TOK_SLASH, "/");
  case '^': advance(lexer); return make_token(TOK_CARET, "^");
  case '<': advance(lexer); return make_token(TOK_BRA, "<");
  case '>': advance(lexer); return make_token(TOK_KET, ">");
  case '|': advance(lexer); return make_token(TOK_VERTICAL, "|");
  case ':': advance(lexer); return make_token(TOK_COLON, ":");
  case ';': advance(lexer); return make_token(TOK_SEMICOLON, ";");
  case '\'': advance(lexer); return make_token(TOK_FUNCTION, "'");
  default: {
    char unknown_char = advance(lexer);
    char unk[2] = { unknown_char, '\0' };
    return make_token(TOK_UNKNOWN, unk);
  }
  }
}

const char* token_type_str(token_type type) {
  switch (type) {
  case TOK_EOF: return "EOF";
  case TOK_NUMBER: return "NUMBER";
  case TOK_COMPLEX: return "COMPLEX";
  case TOK_STRING: return "STRING";
  case TOK_MATRIX_FILE: return "MATRIX_FILE";
  case TOK_MATRIX_INLINE_REAL: return "MATRIX_INLINE_REAL";
  case TOK_MATRIX_INLINE_COMPLEX: return "MATRIX_INLINE_COMPLEX";
  case TOK_MATRIX_INLINE_MIXED: return "MATRIX_INLINE_MIXED";
  case TOK_PLUS: return "PLUS";
  case TOK_MINUS: return "MINUS";
  case TOK_STAR: return "STAR";
  case TOK_SLASH: return "SLASH";
  case TOK_CARET: return "CARET";
  case TOK_BRA: return "BRA";
  case TOK_KET: return "KET";
  case TOK_IDENTIFIER: return "IDENTIFIER";
  case TOK_FUNCTION: return "FUNCTION";
  case TOK_VERTICAL: return "VERTICAL";
  default: return "UNKNOWN";
  }
}

