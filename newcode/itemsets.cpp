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

#include <cassert>
#include <forward_list>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "dataset.h"
#include "itemsets.h"

/**
 * Return true if first is a subset of second, false otherwise
 */
bool is_subset(const std::set<int> &first, const std::set<int> &second) {
	if (first.size() > second.size()) {
		return false;
	} else if (first.size() == second.size()) {
		return first == second;
	} else {
		for (const int item : first) {
			if (second.find(item) == second.end()) {
				return false;
			}
		}
	}
	return true;
}

/**
 * Remove itemsets that do not appear in the dataset
 */
int filter_negative_border(const Dataset &dataset, const std::set<std::set<int> > &negative_border, std::unordered_set<const std::set<int>*> &filtered) {
	std::ifstream dataset_str(dataset.get_path());
	if(! dataset_str.good()) {
		dataset_str.close();
		std::cerr << "ERROR: cannot open dataset file" << std::endl;
		std::exit(EXIT_FAILURE);
	}
	filtered.clear();
	std::string line;
	while (filtered.size() < negative_border.size() && getline(dataset_str, line)) {
		std::set<int> trans = string2itemset(line);
		for (std::set<std::set<int> >::const_iterator it = negative_border.begin(); it != negative_border.end(); ++it) {
			if (filtered.find(&(*it)) == filtered.end() && is_subset(*it, trans)) {
				filtered.insert(&(*it));
			}
		}
	}
	dataset_str.close();
	return filtered.size();
}

/**
 * Find a superset of a set in a container of pointers to sets. Time is linear in the size of
 * the container.
 */
bool find_superset(std::unordered_set<const std::set<int>*> &collection, std::set<int> &key) {
	for (const std::set<int> *itemset : collection) {
		if (is_subset(key, *itemset)) {
			return true;
		}
	}
	return false;
}

/**
 * Utility function to sort a container of sets by size.
 */
bool size_comp(const std::set<int> *first, const std::set<int> *second) {
	if (first->size() != second->size()) {
		return first->size() < second->size();
	} else {
		for (std::set<int>::const_iterator f_it = first->begin(), s_it = second->begin(); f_it != first->end(); ++f_it, ++s_it) {
			if (*f_it != *s_it) {
				return *f_it < *s_it;
			}
		}
	}
	return true;
}

/**
 * Utility function to sort a container of sets in decreasing order by size.
 */
bool decreasing_size_comp(const std::set<int> *first, const std::set<int> *second) {
	return !size_comp(first, second);
}

bool check_closed_itemsets(const std::map<std::set<int>, const double> &collection,
		std::unordered_set<const std::set<int> *> &closed_itemsets) {
	for (std::map<std::set<int>, const double>::const_iterator it = collection.begin(); it != collection.end(); ++it) {
		const std::set<int> itemset = it->first;
		bool found_ci = false;
		for (std::unordered_set<const std::set<int>*>::const_iterator c_it = closed_itemsets.begin(); (! found_ci) && c_it != closed_itemsets.end(); ++c_it) {
			if (is_subset(itemset, *(*c_it)) && collection.at(itemset) == collection.at(*(*c_it))) {
				found_ci = true;
			}
		}
		if (! found_ci) {
			return false;
		}
	}

	for (std::unordered_set<const std::set<int>*>::const_iterator first_it = closed_itemsets.begin(); first_it != closed_itemsets.end(); ++first_it) {
		for (std::unordered_set<const std::set<int>*>::const_iterator second_it = closed_itemsets.begin(); second_it != closed_itemsets.end(); ++second_it) {
			if (second_it == first_it) {
				continue;
			}
			if (is_subset(**first_it, **second_it) || is_subset(**second_it, **first_it)) {
				assert(collection.at(**first_it) != collection.at(**second_it));
			}
		}
	}
	return true;
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
		collection_sorted_by_size.insert(&((*it).first));
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
	//assert(check_closed_itemsets(collection, closed_itemsets));
	return closed_itemsets.size();
}

bool check_maximal_itemsets(const std::unordered_set<const std::set<int>*> &collection, std::unordered_set<const std::set<int>*> &maximal_itemsets) {
	for (std::unordered_set<const std::set<int> *>::const_iterator it = collection.begin(); it != collection.end(); ++it) {
		bool found_mi = false;
		for (std::unordered_set<const std::set<int>*>::const_iterator m_it = maximal_itemsets.begin(); (! found_mi) && m_it != maximal_itemsets.end(); ++m_it) {
			if (is_subset(**it, **m_it)) {
				found_mi = true;
			}
		}
		if (! found_mi) {
			return false;
		}
	}
	for (std::unordered_set<const std::set<int>*>::const_iterator first_it = maximal_itemsets.begin(); first_it != maximal_itemsets.end(); ++first_it) {
		for (std::unordered_set<const std::set<int>*>::const_iterator second_it = maximal_itemsets.begin(); second_it != maximal_itemsets.end(); ++second_it) {
			if (second_it == first_it) {
				continue;
			}
			assert(!(is_subset(**first_it, **second_it) || is_subset(**second_it, **first_it)));
		}
	}
	return true;
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
	//assert(check_maximal_itemsets(collection, maximal_itemsets));
	return maximal_itemsets.size();
}

/**
 * Compute the Negative Border of the passed collection of maximal itemsets. The
 * NB is stored in the passed set 'negative_border', which is emptied before storing the NB.
 *
 * Returns the size of the NB.
 *
 */
int get_negative_border(std::unordered_set<const std::set<int> *> &collection, std::set<std::set<int> > &negative_border) {
	std::unordered_set<int> items;
	for (const std::set<int> *maximal : collection) {
		items.insert(maximal->begin(), maximal->end());
	}
	negative_border.clear();
	for (const std::set<int> *maximal : collection) {
		for (const int item_to_remove_from_maximal : *maximal) {
			std::set<int> reduced_maximal(*maximal);
			reduced_maximal.erase(item_to_remove_from_maximal);
			for (const int item : items) {
				if (reduced_maximal.find(item) != reduced_maximal.end()) {
					continue;
				}
				// Create sibling of the maximal, in the sense that they have
				// the same size but differ by one item, hence they have at
				// least one common ancestor.
				std::set<int> sibling(reduced_maximal);
				sibling.insert(item);
				if (negative_border.find(sibling) != negative_border.end() || find_superset(collection, sibling)) {
					continue;
				}
				bool to_add = true;
				for (const int item_to_remove : sibling) {
					std::set<int> subset(sibling);
					subset.erase(item_to_remove);
					if (! find_superset(collection, subset)) {
						to_add = false;
						break;
					}
				}
				if (to_add) {
					negative_border.insert(sibling);
				} else {
					// if we added the sibling, there's no way we can also add
					// the child (as this child would have the sibling as
					// parent....incest going on here...)
					std::set<int> child(*maximal);
					child.insert(item);
					if (negative_border.find(child) != negative_border.end()) {
						continue;
					}
					to_add = true;
					for (const int item_to_remove : child) {
						std::set<int> subset(child);
						subset.erase(item_to_remove);
						if (! find_superset(collection, subset)) {
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
	for (std::unordered_set<int>::iterator first = items.begin(); first != items.end(); ++first) {
		std::unordered_set<int>::iterator second = first;
		++second;
		for (; second != items.end(); ++second) {
			std::set<int> candidate;
			candidate.insert(*first);
			candidate.insert(*second);
			if (! find_superset(collection, candidate)) {
				negative_border.insert(candidate);
			}
		}
	}
	return negative_border.size();
}

/**
 * Convert an itemset (a set of integers) to a string and return the string.
 */
std::string itemset2string(const std::set<int> &itemset, const char sep) {
	std::stringstream out;
	for (const int item : itemset) {
		out << item << sep;
	}
	out.unget(); // remove last sep;
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

