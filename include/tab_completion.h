#ifndef TAB_COMPLETION_H
#define TAB_COMPLETION_H

char* function_name_generator(const char* text, int state);
char** function_name_completion(const char* text, int start, int end);

#endif // TAB_COMPLETION_H
