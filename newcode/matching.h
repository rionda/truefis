#ifndef _GRAPH_H
#define _GRAPH_H

#include <list>
#include <set>

#include <igraph.h>

int solve_max_bipartite_matching(const std::list<std::set<int > > sets) {
	igraph_t graph;
	igraph_vector_bool_t types;
	igraph_vector_bool_init(types, sets.size() * 2);
	for (int i = 0; i < igraph_vector_bool_size(&types) / 2; ++i) {
		VECTOR(types)[i] = 0;
		VECTOR(types)[sets.size() + i] = 1;
	}

	igraph_create_bipartite(&graph, &types, &edges, false);

	igraph_integer_t matching_size;
	igraph_vector_long_t matching;
	igraph_vector_long_init(&matching, 0);
	igraph_maximum_bipartite_matching(&graph, &types, &matching_size, NULL,
			&matching, NULL, 0);
	igraph_vector_long_destroy(&matching);
	igraph_vector_bool_destroy(&types);
	igraph_destroy(&graph);
	return (int) matching_size;
}

#endif
