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

#ifndef LINEAR_ALGEBRA_H
#define LINEAR_ALGEBRA_H


int matrix_inverse(Stack* stack);
int matrix_determinant(Stack* stack);
int matrix_frobenius_norm(Stack *stack);
int solve_linear_system(Stack* stack);
int matrix_eigen_decompose(Stack* stack);
int matrix_transpose(Stack* stack);
int matrix_cholesky(Stack* stack);
int matrix_svd(Stack* stack);
void gsl_matrix_pseudoinverse(const gsl_matrix* A, gsl_matrix* A_pinv);
int matrix_pseudoinverse(Stack* stack);

#endif // LINEAR_ALGEBRA_H
