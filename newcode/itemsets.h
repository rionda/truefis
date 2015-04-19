/**
 * Definitions of functions to manipulate itemsets and collections of itemsets.
 * Code in itemsets.cpp
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

#ifndef _ITEMSETS_H
#define _ITEMSETS_H

#include <list>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "config.h"

class Itemset {
	public:
		int visited;
		const std::set<int> *itemset;
		std::list<Itemset *> parents;
		std::list<Itemset *> children;
		explicit Itemset(const std::set<int>*);
		std::list<Itemset *>::iterator add_parent(Itemset *);
		std::list<Itemset *>::iterator add_child(Itemset *);
};

class Dataset {
	int items;
	int max_supp;
	int size;
	const std::string fi_path;
	const std::string path;
	public:
		Dataset(const ds_config &, const bool);
		Dataset(const std::string &, const int=-1, const int=-1, const int=-1, const std::string & = std::string());
		std::string get_fi_path() const { return fi_path; }
		std::string get_path() const { return path; }
		int get_frequent_itemsets(const double, std::map<std::set<int>, const double> &, Itemset * = NULL);
		int get_items_num(const bool = false);
		int get_max_supp(const bool = false);
		int get_size(const bool = false);
		int set_max_supp(const int);
		int set_size(const int);
};

int filter_negative_border(const Dataset &, const std::set<std::set<int> > &, std::unordered_set<const std::set<int>*> &);
int find_itemsets_in_transaction(std::set<int> &, const std::unordered_set<const std::set<int>*> &, Itemset *);
int get_closed_itemsets(const std::map<std::set<int>, const double> &, std::unordered_set<const std::set<int>*> &);
int get_maximal_itemsets(Itemset *, std::unordered_set<const std::set<int>*> &);
int get_negative_border(const std::map<std::set<int>, const double> &, const std::unordered_set<const std::set<int>*> &, std::set<std::set<int> > &);
int get_visit_id();
bool is_subset(const std::set<int> &, const std::set<int> &);
std::string itemset2string(const std::set<int> &, const char=' ');
void set_parents(Itemset *, Itemset *);
std::set<int> string2itemset(const std::string &);
bool size_comp_Itemset(Itemset *, Itemset *);
bool size_comp_nopointers(const std::set<int> &, const std::set<int> &);
void create_frequent_itemsets_tree(const std::map<std::set<int>, const double, bool (*)(const std::set<int> &, const std::set<int> &)> &, Itemset* const);
#endif
