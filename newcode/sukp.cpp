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
#include <iostream>
#include <forward_list>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <string>

#include <ilcplex/ilocplex.h>

#include "sukp.h"

double get_SUKP_profit(const IloCplex &cplex) {
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
		double profit = floor(((double) best_integer_value) / (1.0 - sol_gap));
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
	static const string capacity_constr_name = "capacity";
	try {
		for (IloExtractable extr : IloModel) {
			if (extr.isConstraint() && capacity_constr_name.compare(extr.getName())) {
				IloRange constr = (IloRange) extr.asConstraint();
				IloRange.setUB(capacity);
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

int get_CPLEX(IloModel &model, IloCplex &cplex,  const IloEnv &env, const std::unordered_set<int> &items, const std::forward_list<std::set<int> > &collection, const int capacity, const bool use_antichain, const double gap = 0.1);
	try {
		std::unsorted_map<int, int> items_to_vars;
		int var_ind = 0;
		IloIntVarArray vars(env);
		IloRangeArray constraints(env);
		IloModel model(env);
		// Create variables for the items and build the capacity constraint
		IloExpr capacity_expr(env);
		for (int item : items) {
			vars.add(IloIntVar(env, 0, 1));
			capacity_expr.add(1.0 * vars[var_ind++]);
		}
		// Add capacity constraint
		constraints.add(IloRange(env, 0.0, capacity_expr, capacity, "capacity"));

		// Create variables for the itemsets, add knapsack constraints, and
		// create objective function
		IloExpr obj_expr(env);
		std::unordered_map<std::set<int>*, int> itemsets_to_vars;
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
			// Add the antichain constraints: if itemset 'A' the subset of 'B'
			// or vice versa, then add the constraint 0 \le var_A + var_B \le 1.
			// XXX The following may not be good for memory, as we are adding a
			// lot of constraints.
			for (std::forward_list<std::set<int>*>::const_iterator first_it = collection.begin(); first_it != collection.end(); ++first_it) {
				std::forward_list<std::set<int>*>::const_iterator second_it = first_it;
				for (++second_it; second_it != collection.end(); ++second_it) {
					if (min((*(*first_it)).size(),(*(*second_it)).size()) > 1) {
						std::vector<int> intersection(min((*(*first_it)).size(),(*(*second_it)).size()));
						std::vector<int>::iterator it = std::set_intersection(
								(*(*first_it)).begin(),
							(*(*first_it)).end(), (*(*second_it)).begin(),
							(*(*second_it)).end(),  intersection.begin());
						if (it - intersection.begin() == min((*(*first_it)).size(),(*(*second_it)).size())) {
							constraints.add(IloRange(env, 0.0, vars[ ] + vars[ ], 1.0));
						}
					}
				}
			}
		}
		model.add(constraints);
		IloCplex cplex(model);
		// Set the relative gap below which to stop.
		// The solver stops if it founds a feasible solution whose value is
		// proved to be within a fraction 'gap' of the optimal. Note that being
		// a maximization problem, the feasible solution found will be smaller
		// than the optimal. We can then return the profit of the found feasible
		// solution divided by the gap (queried below) to obtain an upper bound
		// to the optimal solution. As long as the gap is below 0.5, this is a
		// good approximation for our purposes. See also discussion in the paper
		cplex.setParam(IloCplex::EpGap, gap);
		// Set the absolute gap below which to stop.
		// As above, but for an absolute gap, instead of relative. Since we are
		// using the log2 of the solution, being within 2.0 doesn't change much.
		cplex.setParam(IloCplex::EpAGap, 2.0);
		// Set a maximum time limit, in seconds (600=10mins. no clue.)
		cplex.setParam(IloCple::TiLim, 600);
	} catch (IloException& e) {
		std::cerr << "ConcertException: " << e << std::endl;
		return -1;
	} catch (...) {
		std::cerr << "UnknownException" << std::endl;
		return -1;
	}
	return 0;
}
