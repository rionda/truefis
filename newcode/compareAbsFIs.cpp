/**
* Compare a collection of itemsets with a "ground-truth" collection of itemsets
*
* Copyright 2015 Matteo Riondato <matteo@cs.brown.edu>
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <sstream>
#include <vector>

#include <libgen.h>
#include <unistd.h>
extern char *optarg;
extern int optind;
extern int optopt;
extern int opterr;
extern int optreset;

using namespace std;

// Allowing for very long transactions, just in case
static const short TRANSACTION_LINE_MAXLEN = 10000;
static const string COMMA = ",";
static const string SPACE = " ";
static const string VERBOSE_HEADER = "INFO: ";
static const string LOG_HEADER = "LOG: ";
static const string ERROR_HEADER = "ERROR: ";


/**
 * Call and check malloc. Print message to stderr and exit if fails.
 *
 */
inline void *malloc_check(size_t size, const char *err_msg) {
	void *ptr = malloc(size);
	if (ptr == NULL) {
		cerr << "CRITICAL: malloc() failed in " << err_msg << endl;
		exit(EXIT_FAILURE);
	}
	return ptr;
}

/**
 * Convert an itemset to a string of items separated by whitespaces.
 *
 */
string __attribute__((pure)) itemset_to_str(const set<unsigned int>& itemset)  {
	stringstream stream;
	for (unsigned int item : itemset) {
		stream << item << " ";
	}
	stream.unget(); // remove last space
	return stream.str();
}

/**
 * Convert a transaction to a set of items.
 *
 */
inline set<unsigned int> __attribute__((pure)) transaction_to_set(const char *line, bool is_utility_line=false) {
	// We copy the line because we modify it.
	char *copy = (char *) malloc_check(strlen(line) + 1, "transaction_to_set");
	char *orig_copy = copy;
	strncpy(copy, line, strlen(line) + 1);
	set<unsigned int> items;
	if (__builtin_expect(!!(is_utility_line), 0)) {
		// If this is an utility dataset, consider only the first part of the transaction
		char *colons_location = strchr(copy, ':');
		if (colons_location != NULL) {
			*colons_location = '\0';
		}
	}
	char *token;
	while ((token = strsep(&copy, " \t")) != NULL) {
		if (*token != '\0') {
			items.insert(strtol(token, NULL, 10));
		}
	}
	free(orig_copy);
	return items;
}

/**
 * Sort the items in a transaction.
 *
 */
inline __attribute__((pure)) string sort_transaction(const char *line) {
	return itemset_to_str(transaction_to_set(line));
}

/**
 * Get next line from in_file.
 *
 * The line is copied in line. Only at most size-1 characters are copied.
 *
 * Metadata / comments (lines that starts with '#') and empty lines are skipped.
 *
 */
inline bool get_next_line(char *line, int size, FILE __restrict__ *in_file) {
	char c;
	// Skip metadata and empty lines
	while (true) {
		c = fgetc(in_file);
		if (c == '#') {
			fgets(line, size, in_file);
		} else if (c == '\n') {
			// do nothing
		} else {
			break;
		}
	}
	// Check for end of file
	if (feof(in_file)) {
		return false;
	}

	// Read line
	ungetc(c, in_file);
	fgets(line, size, in_file);

	return true;
}
map<string, double> get_results(
		FILE *results_file, const double min_freq) {
	char line[TRANSACTION_LINE_MAXLEN];
	get_next_line(line, TRANSACTION_LINE_MAXLEN, results_file);
	unsigned int size = (unsigned int) atoi(line + 1);
	double previous_freq = 2.0;
	map<string, double> results;
	while (get_next_line(line, TRANSACTION_LINE_MAXLEN, results_file)) {
		char *parenthesis = strchr(line, '(');
		*(parenthesis - 1) = '\0';
		string itemset = sort_transaction(line);
		double support = strtod(parenthesis + 1, NULL);
		double freq = support / size;
		if (freq > previous_freq) {
			cerr << ERROR_HEADER << "results must be sorted" << endl;
			exit(EXIT_FAILURE);
		}
		if (freq >= min_freq) {
			results[itemset] = freq;
			previous_freq = freq;
		} else {
			break;
		}
	}
    return results;
}

void usage(char *binary_name) {
	cerr << binary_name
		<< ": verify if the set of itemsets in sample_results_file is an "
		<< "(eps,delta)-approximation of the Frequent Itemsets w.r.t. "
		<< "theta in exact_results_file" << endl;
	cerr << "USAGE: "
		<< binary_name << "[-hv] epsilon delta theta exact_results_file "
		<< "sample_results_file" << endl;
	cerr << "\t-h: print this help message and exit" << endl;
	cerr << "\t-v: verbose output" << endl;
}

int main(int argc, char **argv) {
	bool verbose = false;
	int opt;
	while ((opt = getopt(argc, argv, "hv")) != -1) {
		switch (opt) {
		case 'h':
			usage(argv[0]);
			return EXIT_SUCCESS;
			break;
		case 'v':
			verbose = true;
			break;
		}
	}
	if (optind != argc - 5) {
		cerr << ERROR_HEADER << "wrong number of arguments" << endl;
		return EXIT_FAILURE;
	}
	double epsilon = strtod(argv[argc - 5], NULL);
	if (errno == ERANGE || epsilon <= 0.0 || epsilon >= 1.0) {
		cerr << ERROR_HEADER
			<< "epsilon must be a number greater than 0.0 and smaller than 1"
			<< endl;
		return EXIT_FAILURE;
	}
	double delta = strtod(argv[argc - 4], NULL);
	if (errno == ERANGE || delta <= 0.0 || delta >= 1.0) {
		cerr << ERROR_HEADER
			<< "delta must be a number greater than 0.0 and smaller than 1"
			<< endl;
		return EXIT_FAILURE;
	}
	double theta = strtod(argv[argc - 3], NULL);
	if (errno == ERANGE || theta <= 0.0 || theta >= 1.0) {
		cerr << ERROR_HEADER
			<< "theta must be a number greater than 0.0 and smaller than 1"
			<< endl;
		return EXIT_FAILURE;
	}
	FILE *exact_FILE = fopen(argv[argc - 2], "r");
	if (exact_FILE == NULL) {
		perror("Error opening exact_results_file");
		return errno;
	}
	FILE *sample_FILE = fopen(argv[argc - 1], "r");
	if (sample_FILE == NULL) {
		perror("Error opening sample_results_file");
		return errno;
	}
	if (verbose) {
		cout << VERBOSE_HEADER << "reading extended exact results..." << flush;
	}
	map<string, double> extended_exact_results =
		get_results(exact_FILE, theta - epsilon);
	if (verbose) {
		cout << "done" << endl;
		cout << VERBOSE_HEADER << "creating exact results..." << flush;
	}
	fclose(exact_FILE);
	map<string, double> exact_results;
	set<string> exact_results_keys;
	set<string> acceptable_false_positives_candidates;
	for (auto const &it : extended_exact_results) {
		if (it.second >= theta) {
			exact_results[it.first] = it.second;
			exact_results_keys.insert(it.first);
		} else {
			acceptable_false_positives_candidates.insert(it.first);
		}
	}
	if (verbose) {
		cout << "done" << endl;
		cout << VERBOSE_HEADER << "reading sample results..." << flush;
	}
	map<string, double> sample_results = get_results(sample_FILE,
			theta - (epsilon / 2.0));
	fclose(sample_FILE);
	set<string> sample_results_keys;
	for (auto const &it : sample_results) {
		sample_results_keys.insert(it.first);
	}
	if (verbose) {
		cout << "done" << endl;
		cout << VERBOSE_HEADER << "computing intersection..." << flush;
	}

	vector<string> intersection(max(exact_results_keys.size(),
				sample_results_keys.size()));
	vector<string>::iterator intersection_it =
		set_intersection(exact_results_keys.begin(), exact_results_keys.end(),
				sample_results_keys.begin(), sample_results_keys.end(),
				intersection.begin());
	intersection.resize(intersection_it - intersection.begin());
	if (verbose) {
		cout << "done" << endl;
		cout << VERBOSE_HEADER << "computing false negatives..." << flush;
	}

	unsigned int false_negatives_size = 0u;
	if (intersection.size() < exact_results_keys.size()) {
		if (verbose) {
			cout << "must compute set..." << flush;
		}
		vector<string> false_negatives(exact_results_keys.size());
		vector<string>::iterator false_negatives_it =
			set_difference(exact_results_keys.begin(), exact_results_keys.end(),
					intersection.begin(), intersection.end(),
					false_negatives.begin());
		false_negatives.resize(false_negatives_it - false_negatives.begin());
		false_negatives_size = false_negatives.size();
	}
	if (verbose) {
		cout << "done" << endl;
		cout << VERBOSE_HEADER << "computing false positives..." << flush;
	}

	vector<string> false_positives(sample_results_keys.size());
	vector<string>::iterator false_positives_it =
		set_difference(sample_results_keys.begin(), sample_results_keys.end(),
				intersection.begin(), intersection.end(),
				false_positives.begin());
	false_positives.resize(false_positives_it - false_positives.begin());
	if (verbose) {
		cout << "done" << endl;
		cout << VERBOSE_HEADER << "computing acceptable false positives..."
			<< flush;
	}

	vector<string> acceptable_false_positives(max(false_positives.size(),
				acceptable_false_positives_candidates.size()));
	vector<string>::iterator acceptable_false_positives_it =
		set_intersection(false_positives.begin(), false_positives.end(),
				acceptable_false_positives_candidates.begin(),
				acceptable_false_positives_candidates.end(),
				acceptable_false_positives.begin());
	acceptable_false_positives.resize(acceptable_false_positives_it -
			acceptable_false_positives.begin());
	if (verbose) {
		cout << "done" << endl;
		cout << VERBOSE_HEADER << "computing non-acceptable false positives..."
			<< flush;
	}

	unsigned int non_acceptable_false_positives_size = 0u;
	if (acceptable_false_positives.size() < false_positives.size()) {
		if (verbose) {
			cout << "must compute set..." << flush;
		}
		vector<string> non_acceptable_false_positives(false_positives.size());
		vector<string>::iterator non_acceptable_false_positives_it =
			set_difference(false_positives.begin(), false_positives.end(),
					acceptable_false_positives_candidates.begin(),
					acceptable_false_positives_candidates.end(),
					non_acceptable_false_positives.begin());
		non_acceptable_false_positives.resize(
				non_acceptable_false_positives_it -
				non_acceptable_false_positives.begin());
		non_acceptable_false_positives_size =
			non_acceptable_false_positives.size();
		for (string itemset : non_acceptable_false_positives) {
			cerr << LOG_HEADER << "WARNING! NON ACCEPTABLE FALSE POSITIVE: "
				<<  ", freq=" << sample_results[itemset] << endl;
		}
	}
	if (verbose) {
		cout << "done" << endl;
		cout << VERBOSE_HEADER << "computing jaccard..." << flush;
	}

	unsigned int union_for_jaccard_size = sample_results_keys.size();
	if (intersection.size() < exact_results_keys.size()) {
		if (verbose) {
			cout << "must compute union..." << flush;
		}
		vector<string> union_for_jaccard(exact_results_keys.size() +
				sample_results_keys.size() - intersection.size());
		vector<string>::iterator union_for_jaccard_it =
			set_union(exact_results_keys.begin(), exact_results_keys.end(),
					sample_results_keys.begin(), sample_results_keys.end(),
					union_for_jaccard.begin());
		union_for_jaccard.resize(
				union_for_jaccard_it - union_for_jaccard.begin());
		union_for_jaccard_size = union_for_jaccard.size();
	}
    double jaccard = ((double) intersection.size()) / union_for_jaccard_size;
	if (verbose) {
		cout << "done" << endl;
		cout << VERBOSE_HEADER << "computing error statistics..." << flush;
	}

	set<string> union_for_stats = sample_results_keys;
	if (non_acceptable_false_positives_size > 0u) {
		set<string> new_union_for_stats;
		new_union_for_stats.insert(intersection.begin(), intersection.end());
		new_union_for_stats.insert(acceptable_false_positives.begin(),
				acceptable_false_positives.end());
		union_for_stats = new_union_for_stats;
	}
    double max_absolute_error = 0.0;
    double absolute_error_sum = 0.0;
    double relative_error_sum = 0.0;
    unsigned int wrong_eps = 0u;
	for (const string itemset : union_for_stats) {
		double sample_freq = sample_results[itemset];
		double exact_freq = extended_exact_results[itemset];
		double absolute_error = abs(sample_freq - exact_freq);
		absolute_error_sum += absolute_error;
		if (absolute_error > max_absolute_error) {
			max_absolute_error = absolute_error;
		}
		if (absolute_error > epsilon) {
			++wrong_eps;
		}
		relative_error_sum += absolute_error / exact_freq;
	}

	double avg_absolute_error = absolute_error_sum / (intersection.size() +
			acceptable_false_positives.size());
	double avg_relative_error = relative_error_sum / (intersection.size() +
			acceptable_false_positives.size());
	if (verbose) {
		cout << "done" << endl;
	}

	cout << "large=" << basename(argv[argc - 2]) << COMMA << "sample="
		<< basename(argv[argc - 1]) << COMMA << "e=" << epsilon << COMMA << "d="
		<< delta << COMMA << "minFreq=" << theta << COMMA << "largeFIs="
		<< exact_results_keys.size() << endl;
	cout << "inter=" << intersection.size() << COMMA << "fn="
		<< false_negatives_size << COMMA << "fp=" << false_positives.size()
		<< COMMA << "nafp=" << non_acceptable_false_positives_size << COMMA
		<< "jaccard=" << jaccard << endl;
	cout << "we=" << wrong_eps << COMMA << "maxabserr=" << max_absolute_error
		<< COMMA << "avgabserr=" << avg_absolute_error << COMMA << "avgrelerr="
		<< avg_relative_error <<endl;

    cerr << "large_res,sample_res,epsilon,delta,min_freq,orig_FIs,intersection,"
		<< "false_negs,false_pos,non_acceptable_false_pos,jaccard,wrong_eps,"
		<< "max_abs_err,avg_abs_err,avg_rel_err" << endl;
	cerr << basename(argv[argc - 2])  << COMMA <<  basename(argv[argc - 1])
		<< COMMA << epsilon << COMMA << delta << COMMA << theta << COMMA
		<< exact_results_keys.size() << COMMA << intersection.size() << COMMA
		<< false_negatives_size << COMMA << false_positives.size() << COMMA
		<< non_acceptable_false_positives_size << COMMA << jaccard << COMMA
		<< wrong_eps << COMMA << max_absolute_error << COMMA
		<< avg_absolute_error << COMMA << avg_relative_error << endl;

	return EXIT_SUCCESS;
}
