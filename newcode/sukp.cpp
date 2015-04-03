/**
 * Various functions that use CPLEX. Declarations in sukp.h .
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
#include <iostream>
#include <forward_list>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <string>
#include <vector>

#include <igraph/igraph.h>
#include <ilcplex/ilocplex.h>

#include "graph.h"
#include "sukp.h"

double get_SUKP_profit(IloCplex &cplex) {
	double profit = 0.0;
	try {
		cplex.solve();
		// If I read the documentation correctly, getBestObjValue() corresponds
		// to the best feasible integer solution found, and is a lower bound to
		// the optimal, while getObjValue() may correspond to a non-integer,
		// non-feasible solution and is an upper bound to the optimal. The two
		// are the same if the problem is solved optimally.
		int best_integer_value = cplex.getBestObjValue();
		int obj_value = cplex.getObjValue();
		// Upper bound to the ratio between the 'best' feasible integer solution
		// value found by the solver and the actual maximum. This is equivalent
		// to cplex.getMIPRelativeGap();
		double sol_gap = ((double) best_integer_value - obj_value) / best_integer_value;
		assert(sol_gap <= cplex.getMIPRelativeGap());
		// By dividing by 1-gap, we get an upper bound to the optimal profit.
		profit = floor(((double) best_integer_value) / (1.0 - sol_gap));
		// Given the above discussion about what 'obj_value' is, doing the
		// following should be valid.
		if (profit > obj_value) {
			profit = obj_value;
		}
	} catch (IloException& e) {
		std::cerr << "ConcertException: " << e << std::endl;
		return -1.0;
	} catch (...) {
		std::cerr << "UnknownException" << std::endl;
		return -1.0;
	}
	return profit;
}

int set_capacity(IloModel &model, const int capacity) {
	static const std::string capacity_constr_name = "capacity";
	try {
		for (IloModel::Iterator it(model); it.ok(); ++it) {
			IloExtractable extr = *it;
			if (extr.isConstraint() && capacity_constr_name.compare(extr.getName())) {
				IloRange capacity_range(dynamic_cast<IloRangeI *>(extr.asConstraint().getImpl()));
				capacity_range.setUB(capacity);
			}
		}
	} catch (IloException& e) {
		std::cerr << "ConcertException: " << e << std::endl;
		return -1.0;
	} catch (...) {
		std::cerr << "UnknownException" << std::endl;
		return -1.0;
	}
	return capacity;
}

int get_CPLEX(IloCplex *cplex, IloModel &model, const IloEnv &env, const std::unordered_set<int> &items, const std::forward_list<std::set<int> > &collection, const int capacity, const bool use_antichain, const double gap) {
	try {
		std::unordered_map<int, int> items_to_vars;
		int var_ind = 0;
		IloIntVarArray vars(env);
		IloRangeArray constraints(env);
		// Create variables for the items and build the capacity constraint
		IloExpr capacity_expr(env);
		for (; var_ind < items.size(); ++var_ind) {
			vars.add(IloIntVar(env, 0, 1));
			capacity_expr += vars[var_ind];
		}
		// Add capacity constraint
		constraints.add(IloRange(env, 0.0, capacity_expr, capacity, "capacity"));

		// Create variables for the itemsets, add knapsack constraints, and
		// create objective function
		IloExpr obj_expr(env);
		std::unordered_map<const std::set<int>*, int> itemsets_to_vars;
		for (std::forward_list<std::set<int> >::const_iterator it =
				collection.begin(); it != collection.end(); ++it) {
			vars.add(IloIntVar(env, 0, 1));
			for (int item : *it) {
				// Knapsack constraint: if the item 'a' is in the itemset 'B',
				// then 0 \le var_a - var_B \le 1.
				constraints.add(IloRange(env, 0.0, vars[items_to_vars[item]] - vars[var_ind], 1.0));
			}
			obj_expr += 1.0 * vars[var_ind];
			itemsets_to_vars[&(*it)] = var_ind++;
		}
		// Add objective function
		model.add(IloMaximize(env, obj_expr));

		if (use_antichain) {
			// Add the antichain constraints.
			// We create a graph whose nodes are the itemsets in the collection,
			// and there is an edge between two nodes if one is a subset of the
			// other. Maximal cliques in this graph are maximal chains and we add
			// one constraint per maximal chain, denoting that we can at most
			// pick one itemset per maximal chain. This is a bit smaller than
			// the straightforward solution (commented out below), but it
			// creates the minimum number of constraints, which is good for
			// memory.
			igraph_t *graph = create_antichain_graph(collection, itemsets_to_vars);
			igraph_vector_ptr res;
			igraph_vector_ptr_init(&res, 0);
			// Get maximal cliques of size at least 2.
			// We can ignore single nodes, as the constraint would be irrelevant
			igraph_maximal_cliques(graph, &res, 2, 0);
			for (int i = 0; i < igraph_vector_ptr_size(&cliques); ++i) {
				igraph_vector_t *clique = VECTOR(cliques)[i];
				IloExpr clique_expr(env);
				for (int j = 0; j < igraph_vector_size(clique); ++j) {
					clique_expr += vars[VECTOR(*clique)[j]];
				}
				constraints.add(IloRange(env, 0.0; clique_expr, 1.0));
				igraph_vector_destroy(clique);
				igraph_free(clique);
			}
			igraph_vector_ptr_destroy(&res);
			igraph_destroy(graph);
			// The following is the straightforward solution, but it is
			// commented out because it creates a huge number of constraints,
			// which is bad for memory.
			// Add the antichain constraints: if itemset 'A' the subset of 'B'
			// or vice versa, then add the constraint 0 \le var_A + var_B \le 1.
			// for (std::forward_list<std::set<int>>::const_iterator first_it = collection.begin(); first_it != collection.end(); ++first_it) {
			// 	std::forward_list<std::set<int>>::const_iterator second_it(first_it);
			// 	for (++second_it; second_it != collection.end(); ++second_it) {
			// 		size_t min_size = std::min(first_it->size(), second_it->size());
			// 		if (min_size > 1) {
			// 			std::vector<int> intersection(min_size);
			// 			std::vector<int>::iterator it = std::set_intersection(
			// 				first_it->begin(),
			// 				first_it->end(), second_it->begin(),
			// 				second_it->end(), intersection.begin());
			// 			if (it - intersection.begin() == min_size) {
			// 				constraints.add(IloRange(env, 0.0, vars[itemsets_to_vars[&(*first_it)]] + vars[itemsets_to_vars[&(*second_it)]], 1.0));
			// 			}
			// 		}
			// 	}
			// }
		}
		model.add(constraints);
		IloCplex my_cplex(model);
		cplex = &my_cplex;
		// Set the relative gap below which to stop.
		// The solver stops if it founds a feasible solution whose value is
		// proved to be within a fraction 'gap' of the optimal. Note that being
		// a maximization problem, the feasible solution found will be smaller
		// than the optimal. We can then return the profit of the found feasible
		// solution divided by the gap (queried below) to obtain an upper bound
		// to the optimal solution. As long as the gap is below 0.5, this is a
		// good approximation for our purposes. See also discussion in the paper
		my_cplex.setParam(IloCplex::EpGap, gap);
		// Set the absolute gap below which to stop.
		// As above, but for an absolute gap, instead of relative. Since we are
		// using the log2 of the solution, being within 2.0 doesn't change much.
		my_cplex.setParam(IloCplex::EpAGap, 2.0);
		// Set a maximum time limit, in seconds (600=10mins. no clue.)
		my_cplex.setParam(IloCplex::TiLim, 600);
	} catch (IloException& e) {
		std::cerr << "ConcertException: " << e << std::endl;
		return -1;
	} catch (...) {
		std::cerr << "UnknownException" << std::endl;
		return -1;
	}
	return 0;
}
