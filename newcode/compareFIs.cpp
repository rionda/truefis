/**
 * Compare a collection of TrueFIs from a sample with the 'true' collection of
 * FIs.
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

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

#include <unistd.h> // for getopt
extern char *optarg;
extern int optind;

#include "config.h"
#include "itemsets.h"

/**
 * Print usage on stderr.
 */
void usage(const char *binary_name) {
	std::cerr << binary_name << ": compare two collections of itemsets" << std::endl;
	std::cerr << "USAGE: " << binary_name << "[-h] [-v] theta sample_collection original_collection" << std::endl;
	std::cerr << "\t-h: print this help message and exit" << std::endl;
	std::cerr << "\t-v: be verbose" << std::endl;
}

/**
 * Populate configurations by parsing command line options and arguments
 */
int get_configs(int argc, char **argv, ds_config &sample_conf, ds_config &orig_conf, mine_config &mine_conf) {
	int opt;
	while ((opt = getopt(argc, argv, "hv")) != -1) {
		switch (opt) {
		case 'h':
			usage(argv[0]);
			return EXIT_SUCCESS;
			break;
		case 'v':
			mine_conf.verbose = true;
			break;
		}
	}
	if (optind != argc - 3) {
		std::cerr << "ERROR: wrong number of arguments" << std::endl;
		return EXIT_FAILURE;
	}
	mine_conf.theta = std::stod(argv[argc - 3]);
	if (mine_conf.theta <= 0.0 || mine_conf.theta >= 1.0) {
		std::cerr << "ERROR: theta must be a number greater than 0 and smaller than 1" << std::endl;
		return EXIT_FAILURE;
	}
	sample_conf.path.assign(argv[argc - 2]);
	sample_conf.fi_path = sample_conf.path;
	orig_conf.path.assign(argv[argc - 1]);
	orig_conf.fi_path = orig_conf.path;
	return -1;
}

int main(int argc, char **argv) {
	// Get configurations
	ds_config sample_conf;
	sample_conf.items = -1;
	sample_conf.max_supp = -1;
	sample_conf.size = -1;
	ds_config orig_conf;
	orig_conf.items = -1;
	orig_conf.max_supp = -1;
	orig_conf.size = -1;
	mine_config mine_conf;
	int opt_ret = get_configs(argc, argv, sample_conf, orig_conf, mine_conf);
	if (opt_ret == EXIT_FAILURE || opt_ret == EXIT_SUCCESS) {
		return opt_ret;
	}
	if (mine_conf.verbose) {
		std::cerr << "INFO: creating dataset objects...";
	}
	Dataset orig_dataset(orig_conf, false);
	Dataset sample_dataset(sample_conf, false);
	if (mine_conf.verbose) {
		std::cerr << "done" << std::endl;
		std::cerr << "INFO: computing frequent itemsets...";
	}
	std::map<std::set<int>, const double> orig_fis;
	orig_dataset.get_frequent_itemsets(mine_conf.theta, orig_fis);
	if (mine_conf.verbose) {
		std::cerr << "done (" << orig_fis.size() << " FIs in the original collection)" << std::endl;
		std::cerr << "INFO: computing frequent itemsets...";
	}
	std::map<std::set<int>, const double> sample_fis;
	sample_dataset.get_frequent_itemsets(mine_conf.theta, sample_fis);
	if (mine_conf.verbose) {
		std::cerr << "done (" << sample_fis.size() << " FIs in the sample collection)" << std::endl;
	}
	int intersection = 0;
	for (std::map<std::set<int>, const double>::iterator orig_it = orig_fis.begin(); orig_it != orig_fis.end(); ++orig_it) {
		if (sample_fis.find(orig_it->first) != sample_fis.end()) {
			++intersection;
		}
	}
	int false_positives = sample_fis.size() - intersection;
	int false_negatives = orig_fis.size() - intersection;
	double jaccard = ((double) intersection) / (orig_fis.size() + sample_fis.size() - intersection);
	std::cerr << "sample_file=" << sample_conf.fi_path << ",orig_file=" <<
		orig_conf.fi_path << ",origFIs=" << orig_fis.size() << ",sampleFIs=" <<
		sample_fis.size() << std::endl;
	std::cerr << "intersection=" << intersection << ",fp=" << false_positives << ",fn=" << false_negatives << std::endl;
	std::cerr << "jaccard=" << jaccard << std::endl;
	std::cerr << "sample_file,orig_file,origFIs,sampleFIs,intersection,fp,fn,jaccard" << std::endl;
	std::cerr << sample_conf.fi_path << "," << orig_conf.fi_path << "," <<
		orig_fis.size() << "," << sample_fis.size() << "," << intersection <<
		"," << false_positives << "," << false_negatives << "," << jaccard <<
		std::endl;
}
