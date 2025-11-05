/*
 * This file is part of Mico's MM-15 Calculator
 *
 * Mico's MM-15 Calculator is free software: 
 * you can redistribute it and/or modify
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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include "globals.h"
#include "words.h"

user_word words[MAX_WORDS];
int word_count = 0;

user_word macros[MAX_WORDS];
int macro_count = 0;

// Macros files
void list_macros(void) {
  if (macro_count > 0) {
    for(int i=0; i< macro_count; i++) {
      printf("%2d. %16s: %s\n",i,macros[i].name,macros[i].body);
    }
  } else fprintf(stderr,"No macros are defined!\n");
}

int load_macros_from_file(void) {
  FILE* f = fopen(MACROS_PATH, "r");
  if (!f) {
    perror("Could not open file for reading");
    return -1;
  }

  macro_count = 0;
  while (macro_count < MAX_WORDS && !feof(f)) {
    char name[MAX_WORD_NAME];
    char body[MAX_WORD_BODY];

    if (fscanf(f, "%15s %[^\n]", name, body) == 2) {
      strncpy(macros[macro_count].name, name, MAX_WORD_NAME);
      strncpy(macros[macro_count].body, body, MAX_WORD_BODY);
      macro_count++;
    }
  }

  fclose(f);
  return 0;
}

user_word* find_macro(char* name) {
  for (int i = 0; i < macro_count; i++) {
    if (strcmp(macros[i].name, name) == 0) return &macros[i];
  }
  return NULL;
}

// Words functions
void list_words(void) {
  if (word_count > 0) {
    for(int i=0; i< word_count; i++) {
      printf("%2d. %16s: %s\n",i,words[i].name,words[i].body);
    }
  } else fprintf(stderr,"No words are defined!\n");
}

void delete_word(Stack *stack) {
  stack_element a = pop(stack);
  if (a.type != TYPE_REAL) {
    fprintf(stderr,"Invalid index type\n");
    return;
  } else {
    int index = (int)a.real;
    delete_word_by_index(index);
  }
}

int delete_word_by_index(int index) {
  if (index < 0 || index >= word_count) {
    fprintf(stderr,"Invalid word number\n");
    return -1; // Invalid index
  }

  // Shift elements left from index+1 onward
  for (int i = index; i < word_count - 1; i++) {
    words[i] = words[i + 1];
  }
  word_count--;
  return 0;
}

void word_select(Stack * stack) {
  stack_element a = pop(stack);
  if (a.type != TYPE_REAL) {
    fprintf(stderr,"Invalid index type\n");
    return;
  } else {
    int index = (int)a.real;
    if (index < 0 || index >= word_count) {
      fprintf(stderr,"Invalid index\n");
      return;
    }
    selected_function = index;
  }
}

void clear_words(void) {
  word_count=0;
}

int save_words_to_file(void) {
  FILE* f = fopen(WORDS_FILE , "w");
  if (!f) {
    perror("Could not open file for writing");
    return -1;
  }

  for (int i = 0; i < word_count; i++) {
    fprintf(f, "%s %s\n", words[i].name, words[i].body);
  }

  fclose(f);
  return 0;
}

int load_words_from_file(void) {
  FILE* f = fopen("user_words.txt", "r");
  if (!f) {
    perror("Could not open file for reading");
    return -1;
  }

  word_count = 0;
  while (word_count < MAX_WORDS && !feof(f)) {
    char name[MAX_WORD_NAME];
    char body[MAX_WORD_BODY];

    if (fscanf(f, "%15s %[^\n]", name, body) == 2) {
      strncpy(words[word_count].name, name, MAX_WORD_NAME);
      strncpy(words[word_count].body, body, MAX_WORD_BODY);
      word_count++;
    }
  }

  fclose(f);
  return 0;
}

user_word* find_word(char* name) {
  for (int i = 0; i < word_count; i++) {
    if (strcmp(words[i].name, name) == 0) return &words[i];
  }
  return NULL;
}

bool is_word_definition(const char *s) {
    // Skip leading whitespace
    while (isspace((unsigned char)*s)) s++;

    if (*s != ':') return false;
    s++; // move past ':'

    // Skip whitespace after ':'
    while (isspace((unsigned char)*s)) s++;

    // Parse the name
    const char *name_start = s;
    while (isalnum((unsigned char)*s) || *s == '_') s++;
    const char *name_end = s;

    if (name_start == name_end) return false;  // No valid name

    // Copy name
    size_t name_len = name_end - name_start;
    if (name_len >= MAX_WORD_NAME) name_len = MAX_WORD_NAME - 1;
    char name[MAX_WORD_NAME];
    strncpy(name, name_start, name_len);
    name[name_len] = '\0';

    // Must be at least one space after name
    if (!isspace((unsigned char)*s)) return false;
    while (isspace((unsigned char)*s)) s++;

    // Now s points to the body start
    const char *body_start = s;

    // Find the last non-whitespace character
    const char *end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end)) end--;

    if (*end != ';') return false;

    // Point just before the final semicolon
    const char *body_end = end;
    while (body_end > body_start && isspace((unsigned char)*(body_end - 1))) body_end--;

    // Copy body text
    size_t body_len = body_end - body_start;
    if (body_len >= MAX_WORD_BODY) body_len = MAX_WORD_BODY - 1;

    if (word_count >= MAX_WORDS) {
      fprintf(stderr, "Too many word definitions.\n");
      return false;
    }

    user_word *w = &words[word_count++];
    strncpy(w->name, name, MAX_WORD_NAME);
    w->name[MAX_WORD_NAME - 1] = '\0';

    strncpy(w->body, body_start, body_len);
    w->body[body_len] = '\0';
    printf("New word %s <- %s\n",w->name,w->body);
    return true;
}
