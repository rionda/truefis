/**
 * A class to represent the statistics of a collection of itemsets on a dataset,
 * mainly the bound to the empirical VC-dimension and the maximum frequency (in
 * the dataset) of an itemset in the collection. Code in stats.cpp .
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
#ifndef _STATS_H
#define _STATS_H

#include <set>
#include <string>

enum Stats_method_count {
	COUNT_EXACT = 1,
	COUNT_FAST = 2,
	COUNT_SUKP = 3
};

enum Stats_method_bound {
	BOUND_EXACT = 1,
	BOUND_SCAN = 2
};


class Stats {
	int evc_bound;
	double max_supp;
	public:
		Stats(): evc_bound(0), max_supp(0.0) {};
		Stats(int _evc_bound, double _max_supp): evc_bound(_evc_bound), max_supp(_max_supp) {};
		Stats(Dataset &, const Stats_method_bound &);
		Stats(Dataset &, const std::set<std::set<int> > &, const Stats_method_count &, const Stats_method_bound &, const bool = false);
		int get_evc_bound();
		int get_max_supp();
};

#endif
