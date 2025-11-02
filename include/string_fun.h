/*
 * This file is part of Mico's toy RPN Calculator
 *
 * Mico's toy RPN Calculator is free software: 
 * you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Mico's toy RPN Calculator is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Mico's toy RPN Calculator. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef STRING_FUN_H
#define STRING_FUN_H

#include "stack.h"

int my_substring(Stack* stack);
void concatenate(Stack* stack);
void to_upper(Stack* stack);
void to_lower(Stack* stack);
void string_length(Stack* stack);
void string_reverse(Stack* stack);
void top_to_string(Stack* stack);
#endif /* STRING_FUN_H */
