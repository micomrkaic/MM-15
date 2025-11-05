#ifndef MATRIX_FUN_H
#define MATRIX_FUN_H

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

#include <gsl/gsl_rng.h>

int split_matrix(Stack *s);
int select_matrix_element(Stack *s);
int set_matrix_element(Stack *s);
int matrix_extract_diagonal(Stack* stack);
int make_unit_matrix(Stack* stack);
int make_row_range(Stack* stack);
int make_matrix_of_ones(Stack* stack);
int make_matrix_of_zeroes(Stack* stack);
extern gsl_rng* global_rng;  // Assume you initialize this elsewhere
int make_random_matrix(Stack* stack);
int make_gaussian_random_matrix(Stack* stack);
int matrix_dimensions(Stack* stack);
int reshape_matrix(Stack* stack);
int make_diag_matrix(Stack *stack);
int stack_join_matrix_vertical(Stack* stack);
int stack_join_matrix_horizontal(Stack* stack);
int matrix_cumsum_rows(Stack* stack);
int matrix_cumsum_cols(Stack* stack);

#endif // MATRIX_FUN_H
