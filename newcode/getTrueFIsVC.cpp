/**
 * Compute a collection of itemsets that is, with probability at least 1-\delta,
 * a subset of TFI(\pi,\Itm,\theta), using the 'non-holdout-VC' algorithm.
 * XXX Update the name of the algorithm if needed.
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
#include <cmath> // for sqrt, round
#include <cstdlib> // for EXIT_FAILURE, SUCCESS
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <unordered_set>

#include <unistd.h> // for getopt
extern char *optarg;
extern int optind;

#include "config.h"
#include "dataset.h"
#include "epsilon.h"
#include "itemsets.h"
#include "stats.h"

/**
 * Print usage on stderr.
 */
void usage(const char *binary_name) {
	std::cerr << binary_name << ": compute, with probability at least 1-delta, a subset of the TrueFIs w.r.t. theta" << std::endl;
	std::cerr << "USAGE: " << binary_name << " [-e evc_bound] [-h] [-s size] delta theta bound_method_1st_phase count_method_2nd_phase bound_method_2nd_phase frequent_itemsets_path dataset_path" << std::endl;
	std::cerr << "\t-e evc_bound: use 'evc_bound' as the first bound to the empirical VC-dimension" << std::endl;
	std::cerr << "\t-h: print this help message and exit" << std::endl;
	std::cerr << "\t-m max_supp: use 'max_supp' as the maximum support of an item in the dataset" << std::endl;
	std::cerr << "\t-s size: specify the size of the dataset" << std::endl;
	std::cerr << "\t-v: be verbose" << std::endl;
}

/**
 * Populate configurations by parsing command line options and arguments
 */
int get_configs(int argc, char **argv, ds_config &ds_conf, mine_config &mine_conf, stats_config &stats_conf_1st, stats_config &stats_conf_2nd) {
	int opt;
	while ((opt = getopt(argc, argv, "e:hm:s:v")) != -1) {
		switch (opt) {
		case 'e':
			stats_conf_1st.evc_bound = std::stoi(optarg);
			break;
		case 'h':
			usage(argv[0]);
			return EXIT_SUCCESS;
			break;
		case 'm':
			ds_conf.max_supp = std::stoi(optarg);
			stats_conf_1st.max_supp = ds_conf.max_supp;
			break;
		case 's':
			ds_conf.size = std::stoi(optarg);
			break;
		case 'v':
			mine_conf.verbose = true;
			break;
		}
	}
	if (optind != argc - 7) {
		std::cerr << "ERROR: wrong number of arguments" << std::endl;
		return EXIT_FAILURE;
	}
	mine_conf.delta = std::stod(argv[argc - 7]);
	if (mine_conf.delta <= 0.0 || mine_conf.delta >= 1.0) {
		std::cerr <<  "ERROR: delta must be a number greater than 0 and smaller than 1" << std::endl;
		return EXIT_FAILURE;
	}
	mine_conf.theta = std::stod(argv[argc - 6]);
	if (mine_conf.theta <= 0.0 || mine_conf.theta >= 1.0) {
		std::cerr << "ERROR: theta must be a number greater than 0 and smaller than 1" << std::endl;
		return EXIT_FAILURE;
	}
	stats_conf_1st.use_antichain = false;
	stats_conf_1st.cnt_method = COUNT_EXACT; // not used anyway
	std::string method(argv[argc - 5]);
	if (method == "exact") {
		stats_conf_1st.bnd_method = BOUND_EXACT;
	} else if (method == "scan") {
		stats_conf_1st.bnd_method = BOUND_SCAN;
	} else {
		std::cerr << "ERROR: bound method for 1st phase must be 'exact' or 'scan'" << std::endl;
		return EXIT_FAILURE;
	}
	stats_conf_2nd.use_antichain = true;
	method.assign(argv[argc - 4]);
	if (method == "exact") {
		stats_conf_2nd.cnt_method = COUNT_EXACT;
	} else if (method == "fast") {
		stats_conf_2nd.cnt_method = COUNT_FAST;
	} else if (method == "sukp") {
		stats_conf_2nd.cnt_method = COUNT_SUKP;
	} else {
		std::cerr << "ERROR: count method for 2nd phase must be 'exact', 'fast', or 'sukp'" << std::endl;
		return EXIT_FAILURE;
	}
	method.assign(argv[argc - 3]);
	if (method == "exact") {
		stats_conf_2nd.bnd_method = BOUND_EXACT;
	} else if (method == "scan") {
		stats_conf_2nd.bnd_method = BOUND_SCAN;
	} else {
		std::cerr << "ERROR: bound method for 2nd phase must be 'exact' or 'scan'" << std::endl;
		return EXIT_FAILURE;
	}
	ds_conf.fi_path.assign(argv[argc - 2]);
	ds_conf.path.assign(argv[argc - 1]);
	return -1;
}

int main(int argc, char **argv) {
	// Get configurations
	ds_config ds_conf;
	ds_conf.items = -1;
	ds_conf.max_supp = -1;
	ds_conf.size = -1;
	mine_config mine_conf;
	stats_config stats_conf1;
	stats_conf1.evc_bound = -1;
	stats_conf1.max_supp = -1;
	stats_config stats_conf2;
	int opt_ret = get_configs(argc, argv, ds_conf, mine_conf, stats_conf1, stats_conf2);
	if (opt_ret == EXIT_FAILURE || opt_ret == EXIT_SUCCESS) {
		return opt_ret;
	}
	if (mine_conf.verbose) {
		std::cerr << "INFO: creating dataset object...";
	}
	Dataset dataset(ds_conf, false);
	if (mine_conf.verbose) {
		std::cerr << "done" << std::endl;
		std::cerr << "INFO: creating stats object...";
	}
	Stats stats1(dataset, stats_conf1);
	if (mine_conf.verbose) {
		std::cerr << "done (evc_bound=" << stats1.get_evc_bound() << ", max_supp=" << stats1.get_max_supp() << ")" << std::endl;
		std::cerr << "INFO: dataset size is " << dataset.get_size() << std::endl;
	}
	// The following acts as both \delta_1 and \delta_2 in the pseudocode.
	double lowered_delta = 1.0 - std::sqrt(1 - mine_conf.delta);
	double epsilon_1 = get_epsilon(stats1, dataset, lowered_delta);
	if (mine_conf.verbose) {
		std::cerr << "INFO: epsilon_1=" << epsilon_1 << std::endl;
		std::cerr << "INFO: computing frequent itemsets...";
	}
	// The following is called \mathcal{C}_1 in the pseudocode.
	std::map<std::set<int>, const double> frequent_itemsets;
	dataset.get_frequent_itemsets(mine_conf.theta - epsilon_1, frequent_itemsets);
	if (mine_conf.verbose) {
		std::cerr << "done (" << frequent_itemsets.size() << " FIs)" << std::endl;
		std::cerr << "INFO: computing closed itemsets...";
	}
	std::unordered_set<const std::set<int>*> closed_itemsets;
	get_closed_itemsets(frequent_itemsets, closed_itemsets);
	if (mine_conf.verbose) {
		std::cerr << "done (" << closed_itemsets.size() << " CIs)" << std::endl;
		std::cerr << "INFO: computing maximal itemsets...";
	}
	std::unordered_set<const std::set<int>*> maximal_itemsets;
	get_maximal_itemsets(closed_itemsets, maximal_itemsets);
	if (mine_conf.verbose) {
		std::cerr << "done (" << maximal_itemsets.size() << " MIs)" << std::endl;
		std::cerr << "INFO: computing negative border...";
	}
	// The following is called \mathcal{W} in the pseudocode
	std::set<std::set<int> > neg_border;
	get_negative_border(frequent_itemsets, maximal_itemsets, neg_border);
	if (mine_conf.verbose) {
		std::cerr << "done (" << neg_border.size() << " itemsets in the neg. border)" << std::endl;
		std::cerr << "INFO: filtering out negative border...";
	}
	// We can actually remove all itemsets in the negative border that never
	// appear in the dataset.
	// The following is called \mathcal{F} in the pseudocode
	std::unordered_set<const std::set<int>*> collection_F;
	filter_negative_border(dataset, neg_border, collection_F);
	if (mine_conf.verbose) {
		std::cerr << "done (" << collection_F.size() << " itemset survived)" << std::endl;
		std::cerr << "INFO: adding relevant CIs to collection_F...";
	}
	// After the loop is completed, the map will only contain the (closed)
	// itemsets in the set \mathcal{G} in the pseudocode.
	// In the loop, we also insert (pointers to) these closed itemsets into
	// collection_F
	std::cout << "(" << dataset.get_size() << ")" << std::endl;
	double max_freq_F = 0;
	int output_count = 0;
	for (std::map<std::set<int>, const double>::iterator fis_it = frequent_itemsets.begin(); fis_it != frequent_itemsets.end();) {
		// Print and remove the itemsets with frequency at least theta+epsilon_1.
		if (fis_it->second >= mine_conf.theta + epsilon_1) {
			std::cout << itemset2string(fis_it->first) << " (" << (int) round(fis_it->second * dataset.get_size()) << ")" << std::endl;
			// Exploit the fact that changes to STL maps do not invalidate iterators
			std::map<std::set<int>, const double>::iterator tmp(fis_it);
			++fis_it;
			frequent_itemsets.erase(tmp->first);
			++output_count;
			continue;
		}
		// Remove frequent itemsets (with frequency lower than \theta+\epsilon_1) that are not closed
		if (closed_itemsets.find(&((*fis_it).first)) == closed_itemsets.end()) {
			assert(maximal_itemsets.find(&((*fis_it).first)) == maximal_itemsets.end());
			// Exploit the fact that changes to STL maps do not invalidate iterators
			std::map<std::set<int>, const double>::iterator tmp(fis_it);
			++fis_it;
			frequent_itemsets.erase(tmp->first);
			continue;
		} else { // Insert the pointer into collection_F
			collection_F.insert(&((*fis_it).first));
			if (fis_it->second > max_freq_F) {
				max_freq_F = fis_it->second;
			}
		}
		++fis_it;
	}

	if (mine_conf.verbose) {
		std::cerr << "done (" << collection_F.size() << " itemsets)" << std::endl;
		std::cerr << "INFO: computing stats2..." << std::endl;
	}
	// The following compute the EVC bound called d_2 in the pseudocode, and the
	// maximum frequency of an itemset from F.
	Stats stats2(dataset, collection_F, stats_conf2);
	stats2.set_max_supp((int) round(max_freq_F * dataset.get_size()));
	if (mine_conf.verbose) {
		std::cerr << "done (evc_bound=" << stats2.get_evc_bound() << ", max_supp=" << stats2.get_max_supp() << ")" << std::endl;
	}
	double epsilon_2 = get_epsilon(stats2, dataset, lowered_delta);
	if (mine_conf.verbose) {
		std::cerr << "INFO: epsilon_2=" << epsilon_2 << std::endl;
	}
	// Print the itemsets with frequency at least theta+epsilon_2
	for (std::map<std::set<int>, const double>::iterator fis_it = frequent_itemsets.begin(); fis_it != frequent_itemsets.end(); ++fis_it) {
		if (fis_it->second >= mine_conf.theta + epsilon_2) {
			std::cout << itemset2string(fis_it->first) << " (" << (int) round(fis_it->second * dataset.get_size()) << ")" << std::endl;
			++output_count;
		}
	}
	if (mine_conf.verbose) {
		std::cerr << "INFO: output size is " << output_count << " itemsets" << std::endl;
	}

	std::cerr << "res_file=" << ds_conf.fi_path << ",e1=" << epsilon_1 <<
		",e2=" << epsilon_2 << ",d=" << mine_conf.delta << ",min_freq=" << mine_conf.theta <<
		",trueFIs=" << output_count << std::endl;
	std::cerr << "base_set=" << frequent_itemsets.size() << ",closed_itemsets=" <<
		closed_itemsets.size() << ",maximal_itemsets=" <<
		maximal_itemsets.size() << ",neg_border=" << neg_border.size() <<
		",collection_F=" << collection_F.size() << ",evc_bound_1=" <<
		stats1.get_evc_bound() << ",evc_bound_2=" << stats2.get_evc_bound() <<
		std::endl;
	std::cerr << "res_file,e1,e2,d,min_freq,trueFIs,base_set,closed_itemsets,maximal_itemsets,neg_border,collection_F,evc_bound_1,evc_bound_2" << std::endl;
	std::cerr << ds_conf.fi_path << "," << epsilon_1 << "," << epsilon_2 << ","
		<< mine_conf.delta << "," << mine_conf.theta << "," << output_count <<
		"," << frequent_itemsets.size() << "," << closed_itemsets.size() << ","
		<< maximal_itemsets.size() << "," << neg_border.size() << "," <<
		collection_F.size() << "," << stats1.get_evc_bound() << "," <<
		stats2.get_evc_bound() << std::endl;
}
