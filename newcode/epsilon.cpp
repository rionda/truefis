/**
 * Compute the epsilon.
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

#include "dataset.h"
#include "epsilon.h"
#include "stats.h"
/**
 * Compute the bound to the maximum deviation
 */
double get_epsilon(const double evc_bound, const double size, const double max_supp, const double delta, const double c) {
	return 2.0 * c * std::sqrt(2.0 * evc_bound * ( max_supp  / size) / size) +
		std::sqrt(4.0 * M_LN2 - 2.0 * log(delta));
}

/**
 * Compute the bound to the maximum deviation
 */
double get_epsilon(Stats &stats, Dataset &dataset, const double delta, const double c) {
	return 2.0 * c * std::sqrt(2.0 * stats.get_evc_bound() * (
				stats.get_max_supp()  / dataset.get_size()) /
			dataset.get_size()) + std::sqrt(4.0 * M_LN2 - 2.0 * log(delta));
}
