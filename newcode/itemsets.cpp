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
#include <cassert>
#include <fstream>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <queue>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "itemsets.h"

Itemset::Itemset(const std::set<int> *_itemset) : visited(0), itemset(_itemset),  parents(), children() {}

std::list<Itemset *>::iterator Itemset::add_parent(Itemset * const parent) {
	parents.push_back(parent);
	return parents.begin();
}

std::list<Itemset *>::iterator Itemset::add_child(Itemset * const child) {
	children.push_back(child);
	return children.begin();
}

int get_visit_id() {
	static int id = 1;
	return id++;
}

/**
 * Utility function to sort a container of sets by size.
 */
bool size_comp_nopointers(const std::set<int> &first, const std::set<int> &second) {
	if (first.size() != second.size()) {
		return first.size() < second.size();
	} else {
		for (std::set<int>::const_iterator f_it = first.begin(), s_it = second.begin(); f_it != first.end(); ++f_it, ++s_it) {
			if (*f_it != *s_it) {
				return *f_it < *s_it;
			}
		}
		return false;
	}
	return true;
}

bool size_comp(const std::set<int> *first, const std::set<int> *second) {
	return size_comp_nopointers(*first, *second);
}

bool size_comp_Itemset(Itemset *first, Itemset *second) {
	return size_comp(first->itemset, second->itemset);
}

void create_frequent_itemsets_tree(const std::map<std::set<int>, const double, bool (*)(const std::set<int> &, const std::set<int> &)> &collection, Itemset * root) {
	std::set<Itemset *, bool (*)(Itemset *, Itemset *)> prev(size_comp_Itemset);
	prev.insert(root);
	std::map<std::set<int>, const double, bool (*)(std::set<int>, std::set<int>)>::const_iterator it = collection.begin();
	int size = 1;
	while (it != collection.end()) {
		std::set<Itemset *, bool (*)(Itemset *, Itemset *)> curr(size_comp_Itemset);
		while (it != collection.end() && it->first.size() == size) {
			Itemset *itemset = new Itemset(&((*it).first));
			curr.insert(itemset);
			++it;
		}
		for (std::set<Itemset *, bool (*)(Itemset *, Itemset *)>::iterator itemset = curr.begin(); itemset != curr.end(); ++itemset) {
			for (int item : *((*itemset)->itemset)) {
				std::set<int> parent_set(*((*itemset)->itemset));
				parent_set.erase(item);
				Itemset *parent = new Itemset(&parent_set);
				std::set<Itemset *, bool(*)(Itemset *, Itemset *)>::iterator parent_it = prev.lower_bound(parent);
				(**itemset).add_parent(*parent_it);
				(**parent_it).add_child(*itemset);
				delete parent;
			}
		}
		prev = curr;
		++size;
	}
}

int find_itemsets_in_transaction(std::set<int> &intersection, const std::unordered_set<const std::set<int>*> &collection, Itemset *root) {
	int count = 0;
	int visit_id = get_visit_id();
	std::set<Itemset *, bool (*)(Itemset *, Itemset *)> to_visit(size_comp_Itemset);
	to_visit.insert(root);
	while (! to_visit.empty()) {
		Itemset *head = *(to_visit.begin());
		if (head->visited != visit_id) {
			head->visited = visit_id;
			if (includes(intersection.begin(), intersection.end(),
						head->itemset->begin(), head->itemset->end())) {
				if (collection.find(head->itemset) != collection.end()) {
					++count;
				}
				for (Itemset *child : head->children) {
					to_visit.insert(child);
				}
			} else { // Remove from the list of nodes to explore all the children of this node, as they can't be subsets.
				for (Itemset *child : head->children) {
						to_visit.erase(child);
				}
			}
		}
		to_visit.erase(to_visit.begin());
	}
	return count;
}

/**
 * Return true if first is a subset of second, false otherwise
 */
bool is_subset(const std::set<int> &first, const std::set<int> &second) {
	if (first.size() > second.size()) {
		return false;
	} else if (first.size() == second.size()) {
		return first == second;
	} else {
		return includes(second.begin(), second.end(), first.begin(), first.end());
	}
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
	std::set<const std::set<int>*> local_negative_border;
	std::unordered_set<int> items;
	for (std::set<std::set<int>>::const_iterator it = negative_border.begin(); it != negative_border.end(); ++it) {
		local_negative_border.insert(&(*it));
		items.insert(it->begin(), it->end());
	}
	filtered.clear();
	std::string line;
	std::set<std::set<int>> transactions;
	while (filtered.size() < negative_border.size() && getline(dataset_str, line)) {
		std::set<int> trans = string2itemset(line);
		if (transactions.find(trans) == transactions.end()) {
			transactions.insert(trans);
			for (std::set<const std::set<int>*>::iterator it = local_negative_border.begin(); it != local_negative_border.end();) {
				if (is_subset(**it, trans)) {
					filtered.insert(*it);
					std::set<const std::set<int>*>::iterator tmp(it);
					++it;
					local_negative_border.erase(tmp);
					continue;
				}
				++it;
			}
		}
	}
	dataset_str.close();
	return filtered.size();
}

struct SetHash
{
    std::size_t operator () (const std::set<int> &my_set) const
    {
		std::stringstream stream;
		for (const int item : my_set) {
			stream << item << " ";
		}
		return std::hash<std::string>()(stream.str());
    }
};

/**
 * Utility function to sort a container of sets in decreasing order by size.
 */
bool decreasing_size_comp(const std::set<int> *first, const std::set<int> *second) {
	return !size_comp(first, second);
}

bool check_closed_itemsets(const std::map<std::set<int>, const double, bool (*)(const std::set<int> &, const std::set<int> &)> &collection,
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
int get_closed_itemsets(const std::map<std::set<int>, const double, bool (*)(const std::set<int> &, const std::set<int> &)> &collection,
		std::unordered_set<const std::set<int> *> &closed_itemsets) {
	std::unordered_map<double, std::set<const std::set<int> *, bool(*)(const std::set<int>*, const std::set<int>*)>> freq_to_itemsets;
	for (std::map<std::set<int>, const double>::const_iterator it = collection.begin(); it != collection.end(); ++it) {
		if (freq_to_itemsets.find(it->second) == freq_to_itemsets.end()) {
			std::set<const std::set<int>*, bool (*)(const std::set<int>*, const std::set<int>*)> itemsets(size_comp);
			freq_to_itemsets[it->second] = itemsets;
		}
		freq_to_itemsets[it->second].insert(&(*it).first);
	}
	// Return early if we can easily check that all itemsets are closed
	if (freq_to_itemsets.size() == collection.size()) {
		for (std::map<std::set<int>, const double>::const_iterator it = collection.begin(); it != collection.end(); ++it) {
			closed_itemsets.insert(&((*it).first));
		}
		return freq_to_itemsets.size();
	}
	for (auto it = freq_to_itemsets.begin(); it != freq_to_itemsets.end(); ++it) {
		if ((it->second).size() == 1) {
			closed_itemsets.insert(*((it->second).begin()));
		} else {
			std::unordered_set<const std::set<int> *> local_closed_itemsets;
			std::unordered_set<const std::set<int> *> border;
			for (const std::set<int> *itemset : it->second) {
				std::unordered_set<const std::set<int>*> to_remove;
				std::unordered_set<const std::set<int>*> to_remove_border;
				for (const std::set<int> *closed : border) {
					if (is_subset(*closed, *itemset)) {
						to_remove.insert(closed);
						to_remove_border.insert(closed);
					}
				}
				for (const std::set<int> *removing : to_remove_border) {
					border.erase(removing);
				}
				border.insert(itemset);
				for (const std::set<int> *removing : to_remove) {
					local_closed_itemsets.erase(removing);
				}
				local_closed_itemsets.insert(itemset);
			}
			for (const std::set<int> *itemset : local_closed_itemsets) {
				closed_itemsets.insert(itemset);
			}
		}
	}
	return closed_itemsets.size();
}

int old_get_closed_itemsets(const std::map<std::set<int>, const double> &collection,
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
int get_maximal_itemsets(Itemset *root, std::unordered_set<const std::set<int>*> &maximal_itemsets) {
	maximal_itemsets.clear();
	int visit_id = get_visit_id();
	std::set<Itemset *, bool (*)(Itemset *, Itemset*)> to_visit(size_comp_Itemset);
	to_visit.insert(root);
	while (! to_visit.empty()) {
		Itemset *head = *(to_visit.begin());
		if (head->visited != visit_id) {
			head->visited = visit_id;
			if (head->children.empty()) {
				maximal_itemsets.insert(head->itemset);
			} else {
				for (Itemset *child : head->children) {
					to_visit.insert(child);
				}
			}
		}
		to_visit.erase(to_visit.begin());
	}
	return maximal_itemsets.size();
}

/**
 * Compute the Negative Border of the passed collection of maximal itemsets. The
 * NB is stored in the passed set 'negative_border', which is emptied before storing the NB.
 *
 * Returns the size of the NB.
 *
 */
int get_negative_border(const std::map<std::set<int>, const double, bool (*)(const std::set<int> &, const std::set<int> &)> &collection, const std::unordered_set<const std::set<int> *> &maximal_itemsets, std::set<std::set<int> > &negative_border) {
	std::unordered_set<int> items;
	for (const std::set<int> *maximal : maximal_itemsets) {
		items.insert(maximal->begin(), maximal->end());
	}
	std::set<std::set<int>, bool (*)(const std::set<int> &, const std::set<int> &)> local_negative_border(size_comp_nopointers);
	for (std::unordered_set<int>::iterator first = items.begin(); first != items.end(); ++first) {
		std::unordered_set<int>::iterator second = first;
		++second;
		for (; second != items.end(); ++second) {
			std::set<int> candidate;
			candidate.insert(*first);
			candidate.insert(*second);
			if (collection.find(candidate) == collection.end()) {
				local_negative_border.insert(candidate);
			}
		}
	}
	std::unordered_set<std::set<int>, SetHash> rejected;
	for (const std::set<int> *maximal : maximal_itemsets) {
		for (const int item_to_remove_from_maximal : *maximal) {
			std::set<int> reduced_maximal(*maximal);
			reduced_maximal.erase(item_to_remove_from_maximal);
			for (const int item : items) {
				if (item == item_to_remove_from_maximal) {
					continue;
				}
				if (reduced_maximal.find(item) != reduced_maximal.end()) {
					continue;
				}
				// Create sibling of the maximal, in the sense that they have
				// the same size but differ by one item, hence they have at
				// least one common ancestor.
				std::set<int> sibling(reduced_maximal);
				sibling.insert(item);
				if (rejected.find(sibling) != rejected.end()) {
					continue;
				}
				if (local_negative_border.find(sibling) != local_negative_border.end()) {
					continue;
				}
				bool to_add = true;
				for (const std::set<int> neg_border_itemset : local_negative_border) {
					if (is_subset(neg_border_itemset, sibling)) {
						to_add = false;
						break;
					}
				}
				if (! to_add) {
					rejected.insert(sibling);
					continue;
				}
				if (collection.find(sibling) != collection.end()) {
					rejected.insert(sibling);
					continue;
				}
				for (const int item_to_remove : sibling) {
					std::set<int> subset(sibling);
					subset.erase(item_to_remove);
					if (collection.find(subset) == collection.end()) {
						to_add = false;
						rejected.insert(sibling);
						break;
					}
				}
				if (to_add) {
					local_negative_border.insert(sibling);
				} else {
					// if we added the sibling, there's no way we can also add
					// the child (as this child would have the sibling as
					// parent...)
					std::set<int> child(*maximal);
					child.insert(item);
					if (rejected.find(child) != rejected.end()) {
						continue;
					}
					if (local_negative_border.find(child) != local_negative_border.end()) {
						continue;
					}
					to_add = true;
					for (const std::set<int> neg_border_itemset : local_negative_border) {
						if (is_subset(neg_border_itemset, child)) {
							to_add = false;
							break;
						}
					}
					if (! to_add) {
						rejected.insert(child);
						continue;
					}
					for (const int item_to_remove : child) {
						std::set<int> subset(child);
						subset.erase(item_to_remove);
						if (collection.find(subset) == collection.end()) {
							to_add = false;
							break;
						}
					}
					if (to_add) {
						local_negative_border.insert(child);
					} else {
						rejected.insert(child);
					}
				}
			}
		}
	}
	negative_border.clear();
	negative_border.insert(local_negative_border.begin(), local_negative_border.end());
	return negative_border.size();
}

void add_nodes_to_tree(Itemset * const root, const std::unordered_set<const std::set<int>*> &collection) {
	for (const std::set<int>* itemset : collection) {
		Itemset *neg = new Itemset(itemset);
		std::set<Itemset*, bool (*)(Itemset *, Itemset *)> to_visit(size_comp_Itemset);
		to_visit.insert(root);
		int visit_id = get_visit_id();
		while (neg->parents.size() != neg->itemset->size()) {
			Itemset *head = *(to_visit.begin());
			if (head->visited != visit_id) {
				head->visited = visit_id;
				if (includes(itemset->begin(), itemset->end(), head->itemset->begin(), head->itemset->end())) {
					if (head->itemset->size() == itemset->size() - 1) {
						head->add_child(neg);
						neg->add_parent(head);
					}
					for (Itemset * const child : head->children) {
						to_visit.insert(child);
					}
				} else {
					for (Itemset * const child : head->children) {
						to_visit.erase(child);
					}
				}
			}
			to_visit.erase(to_visit.begin());
		}
	}
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

