/**
 * A class to represent a dataset. Definition in dataset.h .
 *
 *  Copyright 2015 Matteo Riondato <matteo@cs.brown.edu>
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 */
#include <fstream>
#include <string>

#include "dataset.h"

/**
 * Constructor.
 *
 * Parameters:
 * _path: the path to the file containing the transactions. Must be readable.
 * _size (optional, default = -1): if not less than -1, set the size of the
 * dataset to this value. A value of -1 is like not setting any size.
 * compute_size (optional, default = false): if true, force the computation of
 * the size of the dataset. If true, the value of _size is ignored.
 */
Dataset::Dataset(
		const std::string _path, const int _size, const bool compute_size) :
	path(_path), size(_size) {
		std::ifstream dataset(path);
		if(! dataset.good()) {
			// TODO Handle failure if problem in opening file;
			dataset.close();
		}
		dataset.close();
		if (compute_size) {
			get_size(true);
		} else if (size < -1) {
			size = -1;
		}
}

/**
 * Return the size of the dataset.
 *
 * Recompute it if we never computed before or 'recompute' is true
 */
int Dataset::get_size(const bool recompute) {
	if (size == -1 || recompute) {
		std::ifstream dataset(path);
		if(! dataset.good()) {
			// TODO Handle failure if problem in opening file;
			dataset.close();
			return -1;
		}
		size = 0;
		std::string line;
		while (getline(dataset, line)) {
			++size;
		}
		dataset.close();
	}
	return size;
}

/**
 * Return the maximum support of an item in the dataset.
 *
 * Recompute it if we never computed before or 'recompute' is true
 */
int Dataset::get_max_supp(const bool recompute) {
	if (max_supp == -1 || recompute) {
		// TODO Recompute;
	}
	return max_supp;
}

/**
 * Set the maximum support of an item(set) in the dataset.
 *
 * If the passed support is negative, return -1, otherwise return the passed
 * support.
 */
int Dataset::set_max_supp(const int new_max_supp) {
	if (new_max_supp >= 0) {
		max_supp = new_max_supp;
		return max_supp;
	}
	return -1;
}

/**
 * Set the size of the dataset.
 *
 * If the passed size is negative, return -1, otherwise return the passed size.
 */
int Dataset::set_size(const int new_size) {
	if (new_size >= 0) {
		size = new_size;
		return size;
	}
	return -1;
}
