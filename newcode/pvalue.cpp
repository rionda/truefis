/**
 * Declarations of functions to compute the p-value. Definitions in pvalue.h
 *
 * Copyright 2015 Matteo Riondato <matteo@cs.brown.edu>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#include <cmath>

/**
 * Compute the logarithm of the p-value using the strong Chernoff bound.
 *
 * We use Equation 4.1 from Thm. 4.4 in Mitzenmacher and Upfal, 'Probability and
 * Computing", Cambridge University Press, 2005.
 */
double get_pvalue_log_chernoff(const double test_freq, const double size, const double supposed_freq) {
	return size * (test_freq - supposed_freq - (test_freq * log(test_freq / supposed_freq)));
}

/**
 * Compute the logarithm of the p-value using the weak Chernoff bound.
 *
 * We use Equation 4.2 from Thm. 4.4 in Mitzenmacher and Upfal, 'Probability and
 * Computing", Cambridge University Press, 2005.
 */
double get_pvalue_log_weak(const double test_freq, const double size, const double supposed_freq) {
    return -size * pow(test_freq - supposed_freq, 2.0) / (supposed_freq * 3);
}
