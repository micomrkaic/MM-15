#ifndef HELP_H
#define HELP_H

typedef struct {
    const char *name;         // word, e.g. "sin"
    const char *stack_effect; // RPN stack effect, e.g. "x -- sin(x)"
    const char *description;  // short description
    const char *example;      // short usage example
} HelpEntry;

extern const HelpEntry help_table[];

void help_menu(void);
void list_all_functions(void);
void list_all_functions_sorted(void);
void usage(const char *name);
int op_usage(Stack *stack);
void whose_place(void);

#endif //HELPER_FUNCTIONS_H
