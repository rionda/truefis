/**
 * Declarations for functions that use CPLEX. Code in sukp.cpp .
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
#ifndef _SUKP_H
#define _SUKP_H

#include <forward_list>
#include <set>

#include <ilcplex/ilocplex.h>

int get_CPLEX_model(IloModel &, const IloEnv &, const std::unordered_set<int> &, const std::unordered_set<const std::set<int>*> &, const int, const bool);
double get_SUKP_profit(IloCplex &);
int set_capacity(IloModel &, const int);
int set_CPLEX_params(IloCplex &, const double=0.1, const double=2.0, const int=600);

#endif
