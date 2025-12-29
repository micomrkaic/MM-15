/*
 * This file is part of MM-14.
 *
 * MM-14 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * MM-14 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with MM-14 If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef WORDS_H
#define WORDS_H

#define MAX_WORDS 64
#define MAX_WORD_NAME 16
#define MAX_WORD_BODY 1024
#define MAX_MACROS 64

#include <stdbool.h>
#include "stack.h"

typedef struct {
  char name[MAX_WORD_NAME];
  char body[MAX_WORD_BODY];
} user_word;

extern user_word words[MAX_WORDS];
extern int word_count;

extern user_word macros[MAX_WORDS];
extern int macro_count;

// Macros functions
void list_macros(void);
int load_macros_from_file(void);
user_word* find_macro(char* name);

// Words functions
void list_words(void);
void delete_word(Stack *stack);
int delete_word_by_index(int index);
void clear_words(void);
int save_words_to_file(void);
int load_words_from_file(void);
user_word* find_word(char* name);
void word_select(Stack *stack);
int is_word_definition(const char *s);

#endif // WORDS_H
