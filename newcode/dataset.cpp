/**
 * A class to represent a dataset. Definition in itemsets.h .
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
#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <map>
#include <set>
#include <unordered_map>

#include "config.h"
#include "itemsets.h"

/**
 * Constructor using configuration.
 *
 */
Dataset::Dataset(const ds_config &conf, const bool compute) : items(conf.items), max_supp(conf.max_supp), size(conf.size), fi_path(conf.fi_path), path(conf.path) {
	assert(! path.empty());
	assert(! fi_path.empty());
	std::ifstream dataset(path);
	if(! dataset.good()) {
		dataset.close();
		std::cerr << "ERROR: cannot open dataset file" << std::endl;
		std::exit(EXIT_FAILURE);
	}
	dataset.close();
	std::ifstream fi(fi_path);
	if(! fi.good()) {
		fi.close();
		std::cerr << "ERROR: cannot open frequent itemsets file" << std::endl;
		std::exit(EXIT_FAILURE);
	}
	fi.close();
	if (compute) {
		get_size(true);
	}
}

/**
 * Constructor using argument. Only the first one is mandatory.
 */
Dataset::Dataset(const std::string &_path, const int _items, const int _max_supp, const int _size, const std::string &_fi_path) : items(_items), max_supp(_max_supp), size(_size), fi_path(_fi_path), path(_path) {
	assert(! path.empty());
	std::ifstream dataset(path);
	if(! dataset.good()) {
		dataset.close();
		std::cerr << "ERROR: cannot open dataset file" << std::endl;
		std::exit(EXIT_FAILURE);
	}
}

int Dataset::get_frequent_itemsets(const double theta, std::map<std::set<int>, const double> &frequent_itemsets, Itemset *root) {
	std::fstream fi_stream(fi_path);
	if (! fi_stream.good()) {
		fi_stream.close();
		std::cerr << "ERROR: cannot open frequent itemsets file" << std::endl;
		std::exit(EXIT_FAILURE);
	}
	std::string line;
	std::getline(fi_stream, line);
	if (size > -1) {
		assert(std::stoi(line.substr(1)) == size);
	} else {
		size = std::stoi(line.substr(1));
	}
	double prev_freq = 2.0;
	frequent_itemsets.clear();
	while (getline(fi_stream, line)) {
		const size_t parenthesis_index = line.find_first_of("(");
		const std::string itemset_str = line.substr(0, parenthesis_index - 1);
		const std::set<int> itemset = string2itemset(itemset_str);
		const double support = std::stoi(line.substr(parenthesis_index + 1));
		const double freq = support / size;
		if (freq > prev_freq) {
			std::cerr << "ERROR: results must be sorted" << std::endl;
			std::exit(EXIT_FAILURE);
		}
		if (freq >= theta) {
			std::map<std::set<int>, const double>::iterator it = (frequent_itemsets.emplace(itemset, freq)).first;
			if (root != NULL) {
				Itemset *node = new Itemset(&(it->first));
				set_parents(node, root);
			}
			prev_freq = freq;
		} else {
			break;
		}
	}
	fi_stream.close();
	return frequent_itemsets.size();
}

/**
 * Return the size of the dataset.
 *
 * Recompute it if we never computed before or 'recompute' is true
 *
 * As a desired side effect, max_supp and items are also computed.
 */
int Dataset::get_size(const bool recompute) {
	if (size == -1 || recompute) {
		std::ifstream dataset(path);
		if(! dataset.good()) {
			dataset.close();
			std::cerr << "ERROR: cannot open dataset file" << std::endl;
			std::exit(EXIT_FAILURE);
		}
		max_supp = 0;
		size = 0;
		std::string line;
		std::unordered_map<int, int> item_freqs;
		while (getline(dataset, line)) {
			++size;
			const std::set<int> transaction = string2itemset(line);
			for (const int item : transaction) {
				if (item_freqs.find(item) == item_freqs.end()) {
					item_freqs[item] = 1;
				} else {
					item_freqs[item]++;
					if (max_supp < item_freqs[item]) {
						max_supp = item_freqs[item];
					}
				}
			}
		}
		items = item_freqs.size();
		dataset.close();
	}
	return size;
}

/**
 * Return the maximum support of an item in the dataset.
 *
 * Recompute it if we never computed before or 'recompute' is true.
 * In this case, size is also computed, as a desired side effect.
 */
int Dataset::get_max_supp(const bool recompute) {
	if (max_supp == -1 || recompute) {
		get_size(true);
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

int Dataset::get_items_num(const bool recompute) {
	if (items == -1 || recompute) {
		get_size(true);
	}
	return items;
}
