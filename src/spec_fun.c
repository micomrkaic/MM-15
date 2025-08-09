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

#include <gsl/gsl_sf_gamma.h>
#include "spec_fun.h"

// Computes the Gamma function Γ(x)
double gamma_function(double x) {
  return gsl_sf_gamma(x);
}

double ln_gamma_function(double x) {
  return gsl_sf_lngamma(x);
}

double beta_function(double x, double y) {
    return gsl_sf_beta(x, y);
}

double ln_beta_function(double x, double y) {
    return gsl_sf_lnbeta(x, y);
}


