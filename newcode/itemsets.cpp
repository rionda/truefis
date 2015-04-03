/**
 * Functions to manipulate itemsets and collections of itemsets. Definitions in
 * itemsets.h
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

#include <forward_list>
#include <set>
#include <sstream>
#include <string>

#include "itemsets.h"

/**
 * Convert a string into a set of integers, representing an itemset or a
 * transaction.
 */
std::set<int> line2itemset(const std::string &line) {
	std::stringstream linestream(line);
	std::string item;
	std::set<int> items;
	while(std::getline(linestream, item, ' ')) {
		items.insert(std::stoi(item));
	}
	return items;
}

int get_closed_itemsets(const std::forward_list<std::set<int> > &collection, std::forward_list<std::set<int> > &closed_itemsets) {
	return 0;
}

int get_maximal_itemsets(const std::forward_list<std::set<int> > &collection, std::forward_list<std::set<int> > &maximal_itemsets) {
	return 0;
}
