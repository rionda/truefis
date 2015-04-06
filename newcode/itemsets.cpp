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

#include <algorithm>
#include <forward_list>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "itemsets.h"

/**
 * Return true if first is a subset of second, false otherwise
 */
bool is_subset(const std::set<int> &first, const std::set<int> &second) {
	if (first.size() > second.size()) {
		return false;
	}
	std::vector<int>::iterator it;
	std::vector<int> difference(first.size());
	it = std::set_difference(
			first.begin(), first.end(), second.begin(), second.end(),
			difference.begin());
	return it == difference.begin();
}

/**
 * Find a set in a container of pointers to sets. Time is linear in the size of
 * the container.
 */
bool find_set(std::unordered_set<const std::set<int>*> &collection, std::set<int> &key) {
	for (const std::set<int> *itemset : collection ) {
		if (itemset->size() != key.size()) {
			continue;
		}
		bool found = true;
		for (std::set<int>::iterator it = itemset->begin(), key_it = key.begin(); key_it != key.end(); ++it, ++key_it) {
			if (*it != *key_it) {
				found = false;
				break;
			}
		}
		if (found) {
			return true;
		}
	}
	return false;
}

/**
 * Utility function to sort a container of sets in decreasing order by size.
 */
bool decreasing_size_comp(const std::set<int> *first, const std::set<int> *second) {
	return first->size() > second->size();
}

/**
 * Utility function to sort a container of sets by size.
 */
bool size_comp(const std::set<int> *first, const std::set<int> *second) {
	return first->size() < second->size();
}

/**
 * Compute the Closed Itemsets among those in the passed collection, which is a
 * map from itemsets to frequencies. The collection of CIs is stored (as
 * pointers) in the passed set 'closed_itemsets', which is emptied before
 * storing the CIs.
 *
 * Returns the number of CIs.
 */
int get_closed_itemsets(const std::map<std::set<int>, const double> &collection,
		std::unordered_set<const std::set<int> *> &closed_itemsets) {
	// Sorting the collection by size simplifies the computation
	std::set<const std::set<int>*, bool (*)(const std::set<int>*, const std::set<int>*)> collection_sorted_by_size(size_comp);
	for (std::map<std::set<int>, const double>::const_iterator it = collection.begin(); it != collection.end(); ++it) {
		collection_sorted_by_size.insert(&(it->first));
	}
	std::unordered_set<const std::set<int> *> border;
	closed_itemsets.clear();
	for (const std::set<int> *itemset : collection_sorted_by_size) {
		std::unordered_set<const std::set<int>*> to_remove;
		std::unordered_set<const std::set<int>*> to_remove_border;
		for (const std::set<int> *closed : border) {
			if (is_subset(*closed, *itemset)) {
				if (collection.at(*closed) == collection.at(*itemset)) {
					to_remove.insert(closed);
				}
				to_remove_border.insert(closed);
			}
		}
		for (const std::set<int> *removing : to_remove_border) {
			border.erase(removing);
		}
		border.insert(itemset);
		for (const std::set<int> *removing : to_remove) {
			closed_itemsets.erase(removing);
		}
		closed_itemsets.insert(itemset);
	}
	return closed_itemsets.size();
}

/**
 * Compute the Maximal Itemsets among those in the passed collection. The
 * collection of MIs is stored in the passed set 'maximal_itemsets',
 * which is emptied before storing the MIs.
 *
 * Returns the number of MIs.
 */
int get_maximal_itemsets(const std::unordered_set<const std::set<int>*> &collection, std::unordered_set<const std::set<int>*> &maximal_itemsets) {
	std::set<const std::set<int>*, bool (*)(const std::set<int>*, const std::set<int>*)> collection_sorted_by_decreasing_size(decreasing_size_comp);
	collection_sorted_by_decreasing_size.insert(collection.begin(), collection.end());
	maximal_itemsets.clear();
	for (const std::set<int> *itemset : collection_sorted_by_decreasing_size) {
		bool to_add = true;
		for (const std::set<int> *maximal : maximal_itemsets) {
			if (is_subset(*itemset, *maximal)) {
				to_add = false;
				break;
			}
		}
		if (to_add) {
			maximal_itemsets.insert(itemset);
		}
	}
	return maximal_itemsets.size();
}

/**
 * Compute the Negative Border of the passed collection. The NB is stored in the
 * passed forward_list 'negative_border', which is emptied before storing the NB.
 *
 * Returns the size of the NB.
 *
 * The forward_list 'collection' is not 'const' because we need to sort its
 * content in decreasing order by size, to simplify the computation.
 */
int get_negative_border(std::unordered_set<const std::set<int> *> &collection, std::set<std::set<int> > &negative_border) {
	std::unordered_set<const std::set<int>*> maximals;
	get_maximal_itemsets(collection, maximals);
	std::unordered_set<int> items;
	for (const std::set<int> *maximal : maximals) {
		items.insert(maximal->begin(), maximal->end());
	}
	negative_border.clear();
	int count = 0;
	for (const std::set<int> *maximal : collection) {
		for (int item_to_remove_from_maximal : *maximal) {
			std::set<int> reduced_maximal(*maximal);
			reduced_maximal.erase(item_to_remove_from_maximal);
			for (int item : items) {
				if (reduced_maximal.find(item) != reduced_maximal.end()) {
					continue;
				}
				std::set<int> sibling(reduced_maximal);
				sibling.insert(item);
				if (negative_border.find(sibling) != negative_border.end() || find_set(collection, sibling)) {
					continue;
				}
				bool to_add = true;
				for (int item_to_remove : sibling) {
					std::set<int> subset(sibling);
					subset.erase(item_to_remove);
					if (! find_set(collection, subset)) {
						to_add = false;
						break;
					}
				}
				if (to_add) {
					negative_border.insert(sibling);
				} else {
					// if we added the sibling, there's no way we can also add
					// the child
					std::set<int> child(*maximal);
					child.insert(item);
					if (negative_border.find(child) != negative_border.end()) {
						continue;
					}
					to_add = true;
					for (int item_to_remove : child) {
						std::set<int> subset(child);
						subset.erase(item_to_remove);
						if (! find_set(collection, subset)) {
							to_add = false;
							break;
						}
					}
					if (to_add) {
						negative_border.insert(child);
					}
				}
			}
		}
	}
	return count;
}

/**
 * Convert an itemset (a set of integers) to a string and return the string.
 */
std::string itemset2string(const std::set<int> &itemset) {
	std::stringstream out;
	for (const int item : itemset) {
		out << item << " ";
	}
	out.unget(); // remove last space;
	return out.str();
}

/**
 * Convert a string into a set of integers, representing an itemset or a
 * transaction.
 */
std::set<int> string2itemset(const std::string &line) {
	std::stringstream linestream(line);
	std::string item;
	std::set<int> items;
	while(std::getline(linestream, item, ' ')) {
		items.insert(std::stoi(item));
	}
	return items;
}

