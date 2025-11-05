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

#include <string.h>
#include <readline/history.h>
#include <readline/readline.h>
#include "words.h"
#include "function_list.h"


char* function_name_generator(const char* text, int state) {
  static int phase;  // 0 = builtins, 1 = user words, 2 = macros
  static int index;
  static int len;

  extern const char* const function_names[];
  extern user_word words[];
  extern int word_count;
  extern user_word macros[];
  extern int macro_count;

  if (!state) {
    phase = 0;
    index = 0;
    len = strlen(text);
  }

  while (phase < 3) {
    const char* candidate = NULL;
    (void) candidate;

    switch (phase) {
    case 0:
      while (function_names[index]) {
	const char* fn = function_names[index++];
	if (strncmp(fn, text, len) == 0) {
	  return strdup(fn);
	}
      }
      break;

    case 1:
      while (index < word_count) {
	const char* name = words[index++].name;
	if (strncmp(name, text, len) == 0) {
	  return strdup(name);
	}
      }
      break;

    case 2:
      while (index < macro_count) {
	const char* name = macros[index++].name;
	if (strncmp(name, text, len) == 0) {
	  return strdup(name);
	}
      }
      break;
    }

    // Advance to next phase
    phase++;
    index = 0;
  }

  return NULL;
}

char** function_name_completion(const char* text, int start, int end) {
  (void)end;
  // Complete only if at the beginning of the line or after space
  if (start == 0 || rl_line_buffer[start - 1] == ' ') {
    return rl_completion_matches(text, function_name_generator);
  }
  return NULL;
}


