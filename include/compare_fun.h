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

#ifndef COMPARE_FUN_H
#define COMPARE_FUN_H

typedef enum {
  CMP_EQ,
  CMP_NE,
  CMP_LT,
  CMP_LE,
  CMP_GT,
  CMP_GE,
  CMP_AND,
  CMP_OR
} comparison_op;

void dot_cmp_top_two(Stack* stack, comparison_op op);

#endif // COMPARE_FUN_H
