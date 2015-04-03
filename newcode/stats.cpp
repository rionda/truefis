/**
 * A class to represent the statistics of a collection of itemsets on a dataset,
 * mainly the bound to the empirical VC-dimension and the maximum frequency (in
 * the dataset) of an itemset in the collection. Definition in stats.h .
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
#include <cmath>
#include <forward_list>
#include <fstream>
#include <list>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include <ilcplex/ilocplex.h>

#include "dataset.h"
#include "graph.h"
#include "itemsets.h"
#include "stats.h"
#include "sukp.h"

/**
 * Utility function to sort int in decreasing order
 */
bool reverse_int_comp(const int lhs, const int rhs) {
	return lhs > rhs;
}

/**
 * Return the bound to the empirical VC-dimension, computed using method, and
 * using intersections_by_size as what in the pseudocode is known as T.
 *
 * This function implements the second loop in the pseudocode.
 *
 * This function is used by the constructors of stats, and is not exposed
 * outside.
 */
int compute_evc_bound(
		const std::map<int, std::forward_list<const std::set<int> *>, bool (*)(int,int)>
		&intersections_by_size, const Stats_method_bound &method) {
	// The following check is true if all transactions contains all items from U
	// or no item from U.
	if (intersections_by_size.empty()) {
		return 0;
	}
	// The following plays the role both of d_i and d_{i-1}.
	int evc_bound = intersections_by_size.begin()->first + 1;
	// The following plays the role of both b_i and b_{i-1} in the pseudocode.
	int max_antichain_size = 0;
	std::map<int, std::forward_list<const std::set<int> *>, bool (*)(int,int)>::const_iterator trans_it = intersections_by_size.begin();
	// The following is \mathsf{T}_{d_i}}. There is no need for \mathsf{T}_{d_{i-1}}
	std::forward_list<const std::set<int> *> curr_T;
	int prev_T_size = 0;
	while (true) { // We have theoretical guarantees that this will terminate
		int curr_T_size = prev_T_size;
		while (curr_T_size - prev_T_size < evc_bound - max_antichain_size) {
			evc_bound = trans_it->first;
			for (std::forward_list<const std::set<int> *>::const_iterator it = (trans_it->second).begin(); it != (trans_it->second).end(); ++it) {
				curr_T.push_front(*it);
				++curr_T_size;
			}
			++trans_it;
		}
		if (method == BOUND_SCAN) {
			// Return d_1
			break;
		} else if (method == BOUND_EXACT) {
			// Run the exact method, computing antichains
			max_antichain_size = get_largest_antichain_size(curr_T);
			if (max_antichain_size >= evc_bound) {
				break;
			}
		} else { // NOT REACHED
			assert(false);
		}
		prev_T_size = curr_T_size;
	}
	return evc_bound;
}

int compute_evc_bound_using_sukp(Dataset &dataset,
		const std::forward_list<std::set<int> > &collection, const bool use_antichain,
		std::pair<int,int> &result) {
	std::ifstream dataset_s(dataset.get_path());
	if(! dataset_s.good()) {
		// XXX TODO Handle failure if problem in opening file;
		dataset_s.close();
	}
	if (collection.empty()) {
		result.first = 0;
		result.second = 0;
		return 1;
	}
	// The following is called U in the pseudocode
	std::unordered_set<int> items;
	int collection_size = 0;
	for (std::set<int> itemset : collection) {
		items.insert(itemset.begin(), itemset.end());
		++collection_size;
	}
	// The following is called L in the pseudocode
	std::map<int, int, bool (*)(int,int)> intersection_sizes_counts(reverse_int_comp);
	// The following is called D_C in the pseudocode
	std::set<std::set<int> > intersections;
	std::string line;
	int size = 0;
	while (std::getline(dataset_s, line)) {
		++size;
		std::set<int> tau = line2itemset(line);
		std::vector<int> intersection_v(tau.size());
		std::vector<int>::iterator it;
		it = std::set_intersection(
				tau.begin(), tau.end(), items.begin(), items.end(),
				intersection_v.begin());
		std::set<int> intersection(intersection_v.begin(), it);
		std::pair<std::set<std::set<int> >::iterator, bool> insertion_pair;
		if (! intersection.empty() && ! intersection.size() == items.size()) {
			insertion_pair = intersections.insert(intersection);
		}
		if (insertion_pair.second) { // intersection was not already in intersections
			std::map<int, int, bool (*)(int,int)>::iterator intersection_it = intersection_sizes_counts.find(intersection.size());
			if (intersection_it == intersection_sizes_counts.end()) {
				intersection_it = (intersection_sizes_counts.emplace(intersection.size(), 1)).first;
			}
			// Exploit the sorted nature (in decreasing order) of the map
			for (++intersection_it; intersection_it != intersection_sizes_counts.end(); ++intersection_it) {
				intersection_sizes_counts[intersection_it->first]++;
			}
		}
	}
	dataset_s.close();
	dataset.set_size(size); // Set size in the database object

	// We do not need a counter 'i' like in the pseudocode, we can use an
	// iterator that exploits the sorted nature of the map
	std::map<int, int, bool (*)(int,int)>::iterator it = intersection_sizes_counts.begin();
	//ILOSTLBEGIN
	IloEnv env;
	IloModel model(env);
	IloCplex *cplex = NULL;
	if (get_CPLEX(cplex, model, env, items, collection, it->second, use_antichain) == -1) {
		// XXX TODO something went wrong
		env.end();
	};
	while (true) {
		// The following is q in the pseudocode
		double profit = get_SUKP_profit(*cplex);
		if (profit == -1.0) {
			// XXX TODO something went wrong
			env.end();
		}
		// This is b in the pseudocode
		int bound =  ((int) floor(log2(profit))) + 1;
		if (bound <= it->second) {
			return bound;
		} else {
			++it;
			if (it != intersection_sizes_counts.end()) {
				set_capacity(model, it->second);
			} else {
				env.end();
				break;
			}
		}
	}
	--it;
	return (int) floor(fmin(it->second, log2(collection_size)));
}

/**
 * Constructor for the case when the collection of itemsets is the set of all
 * itemsets.
 *
 * A side effect of this constructor is to set the size and the maximum support
 * of an item in the dataset object.
 *
 * Parameters:
 * dataset: the dataset object.
 * method: a Stats_method_bound specifying which strategy to use to compute the
 * bound to the empirical VC-dimension. See stats.h for documentation on
 * Stats_method_bound.
 */
Stats::Stats(Dataset& dataset, const Stats_method_bound &method) {
	std::ifstream dataset_s(dataset.get_path());
	if(! dataset_s.good()) {
		// XXX TODO Handle failure if problem in opening file;
		dataset_s.close();
	}
	// The following is called T in the pseudocode
	std::map<int, std::forward_list<const std::set<int> *>, bool (*)(int,int)>
		transactions_by_size(reverse_int_comp);
	// The following is called L in the pseudocode
	std::set<std::set<int> > transactions;
	// The following is called U in the pseudocode
	std::set<int> items;
	std::string line;
	int size = 0;
	std::unordered_map<int, int> item_supps;
	max_supp = 1;
	// This is the first loop in the pseudocode, to populate T and L. In this
	// special case, we also use it to populate U.
	while (std::getline(dataset_s, line)) {
		++size;
		std::set<int> tau = line2itemset(line);
		for (int item : tau) {
			items.insert(item);
			if (item_supps.count(item) == 0) {
				item_supps[item] = 1;
			} else {
				item_supps[item]++;
				if (item_supps[item] > max_supp) {
					++max_supp;
				}
			}
		}
		std::pair<std::set<std::set<int> >::iterator, bool> insertion_pair = transactions.insert(tau);
		if (insertion_pair.second) { // tau was not already in transactions
			if (transactions_by_size.count(tau.size()) == 0) {
				std::forward_list<const std::set<int> *> v(1, &(*(insertion_pair.first)));
				transactions_by_size[tau.size()] = v;
			} else {
				transactions_by_size[tau.size()].push_front(&(*insertion_pair.first));
			}
		}
	}
	dataset_s.close();
	// Setting values on the dataset object.
	dataset.set_size(size);
	dataset.set_max_supp(max_supp);
	//item_supps.clear(); XXX I never know if this is a good idea
	// Remove the element that contains all items, if there is one.
	// This is done here because we do not have the whole set of items while we
	// build T in the previous loop, so there we cannot check whether a
	// transaction contains all items.
	int key = transactions_by_size.begin()->first;
	if (key == items.size()) {
		transactions_by_size[key].clear();
		transactions_by_size.erase(key);
	}
	evc_bound = compute_evc_bound(transactions_by_size, method);
}

Stats::Stats(
		Dataset &dataset,
		const std::set<std::set<int> > &collection,
		const Stats_method_count &count_method,
		const Stats_method_bound &bound_method,
		const bool use_antichain) {
	std::ifstream dataset_s(dataset.get_path());
	if (count_method == COUNT_SUKP) {
		// XXX TODO Implement
		evc_bound = 0;
		max_supp = 0;
		return;
	}
	if(! dataset_s.good()) {
		// XXX TODO Handle failure if problem in opening file;
		dataset_s.close();
	}
	if (collection.empty()) {
		evc_bound = 0;
		max_supp = 0;
		return;
	}
	// The following is called U in the pseudocode
	std::set<int> items;
	for (std::set<int> itemset : collection) {
		items.insert(itemset.begin(), itemset.end());
	}
	// The following is called T in the pseudocode. The name is a little
	// confusing, because the keys are not really the sizes of the
	// intersections, but rather are the values
	// \lfloor\log_2 \ell_\tau\rfloor + 1,
	// where \ell_tau is the number of itemsets in C that appear in \tau.
	std::map<int, std::forward_list<const std::set<int> *>, bool (*)(int,int)>
		intersections_by_size(reverse_int_comp);
	// The following is called L in the pseudocode
	std::set<std::set<int> > intersections;
	std::string line;
	int size = 0;
	// The following will contain the supports of all itemsets in the
	// collection.
	std::unordered_map<const std::set<int> *, int> itemsets_supps;
	max_supp = 1;
	// This is the first loop in the pseudocode, to populate T and L
	while (std::getline(dataset_s, line)) {
		++size;
		std::set<int> tau = line2itemset(line);
		std::vector<int> intersection_v(tau.size());
		std::vector<int>::iterator it;
		it = std::set_intersection(
				tau.begin(), tau.end(), items.begin(), items.end(),
				intersection_v.begin());
		std::set<int> intersection(intersection_v.begin(), it);
		std::pair<std::set<std::set<int> >::iterator, bool> insertion_pair;
		if (! intersection.empty() && ! intersection.size() == items.size()) {
			insertion_pair = intersections.insert(intersection);
		}
		if (insertion_pair.second) { // intersection was not already in intersections
			int itemsets_in_tau_log = tau.size();
			std::forward_list<const std::set<int> *> itemsets_in_tau_list;
			if (count_method == COUNT_EXACT) {
				int itemsets_in_tau = 0;
				for (std::set<std::set<int> >::const_iterator itemset = collection.begin(); itemset != collection.end(); ++itemset) {
					if ((*itemset).size() <= intersection.size() &&
							std::includes(intersection.begin(),
								intersection.end(), (*itemset).begin(),
								(*itemset).end())) {
						++itemsets_in_tau;
						// If we are interested in antichains, we actually need
						// to store (iterators to) the itemsets.
						if (use_antichain) {
							itemsets_in_tau_list.push_front(&(*itemset));
						}
						if (itemsets_supps.count(&(*itemset)) == 0) {
							itemsets_supps[&(*itemset)] = 1;
						} else {
							itemsets_supps[&(*itemset)]++;
							if (itemsets_supps[&(*itemset)] > max_supp) {
								++max_supp;
							}
						}
					}
				}
				if (use_antichain) {
					itemsets_in_tau =
						get_largest_antichain_size(itemsets_in_tau_list);
				}
				itemsets_in_tau_log = ((int) floor(log2(itemsets_in_tau))) + 1;
			}
			if (intersections_by_size.count(itemsets_in_tau_log) == 0) {
				std::forward_list<const std::set<int> *> v(1, &(*insertion_pair.first));
				intersections_by_size[itemsets_in_tau_log] = v;
			} else {
				intersections_by_size[itemsets_in_tau_log].push_front(&(*insertion_pair.first));
			}
		}
	}
	dataset_s.close();
	dataset.set_size(size); // Set size in the database object
	//itemsets_supps.clear(); XXX I never know if this is a good idea
	evc_bound = compute_evc_bound(intersections_by_size, bound_method);
}


int Stats::get_evc_bound() {
	return evc_bound;
}

int Stats::get_max_supp() {
	return max_supp;
}
