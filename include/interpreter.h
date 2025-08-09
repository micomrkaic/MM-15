#ifndef INTERPRETER_H
#define INTERPRETER_H

#include <stdbool.h>
#include "stack.h"

bool is_unary_operator(const char *token);
bool is_binary_operator(const char *token);
double binary_op(double a, double b, const char* op);
double unary_op(double a, const char* op);
void process_token(char* token, Stack* stack, Stack* old_stack, Tokenizer* t);
double evaluate_input(char *input, Stack *stack, Stack *old_stack);

#endif // INTERPRETER_H
