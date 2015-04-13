/**
 * Sort a collection of itemsets in decreasing order according to their
 * frequency
 *
 * Copyright 2014 Matteo Riondato <matteo@cs.brown.edu>
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
 * 
 */

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <vector>

using namespace std;

// Allowing for very long transactions, just in case
static const short TRANSACTION_LINE_MAXLEN = 10000;

int main(int argc, char **argv) {
	if (argc != 2) {
		cerr << argv[0] << 
			": sort a collection of itemsets in decreasing order according to their frequency"
			<< endl; 
		cerr << "USAGE: " << argv[0] << " itemsets_file" << endl;
		return 1;
	}

	FILE *in_FILE = fopen(argv[1], "r");
	if (in_FILE == NULL) {
		perror("Error opening input file");
		return errno;
	}

	/* first line is special */
	char first_line[TRANSACTION_LINE_MAXLEN]; 
	fgets(first_line, TRANSACTION_LINE_MAXLEN, in_FILE);
	cout << first_line;

	map<long int,vector<string> > supp_map;
	
	char line[TRANSACTION_LINE_MAXLEN];
	while (! feof(in_FILE)) {
		fgets(line, TRANSACTION_LINE_MAXLEN, in_FILE);
		char *open_par = strchr(line, '(');	
		if (open_par == NULL) {
			continue;
		}
		string itemset(line, open_par - line);
		long int support = strtol(open_par + 1, NULL, 10);
		if (supp_map.count(support) == 0) { 
			supp_map[support] = vector<string>();
		}
		supp_map[support].push_back(itemset);
	}

	for (auto rit = supp_map.rbegin(); rit != supp_map.rend(); ++rit) {
		long int support = rit->first;
		vector<string> itemsets = rit->second;
		for (string itemset : itemsets) {
			cout << itemset << "(" << support << ")" << endl;
		}
	}

	fclose(in_FILE);
	return 0;
}

