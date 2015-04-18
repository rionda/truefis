/**
 * Various functions that use igraph. Declarations in graph.h .
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
#include <algorithm>
#include <cassert>
#include <forward_list>
#include <iostream>
#include <set>
#include <unordered_map>
#include <vector>

#include <igraph/igraph.h>

#include "graph.h"
#include "itemsets.h"

/**
 * Return the size of the largest antichain in sets
 *
 * The size is computed by solving an appropriate maximum bipartite matching
 * problem.
 *
 */
int get_largest_antichain_size(std::set<int> &intersection, const std::unordered_set<const std::set<int>*> &collection, Itemset *root) {
	int max_id = 0;
	std::unordered_map<Itemset *, int> sets_to_ids;
	std::unordered_map<Itemset *, const std::set<int>*> ancestors;
	std::set<std::set<int>> ancestors_sets;
	igraph_vector_t edges;
	igraph_vector_init(&edges, 0);
	int visit_id = get_visit_id();
	std::set<Itemset *, bool (*)(Itemset *, Itemset *)> to_visit(size_comp_Itemset);
	to_visit.insert(root);
	for (std::set<Itemset *, bool(*)(Itemset *, Itemset *)>::iterator it = to_visit.begin(); it != to_visit.end();) {
		Itemset *head = *it;
		if (head->visited != visit_id) {
			head->visited = visit_id;
			if (includes(intersection.begin(), intersection.end(),
						head->itemset->begin(), head->itemset->end())) {
				std::set<int> local_ancestors;
				for (Itemset *parent : head->parents) {
					local_ancestors.insert(ancestors[parent]->begin(), ancestors[parent]->end());
					std::unordered_map<Itemset *, int>::iterator index = sets_to_ids.find(parent);
					if (index != sets_to_ids.end()) {
						local_ancestors.insert(index->second);
					}
				}
				std::set<std::set<int>>::iterator pointer = ancestors_sets.find(local_ancestors);
				if (pointer == ancestors_sets.end()) {
					std::pair<std::set<std::set<int>>::iterator, bool> my_pair = ancestors_sets.insert(local_ancestors);
					ancestors[head] = &(*(my_pair.first));
				} else {
					ancestors[head] = &(*pointer);
				}
				if (collection.find(head->itemset) != collection.end()) {
					sets_to_ids[head] = max_id++;
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
		std::set<Itemset *, bool(*)(Itemset *, Itemset *)>::iterator tmp(it);
		++it;
		to_visit.erase(tmp);
	}
	for (std::pair<Itemset *, int> p : sets_to_ids) {
		for (int ancestor_id : *(ancestors[p.first])) {
			igraph_vector_push_back(&edges, ancestor_id);
			igraph_vector_push_back(&edges, max_id + p.second);
		}
	}
	igraph_vector_bool_t types;
	igraph_vector_bool_init(&types, 2 * max_id);
	for (int i = 0; i < max_id; ++i) {
		VECTOR(types)[i] = 0;
		VECTOR(types)[max_id + i] = 1;
	}

	igraph_t graph;
	igraph_create_bipartite(&graph, &types, &edges, 0);

	igraph_integer_t matching_size;
	igraph_vector_long_t matching;
	igraph_vector_long_init(&matching, 0);
	igraph_maximum_bipartite_matching(&graph, &types, &matching_size, NULL,
			&matching, NULL, 0);
	igraph_vector_long_destroy(&matching);
	igraph_vector_bool_destroy(&types);
	igraph_vector_destroy(&edges);
	igraph_destroy(&graph);
	return max_id - matching_size;
}

/**
 * return the size of the largest antichain in sets
 *
 * the size is computed by solving an appropriate maximum bipartite matching
 * problem.
 *
 */
int get_largest_antichain_size_list(const std::forward_list<const std::set<int> *> &sets) {
	int max_id = 0;
	int sets_size = 0;
	std::unordered_map<const std::set<int> *, int> sets_to_ids;
	for (const std::set<int> *s : sets) {
		sets_to_ids[s] = max_id++;
		++sets_size;
	}
	igraph_vector_t edges;
	igraph_vector_init(&edges, 0);
	for (auto first_it = sets.begin(); first_it != sets.end(); ++first_it) {
		auto second_it = first_it;
		++second_it;
		for (; second_it != sets.end(); ++second_it) {
			const std::set<int> *smaller = *first_it;
			const std::set<int> *larger= *second_it;
			if ((*smaller).size() > (*larger).size()) {
				smaller = *second_it;
				larger = *first_it;
			}
			// Add an edge if smaller is a subset of larger
			// edges has 2*no.-of-edges elements. The first edge is between
			// nodes in edges[0] and edges[1], the second between edges[2] and
			// edges[3] and so on.
			if (is_subset(*smaller,*larger)) {
				igraph_vector_push_back(&edges, sets_to_ids[smaller]);
				igraph_vector_push_back(&edges, sets_size + sets_to_ids[larger]);
			}
		}
	}
	igraph_vector_bool_t types;
	igraph_vector_bool_init(&types, 2 * sets_size);
	for (int i = 0; i < sets_size; ++i) {
		VECTOR(types)[i] = 0;
		VECTOR(types)[sets_size + i] = 1;
	}

	igraph_t graph;
	igraph_create_bipartite(&graph, &types, &edges, 0);

	igraph_integer_t matching_size;
	igraph_vector_long_t matching;
	igraph_vector_long_init(&matching, 0);
	igraph_maximum_bipartite_matching(&graph, &types, &matching_size, NULL,
			&matching, NULL, 0);
	igraph_vector_long_destroy(&matching);
	igraph_vector_bool_destroy(&types);
	igraph_vector_destroy(&edges);
	igraph_destroy(&graph);
	return sets_size - matching_size;
}

void create_antichain_graph(igraph_t *graph, const std::unordered_set<const std::set<int>*> &collection, const std::unordered_map<const std::set<int>*, int> &sets_to_ids) {
	igraph_vector_t edges;
	igraph_vector_init(&edges, 0);
	for (std::unordered_set<const std::set<int>*>::const_iterator first_it = collection.begin(); first_it != collection.end(); ++first_it) {
		std::unordered_set<const std::set<int>*>::const_iterator second_it = first_it;
		++second_it;
		for (; second_it != collection.end(); ++second_it) {
			const std::set<int> *smaller = *first_it;
			const std::set<int> *larger = *second_it;
			if (smaller->size() > larger->size()) {
				smaller = *second_it;
				larger = *first_it;
			}
			// Add an edge if smaller is a subset of larger
			// edges has 2*no.-of-edges elements. The first edge is between
			// nodes in edges[0] and edges[1], the second between edges[2] and
			// edges[3] and so on.
			if (is_subset(*smaller, *larger)) {
				igraph_vector_push_back(&edges, sets_to_ids.at(smaller));
				igraph_vector_push_back(&edges, sets_to_ids.at(larger));
			}
		}
	}
	igraph_create(graph, &edges, 0, false);
	igraph_vector_destroy(&edges);
}
