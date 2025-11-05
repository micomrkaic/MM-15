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

#ifndef INTEGRATION_AND_ZEROS_H
#define INTEGRATION_AND_ZEROS_H

#include <math.h>
#include <stdio.h>
#include "words.h"
#include "globals.h"

double f(double x);
double romberg(double (*f)(double), double a, double b, double tol, int max_iter);
void set_integration_precision(Stack *stack);
void set_f0_precision(Stack *stack);
double stack_helper(double x);
void integrate(Stack *stack);
void find_zero(Stack *stack);
bool bisection(double (*f)(double), double a, double b, double tol, double* root);

#endif // INTEGRATION_AND_ZEROS_H
