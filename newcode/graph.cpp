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
#include <forward_list>
#include <set>
#include <unordered_map>
#include <vector>

#include <igraph.h>

#include "graph.h"

/**
 * Return the size of the maximum matching in the bipartite graph with edges in
 * edges_vector.
 *
 * The size is the number of matched pairs of nodes.
 *
 * edges_vector has 2*no.-of-edges elements. The first edge is between nodes in
 * edges_vector[0] and edges_vector[1], the second between edges_vector[2] and
 * edges_vector[3] and so on.
 */
int get_max_bipartite_matching_size(const int num_nodes, const std::vector<int> &edges_vector) {
	igraph_vector_t edges;
	igraph_vector_init(&edges, edges_vector.size());
	for (int i = 0; i < edges_vector.size(); ++i) {
		VECTOR(edges)[i] = edges_vector[i];
	}
	igraph_vector_bool_t types;
	igraph_vector_bool_init(&types, num_nodes);
	for (int i = 0; i < igraph_vector_bool_size(&types) / 2; ++i) {
		VECTOR(types)[i] = 0;
		VECTOR(types)[(num_nodes / 2) + i] = 1;
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
	return (int) matching_size;
}

/**
 * Return the size of the largest antichain in sets
 *
 * The size is computed by solving an appropriate maximum bipartite matching
 * problem.
 *
 */
int get_largest_antichain_size(const std::forward_list<const std::set<int> *> &sets) {
	int sets_size = 0;
	std::unordered_map<const std::set<int> *, int> sets_to_ids;
	int max_id = 0;
	for (const std::set<int> *s : sets) {
		sets_to_ids[s] = max_id++;
		++sets_size;
	}
	std::vector<int> edges;
	for (auto first_it = sets.begin(); first_it != sets.end(); ++first_it) {
		auto second_it = first_it;
		++second_it;
		for (; second_it != sets.end(); ++second_it) {
			const std::set<int> *first = *first_it;
			const std::set<int> *second = *second_it;
			if ((*first).size() > (*second).size()) {
				first = *second_it;
				second = *first_it;
			}
			std::vector<int>::iterator it;
			std::vector<int> difference((*first).size());
			it = std::set_difference(
					(*first).begin(), (*first).end(), (*second).begin(), (*second).end(),
					difference.begin());
			// Add an edge if first is a subset of second
			if (it == difference.begin()) {
				edges.push_back(sets_to_ids[first]);
				edges.push_back(sets.size() + sets_to_ids[second]);
			}
		}
	}
	return sets_size - get_max_bipartite_matching_size(sets_size * 2, edges);
}
