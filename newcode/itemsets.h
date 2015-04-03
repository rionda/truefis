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

#include <forward_list>
#include <set>
#include <string>

std::set<int> line2itemset(const std::string &);

int get_closed_itemsets(const std::forward_list<std::set<int> > &, std::forward_list<std::set<int> > &);

int get_maximal_itemsets(const std::forward_list<std::set<int> > &, std::forward_list<std::set<int> > &);
