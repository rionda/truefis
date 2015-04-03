/**
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
#include <cstdlib>
#include <iostream>
#include <string>

#include "dataset.h"
#include "stats.h"

int main (int argc, char **argv) {
	if (argc != 3) {
		std::cerr << "Error: Wrong number of arguments (" << argc <<
			" instead of 2)" << std::endl;
		std::cerr << "Usage: " << argv[0] << " DATASET METHOD" << std::endl;
		return 1;
	}
	std::string dataset_path(argv[1]);
	Dataset dataset(dataset_path);
	int method = strtol(argv[2], NULL, 10);
	Stats_method_bound method_bound;
	if (method == 1)
		method_bound = BOUND_SCAN;
	else
		method_bound = BOUND_EXACT;
	Stats stats(dataset, method_bound);
	std::cout << "size: " << dataset.get_size() << " max_supp: " <<
		dataset.get_max_supp() << " evc_bound: " << stats.get_evc_bound() <<
		" max_supp: " <<  stats.get_max_supp() << std::endl;
	return 0;
}
