/**
 * Compute a collection of itemsets that is, with probability at least 1-\delta,
 * a subset of TFI(\pi,\Itm,\theta), using the 'VCholdout' algorithm.
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
#include "epsilon.h"
#include "itemsets.h"
#include "stats.h"

/**
 * Print usage on stderr.
 */
void usage(const char *binary_name) {
	std::cerr << binary_name << ": compute, with probability at least 1-delta, a subset of the TrueFIs w.r.t. theta" << std::endl;
	std::cerr << "USAGE: " << binary_name << " [-e evc_bound] [-h] [-s size] delta theta expl_bound_method eval_count_method eval_bound_method exp_frequent_itemsets_path exp_dataset_path eval_frequent_itemsets_path eval_dataset_path" << std::endl;
	std::cerr << "\t-e evc_bound: use 'evc_bound' as the bound to the empirical VC-dimension for the exploratory dataset" << std::endl;
	std::cerr << "\t-h: print this help message and exit" << std::endl;
	std::cerr << "\t-m max_supp: use 'max_supp' as the maximum support of an item in the exploratory dataset" << std::endl;
	std::cerr << "\t-s size: specify the size of BOTH datasets" << std::endl;
	std::cerr << "\t-v: be verbose" << std::endl;
}

/**
 * Populate configurations by parsing command line options and arguments
 */
int get_configs(int argc, char **argv, ds_config &exp_ds_conf, ds_config &eval_ds_conf, mine_config &mine_conf, stats_config &exp_stats_conf, stats_config &eval_stats_conf) {
	int opt;
	while ((opt = getopt(argc, argv, "e:hm:s:v")) != -1) {
		switch (opt) {
		case 'e':
			exp_stats_conf.evc_bound = std::stoi(optarg);
			break;
		case 'h':
			usage(argv[0]);
			return EXIT_SUCCESS;
			break;
		case 'm':
			exp_ds_conf.max_supp = std::stoi(optarg);
			exp_stats_conf.max_supp = exp_ds_conf.max_supp;
			break;
		case 's':
			exp_ds_conf.size = std::stoi(optarg);
			eval_ds_conf.size = exp_ds_conf.size;
			break;
		case 'v':
			mine_conf.verbose = true;
			break;
		}
	}
	if (optind != argc - 9) {
		std::cerr << "ERROR: wrong number of arguments" << std::endl;
		return EXIT_FAILURE;
	}
	mine_conf.delta = std::stod(argv[argc - 9]);
	if (mine_conf.delta <= 0.0 || mine_conf.delta >= 1.0) {
		std::cerr <<  "ERROR: delta must be a number greater than 0 and smaller than 1" << std::endl;
		return EXIT_FAILURE;
	}
	mine_conf.theta = std::stod(argv[argc - 8]);
	if (mine_conf.theta <= 0.0 || mine_conf.theta >= 1.0) {
		std::cerr << "ERROR: theta must be a number greater than 0 and smaller than 1" << std::endl;
		return EXIT_FAILURE;
	}
	exp_stats_conf.use_antichain = false;
	exp_stats_conf.cnt_method = COUNT_EXACT; // UNUSED
	std::string method(argv[argc -7]);
	if (method == "exact") {
		exp_stats_conf.bnd_method = BOUND_EXACT;
	} else if (method == "scan") {
		exp_stats_conf.bnd_method = BOUND_SCAN;
	} else {
		std::cerr << "ERROR: bound method for exploratory phase must be 'exact' or 'scan'" << std::endl;
		return EXIT_FAILURE;
	}
	eval_stats_conf.use_antichain = false;
	method.assign(argv[argc - 6]);
	if (method == "exact") {
		eval_stats_conf.cnt_method = COUNT_EXACT;
	} else if (method == "fast") {
		eval_stats_conf.cnt_method = COUNT_FAST;
	} else if (method == "sukp") {
		eval_stats_conf.cnt_method = COUNT_SUKP;
	} else {
		std::cerr << "ERROR: count method for eval phase must be 'exact', 'fast', or 'sukp'" << std::endl;
		return EXIT_FAILURE;
	}
	method.assign(argv[argc - 5]);
	if (method == "exact") {
		eval_stats_conf.bnd_method = BOUND_EXACT;
	} else if (method == "scan") {
		eval_stats_conf.bnd_method = BOUND_SCAN;
	} else {
		std::cerr << "ERROR: bound method for eval phase must be 'exact' or 'scan'" << std::endl;
		return EXIT_FAILURE;
	}
	exp_ds_conf.fi_path.assign(argv[argc - 4]);
	exp_ds_conf.path.assign(argv[argc - 3]);
	eval_ds_conf.fi_path.assign(argv[argc - 2]);
	eval_ds_conf.path.assign(argv[argc - 1]);
	return -1;
}

int main(int argc, char **argv) {
	// Get configurations
	ds_config exp_ds_conf;
	exp_ds_conf.size = -1;
	exp_ds_conf.max_supp = -1;
	ds_config eval_ds_conf;
	eval_ds_conf.size = -1;
	eval_ds_conf.max_supp = -1;
	mine_config mine_conf;
	stats_config exp_stats_conf;
	exp_stats_conf.evc_bound = -1;
	exp_stats_conf.max_supp = -1;
	stats_config eval_stats_conf;
	int opt_ret = get_configs(argc, argv, exp_ds_conf, eval_ds_conf, mine_conf, exp_stats_conf, eval_stats_conf);
	if (opt_ret == EXIT_FAILURE || opt_ret == EXIT_SUCCESS) {
		return opt_ret;
	}
	if (mine_conf.verbose) {
		std::cerr << "INFO: creating dataset objects...";
	}
	Dataset exp_dataset(exp_ds_conf, false);
	Dataset eval_dataset(eval_ds_conf, false);
	if (mine_conf.verbose) {
		std::cerr << "done" << std::endl;
		std::cerr << "INFO: creating stats object for the exploratory dataset...";
	}
	Stats exp_stats(exp_dataset, exp_stats_conf);
	if (mine_conf.verbose) {
		std::cerr << "done (evc_bound=" << exp_stats.get_evc_bound()
			<< ", max_supp=" << exp_stats.get_max_supp() << ")" << std::endl;
		std::cerr << "INFO: exploratory dataset size is " << exp_dataset.get_size() << std::endl;
	}
	// The following acts as both \delta_1 and \delta_2 in the pseudocode.
	double lowered_delta = 1.0 - std::sqrt(1 - mine_conf.delta);
	double exp_epsilon = get_epsilon(exp_stats, exp_dataset, lowered_delta);
	if (mine_conf.verbose) {
		std::cerr << "INFO: exp_epsilon=" << exp_epsilon << std::endl;
		std::cerr << "INFO: computing frequent itemsets of the exploratory dataset...";
	}
	std::map<std::set<int>, const double, bool (*)(const std::set<int> &, const std::set<int> &)> exp_frequent_itemsets(size_comp_nopointers);
	exp_dataset.get_frequent_itemsets(mine_conf.theta, exp_frequent_itemsets);
	if (mine_conf.verbose) {
		std::cerr << "done (" << exp_frequent_itemsets.size() << " FIs)" << std::endl;
		std::cerr << "INFO: filtering out very frequent itemsets...";
	}
	// We now scan the frequent_itemsets map, and send to output (and removing
	// from the map) those with frequencies at least theta+epsilon. The leftover
	// is what is called \mathcal{G} in the pseudocode.
	std::cout << "(" << eval_dataset.get_size() << ")" << std::endl;
	int output_count = 0;
	for (std::map<std::set<int>, const double>::iterator fis_it = exp_frequent_itemsets.begin(); fis_it != exp_frequent_itemsets.end();) {
		// Print and remove the itemsets with frequency at least theta+epsilon_1.
		if (fis_it->second >= mine_conf.theta + exp_epsilon) {
			std::cout << itemset2string(fis_it->first) << " (" << (int)
				round(fis_it->second * exp_dataset.get_size()) << ")" <<
				std::endl;
			// Exploit the fact that changes to STL maps do not invalidate iterators
			std::map<std::set<int>, const double>::iterator tmp(fis_it);
			++fis_it;
			exp_frequent_itemsets.erase(tmp->first);
			++output_count;
			continue;
		}
		++fis_it;
	}
	if (mine_conf.verbose) {
		std::cerr << "done (" <<  output_count << " FIs sent to output, " <<
			exp_frequent_itemsets.size() << " survived)" << std::endl;
		std::cerr << "INFO: computing (filtered) frequent itemsets of the evaluation dataset...";
	}
	std::map<std::set<int>, const double, bool (*)(const std::set<int> &, const std::set<int> &)> eval_frequent_itemsets(size_comp_nopointers);
	eval_dataset.get_frequent_itemsets(mine_conf.theta, eval_frequent_itemsets);
	// Remove itemsets not in \mathcal{G} from the evaluation results
	double max_freq_G = 0.0;
	for (std::map<std::set<int>, const double>::iterator fis_it = eval_frequent_itemsets.begin(); fis_it != eval_frequent_itemsets.end();) {
		if (fis_it->second > max_freq_G) {
				max_freq_G = fis_it->second;
		}
		if (exp_frequent_itemsets.find(fis_it->first) == exp_frequent_itemsets.end()) {
			// Exploit the fact that changes to STL maps do not invalidate iterators
			std::map<std::set<int>, const double>::iterator tmp(fis_it);
			++fis_it;
			eval_frequent_itemsets.erase(tmp->first);
			continue;
		}
		++fis_it;
	}
	if (mine_conf.verbose) {
		std::cerr << "done (" <<  eval_frequent_itemsets.size() << " FIs , " <<
			exp_frequent_itemsets.size() << " in G)" << std::endl;
		std::cerr << "INFO: computing closed_itemsets...";
	}
	// The following is called \mathcal{C}_1 in the pseudocode.
	std::unordered_set<const std::set<int>*> closed_itemsets;
	get_closed_itemsets(eval_frequent_itemsets, closed_itemsets);
	if (mine_conf.verbose) {
		std::cerr << "done (" << closed_itemsets.size() << " CIs)" << std::endl;
		std::cerr << "INFO: computing eval_stats..." << std::endl;
	}
	// The following compute the EVC bound called d_2 in the pseudocode, and the
	// maximum frequency of an itemset from F.
	Stats eval_stats(eval_dataset, closed_itemsets, eval_stats_conf);
	eval_stats.set_max_supp((int) round(max_freq_G * eval_dataset.get_size()));
	if (mine_conf.verbose) {
		std::cerr << "done (evc_bound=" << eval_stats.get_evc_bound() <<
			", max_supp=" << eval_stats.get_max_supp() << ")" << std::endl;
	}
	double eval_epsilon = get_epsilon(eval_stats, eval_dataset, lowered_delta);
	if (mine_conf.verbose) {
		std::cerr << "INFO: eval_epsilon=" << eval_epsilon << std::endl;
	}
	// Print the itemsets with frequency at least theta+eval_epsilon
	for (std::map<std::set<int>, const double>::iterator fis_it = eval_frequent_itemsets.begin(); fis_it != eval_frequent_itemsets.end(); ++fis_it) {
		if (fis_it->second >= mine_conf.theta + eval_epsilon) {
			std::cout << itemset2string(fis_it->first) << " (" << (int)
				round(fis_it->second * eval_dataset.get_size()) << ")"  <<
				std::endl;
			++output_count;
		}
	}
	if (mine_conf.verbose) {
		std::cerr << "INFO: output size is " << output_count << " itemsets" <<
			std::endl;
	}
	std::cerr << "exp_res_file=" << exp_ds_conf.fi_path << ",eval_res_file=" <<
		eval_ds_conf.fi_path << ",exp_epsilon=" << exp_epsilon <<
		",eval_epsilon=" << eval_epsilon << ",d=" << mine_conf.delta <<
		",min_freq=" << mine_conf.theta << ",trueFIs=" << output_count <<
		std::endl;
	std::cerr << "exp_size=" << exp_dataset.get_size() << ",eval_size=" <<
		eval_dataset.get_size() << std::endl;
	std::cerr << "exp_res_filtered=" << exp_frequent_itemsets.size() <<
		",holdout_intersect=" << eval_frequent_itemsets.size() << std::endl;
	std::cerr << "exp_evc_bound=" << exp_stats.get_evc_bound() <<
		",eval_evc_bound=" << eval_stats.get_evc_bound() << std::endl;
	std::cerr << "exp_res_file,eval_res_file,exp_epsilon,eval_epsilon,d,min_freq,trueFIs,exp_size,eval_size,exp_res_filtered,holdout_intersect,exp_evc_bound,eval_evc_bound" << std::endl;
	std::cerr << exp_ds_conf.fi_path << "," << eval_ds_conf.fi_path << "," <<
		exp_epsilon << "," << eval_epsilon << "," << mine_conf.delta << "," <<
		mine_conf.theta << "," << output_count << "," << exp_dataset.get_size()
		<< "," << eval_dataset.get_size() << "," << exp_frequent_itemsets.size()
		<< "," << eval_frequent_itemsets.size() << "," <<
		exp_stats.get_evc_bound() << "," << eval_stats.get_evc_bound() <<
		std::endl;
}
