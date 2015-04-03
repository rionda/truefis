/**
 * Declarations for functions that use igraph. Code in graph.cpp .
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
#ifndef _GRAPH_H
#define _GRAPH_H

#include <forward_list>
#include <set>
#include <unordered_map>

#include <igraph/igraph.h>

igraph_t *create_antichain_graph(const std::forward_list<std::set<int> > &, const std::unordered_map<const std::set<int>*, int> &);

int get_largest_antichain_size(const std::forward_list<const std::set<int> *> &);

#endif
