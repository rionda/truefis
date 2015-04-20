/**
 * Compute a collection of itemsets that is, with probability at least 1-\delta,
 * a subset of TFI(\pi,\Itm,\theta), using the 'Holdout' algorithm.
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
#include "pvalue.h"
#include "stats.h"

/**
 * Print usage on stderr.
 */
void usage(const char *binary_name) {
	std::cerr << binary_name << ": compute, with probability at least 1-delta, a subset of the TrueFIs w.r.t. theta" << std::endl;
	std::cerr << "USAGE: " << binary_name << "[-h] [-s size] [-v] delta theta exp_frequent_itemsets_path exp_dataset_path eval_frequent_itemsets_path eval_dataset_path" << std::endl;
	std::cerr << "\t-h: print this help message and exit" << std::endl;
	std::cerr << "\t-s size: specify the size of BOTH datasets" << std::endl;
	std::cerr << "\t-v: be verbose" << std::endl;
}

/**
 * Populate configurations by parsing command line options and arguments
 */
int get_configs(int argc, char **argv, ds_config &exp_ds_conf, ds_config &eval_ds_conf, mine_config &mine_conf) {
	int opt;
	while ((opt = getopt(argc, argv, "hs:v")) != -1) {
		switch (opt) {
		case 'h':
			usage(argv[0]);
			return EXIT_SUCCESS;
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
	if (optind != argc - 6) {
		std::cerr << "ERROR: wrong number of arguments" << std::endl;
		return EXIT_FAILURE;
	}
	mine_conf.delta = std::stod(argv[argc - 6]);
	if (mine_conf.delta <= 0.0 || mine_conf.delta >= 1.0) {
		std::cerr <<  "ERROR: delta must be a number greater than 0 and smaller than 1" << std::endl;
		return EXIT_FAILURE;
	}
	mine_conf.theta = std::stod(argv[argc - 5]);
	if (mine_conf.theta <= 0.0 || mine_conf.theta >= 1.0) {
		std::cerr << "ERROR: theta must be a number greater than 0 and smaller than 1" << std::endl;
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
	exp_ds_conf.items = -1;
	exp_ds_conf.max_supp = -1;
	exp_ds_conf.size = -1;
	ds_config eval_ds_conf;
	eval_ds_conf.items = -1;
	eval_ds_conf.max_supp = -1;
	eval_ds_conf.size = -1;
	mine_config mine_conf;
	int opt_ret = get_configs(argc, argv, exp_ds_conf, eval_ds_conf, mine_conf);
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
		std::cerr << "INFO: computing frequent itemsets of the exploratory dataset...";
	}
	std::map<std::set<int>, const double, bool (*)(const std::set<int> &, const std::set<int> &)> exp_frequent_itemsets(size_comp_nopointers);
	exp_dataset.get_frequent_itemsets(mine_conf.theta, exp_frequent_itemsets);
	if (mine_conf.verbose) {
		std::cerr << "done (" << exp_frequent_itemsets.size() << " FIs)" << std::endl;
		std::cerr << "INFO: computing frequent itemsets of the evaluation dataset...";
	}
	std::map<std::set<int>, const double, bool (*)(const std::set<int> &, const std::set<int> &)> eval_frequent_itemsets(size_comp_nopointers);
	eval_dataset.get_frequent_itemsets(mine_conf.theta, eval_frequent_itemsets);
	if (mine_conf.verbose) {
		std::cerr << "done (" << eval_frequent_itemsets.size() << " FIs)" << std::endl;
		std::cerr << "INFO: computing epsilon...";
	}
	double accepted_freq = 1.0;
	double non_accepted_freq = mine_conf.theta - 1.0 / eval_dataset.get_size();
	const double supposed_freq = non_accepted_freq;
	const double min_diff = 1.0 / eval_dataset.get_size();
	const double critical_value_log = log(mine_conf.delta) - log(exp_frequent_itemsets.size());
	while (accepted_freq - non_accepted_freq > min_diff) {
		double mid_point = (accepted_freq + non_accepted_freq) / 2.0;
		double p_value_log = get_pvalue_log_chernoff(mid_point, eval_dataset.get_size(), supposed_freq);
		if (p_value_log < critical_value_log) {
			accepted_freq = mid_point;
		} else {
			non_accepted_freq = mid_point;
		}
	}
	double epsilon = accepted_freq - mine_conf.theta;
	if (mine_conf.verbose) {
		std::cerr << "done, epsilon=" << epsilon << std::endl;
		std::cerr << "INFO: computing trueFIs..." << std::endl;
	}
	std::cout << "(" << eval_dataset.get_size() << ")" << std::endl;
	int output_count = 0;
	for (std::map<std::set<int>, const double>::iterator fis_it = eval_frequent_itemsets.begin(); fis_it != eval_frequent_itemsets.end(); ++fis_it) {
		if (fis_it->second >= accepted_freq && exp_frequent_itemsets.find(fis_it->first) != exp_frequent_itemsets.end()) {
			std::cout << itemset2string(fis_it->first) << " (" << (int)
				round(fis_it->second * eval_dataset.get_size()) << ")"  <<
				std::endl;
			++output_count;
		}
	}
	if (mine_conf.verbose) {
		std::cerr << "done, output size is " << output_count << " itemsets" <<
			std::endl;
	}
	std::cerr << "exp_res_file=" << exp_ds_conf.fi_path << ",eval_res_file=" <<
		eval_ds_conf.fi_path << ",epsilon=" << epsilon << ",d=" <<
		mine_conf.delta << ",min_freq=" << mine_conf.theta << ",trueFIs=" <<
		output_count << std::endl;
	std::cerr << "exp_size=" << exp_dataset.get_size() << ",eval_size=" <<
		eval_dataset.get_size() << std::endl;
	std::cerr << "exp_res=" << exp_frequent_itemsets.size() << ",eval_res=" <<
		eval_frequent_itemsets.size() << std::endl;
	std::cerr <<
		"exp_res_file,eval_res_file,epsilon,d,min_freq,trueFIs,exp_size,eval_size,exp_res,eval_res"
		<< std::endl;
	std::cerr << exp_ds_conf.fi_path << "," << eval_ds_conf.fi_path << "," <<
		epsilon << "," << mine_conf.delta << "," << mine_conf.theta << "," <<
		output_count << "," << exp_dataset.get_size()
		<< "," << eval_dataset.get_size() << "," << exp_frequent_itemsets.size()
		<< "," << eval_frequent_itemsets.size() << std::endl;
}
