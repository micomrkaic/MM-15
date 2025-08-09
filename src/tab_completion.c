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


/* // Called repeatedly by readline to find matches */
/* char* function_name_generator(const char* text, int state) { */
/*   static int list_index; */
/*   static size_t len; */

/*   if (state == 0) { */
/*     list_index = 0; */
/*     len = strlen(text); */
/*   } */

/*   while (function_names[list_index]) { */
/*     const char* name = function_names[list_index++]; */
/*     if (strncmp(name, text, len) == 0) { */
/*       return strdup(name);  // Caller will free it */
/*     } */
/*   } */
/*   return NULL;  // No more matches */
/* } */

char** function_name_completion(const char* text, int start, int end) {
  (void)end;
  // Complete only if at the beginning of the line or after space
  if (start == 0 || rl_line_buffer[start - 1] == ' ') {
    return rl_completion_matches(text, function_name_generator);
  }
  return NULL;
}


