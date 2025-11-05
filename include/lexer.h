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

#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <math.h>
#include <readline/history.h>
#include <readline/readline.h>

#define MAX_TOKEN_LEN 1024
#define MAX_INPUT_LEN 4096
#define MAX_SUBTOKEN_LEN 100

typedef enum {
  TOK_EOF,
  TOK_NUMBER,
  TOK_COMPLEX,
  TOK_STRING,
  TOK_MATRIX_FILE,
  TOK_MATRIX_INLINE_REAL,
  TOK_MATRIX_INLINE_COMPLEX,
  TOK_MATRIX_INLINE_MIXED,
  TOK_PLUS,
  TOK_MINUS,
  TOK_STAR,
  TOK_SLASH,
  TOK_CARET,
  TOK_DOT_STAR,
  TOK_DOT_SLASH,
  TOK_DOT_CARET,  
  TOK_BRA,
  TOK_KET,
  TOK_COLON,
  TOK_SEMICOLON,
  TOK_IDENTIFIER,
  TOK_FUNCTION,
  TOK_VERTICAL,
  TOK_UNKNOWN,
} token_type;

typedef struct {
  token_type type;
  char text[MAX_TOKEN_LEN];
} Token;

typedef struct {
  const char* input;
  size_t pos;
} Lexer;

void skip_whitespace(Lexer* lexer);
char peek(Lexer* lexer);
char advance(Lexer* lexer);
bool match(Lexer* lexer, char expected);
Token make_token(token_type type, const char* text);
bool is_function_name(const char* name);
Token lex_number(Lexer* lexer);
Token lex_identifier(Lexer* lexer);
Token lex_string(Lexer* lexer);
Token lex_complex(Lexer* lexer);
Token lex_matrix_file(Lexer* lexer);
Token lex_matrix_inline(Lexer* lexer);
Token next_token(Lexer* lexer);
const char* token_type_str(token_type type);

#endif // LEXER_H
