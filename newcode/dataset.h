/**
 * A class to represent a dataset. Code in dataset.cpp .
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
#ifndef _DATASET_H
#define _DATASET_H

#include <string>

class Dataset {
	int max_supp;
	int size;
	const std::string path;
	public:
		Dataset(const std::string, const int = -1 , const bool = false);
		std::string get_path() const { return path; }
		int get_max_supp(const bool = false);
		int get_size(const bool = false);
		int set_max_supp(const int);
		int set_size(const int);
};
#endif
