# Finding the True Frequent Itemsets 
#
# Copyright 2014 Matteo Riondato <matteo@cs.brown.edu> and Fabio Vandin
# <vandinfa@imada.sdu.dk>
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import itertools, locale, math, os, os.path, subprocess, sys, tempfile
import networkx as nx
import epsilon
from datasetsinfo import ds_stats


def error_exit(msg):
    """Print message msg to stderr and exit."""
    sys.stderr.write(msg)
    sys.exit(1)


def check_neg_bord(neg_bord, cand):
    """Check that cand can be added to neg_bord.

    Return true if none of the sets in neg_bord is a superset or a subset of
    cand
    """
    ret = True
    for i in neg_bord:
        if i < cand or cand < i: 
            print(i, cand)
            ret = False
            break
    return ret


def powerset(iterable):
    """Build the powerset of the elements from iterable and return it as list.

    powerset([1,2,3]) --> () (1,) (2,) (3,) (1,2) (1,3) (2,3) (1,2,3)

    from http://docs.python.org/3.3/library/itertools.html

    """
    s = list(iterable)
    return itertools.chain.from_iterable(itertools.combinations(s, r) for r in range(len(s)+1))


def create_results(file_name, min_freq):
    """Read Frequent Itemsets at threshold min_freq from filename.
    
    Return a dict where the keys are frequent itemsets (represented as
    frozensets) and the values are their frequencies. Only itemsets with
    frequency at least min_freq are returned.

    The first line of the results file file_name has the form (SIZE) where SIZE
    is the number of transactions in the dataset from which the itemsets where
    extracted. The following lines have the format N1 N2 N3 N4 (SUPPORT) where
    NX is an item (integer) and SUPPORT is the support of the itemset. The
    itemsets are expected to appear in the file in reverse sorted order by
    support (from most frequent to least frequent).

    """
    results = dict()
    with open(file_name) as FILE:
        size_line = FILE.readline()
        try:
            size = int(size_line[1:-2])
        except ValueError:
            error_exit("Cannot compute size of the original dataset: {} is not a number\n".format(size_line[1:-2]))
        prev_freq = 1.0
        for line in FILE:
            if line.find("(") > -1:
                tokens = line.split("(")
                itemset = frozenset(map(int, tokens[0].split()))
                support = int(tokens[1][:-2])
                freq = support / size
                if freq > prev_freq:
                    error_exit("Results file must be sorted\n")
                if freq >= min_freq:
                    results[itemset] = freq
                    prev_freq = freq
                else:
                    break
    return results


def main():
    """This function does everything, from arguments checking, to computing the
    True Frequent Itemsets (TFIs) from the dataset, to comparing the set of
    TFI's extracted from the dataset with the original FI's and printing out a
    summary of the comparison and of other useful statistics.
    
    Not really an example of modular code. =)
    
    """
    # Verify arguments
    if len(sys.argv) != 9: 
        error_exit("Usage: {} PHASES={{2|3}} ADDITIONALKNOWLEDGE={{0|1}} DELTA MINFREQ GAP DATASETNAME ORIGRES SAMPLERES\n".format(os.path.basename(sys.argv[0])))
    dataset_name = sys.argv[6]
    orig_res_filename = os.path.expanduser(sys.argv[7])
    if not os.path.isfile(orig_res_filename):
        error_exit("{} does not exist, or is not a file\n".format(orig_res_filename))
    sample_res_filename = os.path.expanduser(sys.argv[8])
    if not os.path.isfile(sample_res_filename):
        error_exit("{} does not exist, or is not a file\n".format(sample_res_filename))
    try:
        phases = int(sys.argv[1])
    except ValueError:
        error_exit("{} is not a number\n".format(sys.argv[1]))
    try:
        use_additional_knowledge = int(sys.argv[2])
    except ValueError:
        error_exit("{} is not a number\n".format(sys.argv[2]))
    try:
        delta = float(sys.argv[3])
    except ValueError:
        error_exit("{} is not a number\n".format(sys.argv[3]))
    try:
        min_freq = float(sys.argv[4])
    except ValueError:
        error_exit("{} is not a number\n".format(sys.argv[4]))
    try:
        gap = float(sys.argv[5])
    except ValueError:
        error_exit("{} is not a number\n".format(sys.argv[5]))

    # One may want to play with giving different values for the different error
    # probabilities, but there isn't really much point in it.
    if phases == 3:
        delta_1 = delta_2 = delta_3 = 1 - math.pow(1 - delta, 1.0 / 3.0 )
    elif phases == 2:
        delta_1 = delta_2 = 1 - math.pow(1 - delta, 0.5) 
    else:
        error_exit("'PHASES' parameter must be 2 or 3 ('{}' given)".format(phases))

    # Compute the first epsilon using results from the paper (Riondato and Upfal 2014)
    # Incorporate or not 'previous knowledge' about generative process in
    # computation of the VC-dimension, depending on the option passed on the
    # command line
    (eps_vc_dim, eps_emp_vc_dim, returned) = epsilon.epsilon_dataset(delta_1, dataset_name, use_additional_knowledge) 
    epsilon_1 = min(eps_vc_dim, eps_emp_vc_dim)

    # Extract the first (and largest) set of itemsets.
    lower_bound_freq = min_freq - epsilon_1 - (1 / ds_stats[dataset_name]['size'])
    freq_itemsets_1_dict = create_results(sample_res_filename, lower_bound_freq)
    freq_itemsets_1 = sorted(freq_itemsets_1_dict.keys(), key=len, reverse=True)
    freq_itemsets_1_set = frozenset(freq_itemsets_1)
    sys.stderr.write("First set of FI's: {} itemsets\n".format(len(freq_itemsets_1)))
    sys.stderr.flush()

    constr_start_str = "cplex.SparsePair(ind = ["
    constr_end_str = "], val = vals)"

    # we do this phase only if requested
    if phases == 3:

        # Start building the CPLEX optimization problem 
        freq_items_1 = set()
        for itemset in freq_itemsets_1_set:
            freq_items_1 |= itemset
        freq_items_1_sorted = sorted(freq_items_1)
        freq_items_1_num = len(freq_items_1)
        freq_itemsets_1_num = len(freq_itemsets_1_set)

        items = ds_stats[dataset_name]['items']
        sorted_items = sorted(items)
        items_num = len(items)
        non_freq_items_1 = items - freq_items_1

        capacity = min(items_num - 1, freq_items_1_num - 1)

        # For each frequent item, find the itemsets which it belongs to. We need
        # this for the constraints in the optimization problem.
        freq_items_1_in_sets_dict = dict()
        itemset_indexes_dict = dict()
        for itemset_index in range(freq_itemsets_1_num):
            itemset = freq_itemsets_1[itemset_index]
            for item in itemset:
                if item in freq_items_1_in_sets_dict:
                    freq_items_1_in_sets_dict[item].append(itemset_index)
                else:
                    freq_items_1_in_sets_dict[item] = [itemset_index]
            itemset_indexes_dict[itemset] = itemset_index

        #There is one variable for each itemset and one for each item
        vars_num = freq_itemsets_1_num + freq_items_1_num
        constr_names = []

        #Write the Python 2.7 script through which we call cplex.
        (tmpfile_handle, tmpfile_name) = tempfile.mkstemp(prefix="cplx", dir=os.environ['PWD'], text=True)
        os.close(tmpfile_handle)
        with open(tmpfile_name, 'wt') as cplex_script:
            cplex_script.write("capacity = {}\n".format(capacity))
            cplex_script.write("import cplex, os, sys\n")
            cplex_script.write("from cplex.exceptions import CplexError\n")
            cplex_script.write("\n")
            cplex_script.write("\n")
            cplex_script.write("os.environ[\"ILOG_LICENSE_FILE\"] = \"/local/projects/cplex/ilm/site.access.ilm\"\n") 
            cplex_script.write("vals = [-1.0, 1.0]\n")
            cplex_script.write("sets_num = {}\n".format(freq_itemsets_1_num))
            cplex_script.write("items_num = {}\n".format(freq_items_1_num))
            cplex_script.write("vars_num = {}\n".format(vars_num))
            cplex_script.write("my_ub = [1.0] * vars_num\n")
            cplex_script.write("my_types = \"\".join(\"I\" for i in range(vars_num))\n")
            cplex_script.write("my_obj = ([1.0] * sets_num) + ([0.0] * items_num)\n")
            cplex_script.write("my_colnames = [\"set{0}\".format(i) for i in range(sets_num)] + [\"item{0}\".format(j) for j in range(items_num)]\n")
            cplex_script.write("rows = [ ")

            # Create constraints and write them to the script
            sys.stderr.write("Writing knapsack constraints...")
            constr_num = 0
            for item_index in range(freq_items_1_num):
                try:
                    for itemset_index in freq_items_1_in_sets_dict[freq_items_1_sorted[item_index]]:
                        constr_str = "".join((constr_start_str,
                                "\"set{}\",\"item{}\"".format(itemset_index,item_index), constr_end_str))
                        cplex_script.write("{},".format(constr_str))
                        constr_num += 1
                        name = "s{}i{}".format(item_index, itemset_index)
                        constr_names.append(name)
                except KeyError:
                    sys.stderr.write("item_index={} freq_items_1_sorted[item_index]={}\n".format(item_index, freq_items_1_sorted[item_index]))
                    sys.stderr.write("{} in items: {}\n".format(freq_items_1_sorted[item_index], freq_items_1_sorted[item_index] in items))
                    sys.stderr.write("{} in freq_items_1: {}\n".format(freq_items_1_sorted[item_index], freq_items_1_sorted[item_index] in freq_items_1))
                    sys.stderr.write("{} in non_freq_items_1: {}\n".format(sorted_items[item_index], sorted_items[item_index] in non_freq_items_1))
                    sys.exit(1)

            # Create capacity constraint and write it to script
            constr_str = "".join((constr_start_str, ",".join("\"item{}\"".format(j) for
                j in range(freq_items_1_num)), "], val=[", ",".join("1.0" for j in
                    range(freq_items_1_num)), "])"))
            cplex_script.write(constr_str)
            last_tell = cplex_script.tell()
            cplex_script.write(",")
            cap_constr_name = "capacity"
            constr_names.append(cap_constr_name)
            sys.stderr.write("done\n")
            sys.stderr.flush()

            sys.stderr.write("vars_num={} freq_itemsets_1_num={} freq_items_1_num={} constr_num={}\n".format(vars_num, freq_itemsets_1_num,
                       freq_items_1_num, constr_num))
            sys.stderr.flush()

            cplex_script.seek(last_tell) # go back one character to remove last comma ","
            cplex_script.write("]\n")
            cplex_script.write("my_rownames = {}\n".format(constr_names))
            cplex_script.write("constr_num = {}\n".format(constr_num))
            cplex_script.write("my_senses = [\"G\"] * constr_num + [\"L\"]\n")
            cplex_script.write("my_rhs = [0.0] * constr_num + [capacity]\n")
            cplex_script.write("\n")
            cplex_script.write("try:\n")
            cplex_script.write("    prob = cplex.Cplex()\n")
            cplex_script.write("    prob.set_error_stream(sys.stderr)\n")
            cplex_script.write("    prob.set_log_stream(sys.stderr)\n")
            cplex_script.write("    prob.set_results_stream(sys.stderr)\n")
            cplex_script.write("    prob.set_warning_stream(sys.stderr)\n")
            #cplex_script.write("    prob.parameters.mip.strategy.file.set(2)\n")
            cplex_script.write("    prob.parameters.mip.tolerances.mipgap.set({})\n".format(gap))
            cplex_script.write("    prob.parameters.timelimit.set({})\n".format(600))
            #cplex_script.write("    prob.parameters.mip.strategy.variableselect.set(3) # strong branching\n")
            cplex_script.write("    prob.objective.set_sense(prob.objective.sense.maximize)\n")
            cplex_script.write("    prob.variables.add(obj = my_obj, ub = my_ub, types = my_types, names = my_colnames)\n")
            #cplex_script.write("    prob.variables.add(obj = my_obj, ub = my_ub, names = my_colnames)\n")
            cplex_script.write("    prob.linear_constraints.add(lin_expr = rows, senses = my_senses, rhs = my_rhs, names = my_rownames)\n")
            #cplex_script.write("    prob.write(\"itemsets.lp\")\n")
            cplex_script.write("    prob.MIP_starts.add(cplex.SparsePair(ind = [i for i in range(vars_num)], val = [1.0] * vars_num), prob.MIP_starts.effort_level.auto)\n")
            cplex_script.write("    prob.solve()\n")
            cplex_script.write("    print (prob.solution.get_status(),prob.solution.status[prob.solution.get_status()],prob.solution.MIP.get_best_objective(),prob.solution.MIP.get_mip_relative_gap())\n")
            cplex_script.write("except CplexError, exc:\n")
            cplex_script.write("    print exc\n")

        # Run the script, solve the optimization problem, extract solution
        try:
            cplex_output_binary_str = subprocess.check_output(["python2.6", tmpfile_name], cwd=os.environ["PWD"])
        except subprocess.CalledProcessError as err:
            os.remove(tmpfile_name)
            error_exit("CPLEX exited with error code {}: {}\n".format(err.returncode, err.output))
        #finally:
        #    os.remove(tmpfile_name)

        cplex_output = cplex_output_binary_str.decode(locale.getpreferredencoding())
        cplex_output_lines = cplex_output.split("\n")
        sys.stderr.write("{}\n".format(cplex_output_lines))
        cplex_solution_line = cplex_output_lines[-1 if len(cplex_output_lines[-1]) > 0 else -2]
        try:
            cplex_solution = eval(cplex_solution_line)
        except Exception:
            os.remove(tmpfile_name)
            error_exit("Error evaluating the CPLEX solution line: {}\n".format(cplex_solution_line))

        sys.stderr.write("{}\n".format(cplex_solution))
        #if cplex_solution[0] not in (1, 101, 102):
        #    error_exit("CPLEX didn't find the optimal solution: {} {} {}\n".format(cplex_solution[0], cplex_solution[1], cplex_solution[2]))

        optimal_sol_upp_bound = int(math.ceil(cplex_solution[2] / (1 - cplex_solution[3])))
        
        # Compute the first candidate for epsilon_2 
        not_emp_vc_dim = int(math.floor(math.log2(optimal_sol_upp_bound))) +1
        not_emp_epsilon_2 = epsilon.get_eps_vc_dim(delta_2,
                ds_stats[dataset_name]['size'], not_emp_vc_dim,
                ds_stats[dataset_name]['maxsupp'] / ds_stats[dataset_name]['size'])
        sys.stderr.write("{} {} {} {}\n".format(capacity, optimal_sol_upp_bound,
            not_emp_vc_dim, not_emp_epsilon_2))

        # Solve the "empirical" version of the optimization problem using the
        # length distribution.
        capacity_str_len = len(str(capacity))

        lengths_dict = ds_stats[dataset_name]['lengths']
        lengths = sorted(lengths_dict.keys(), reverse=True)

        longer_equal = 0
        for i in range(len(lengths)):
            cand_len = lengths[i]
            longer_equal += lengths_dict[cand_len]

            # XXX THINK ABOUT THIS.
            if cand_len > capacity:
                cand_len = capacity

            # Modify script to use cand_len as capacity
            with open(tmpfile_name, 'r+t') as cplex_script:
                cplex_script.seek(0)
                cplex_script.write("capacity = {}\n".format(str(cand_len).ljust(capacity_str_len)))

            # Run script, solve optimization problem, extract solution
            try:
                cplex_output_binary_str = subprocess.check_output(["python2.6", tmpfile_name], cwd=os.environ["PWD"])
            except subprocess.CalledProcessError as err:
                os.remove(tmpfile_name)
                error_exit("CPLEX exited with error code {}: {}\n".format(err.returncode, err.output))

            cplex_output = cplex_output_binary_str.decode(locale.getpreferredencoding())
            cplex_output_lines = cplex_output.split("\n")
            cplex_solution_line = cplex_output_lines[-1 if len(cplex_output_lines[-1]) > 0 else -2]
            try:
                cplex_solution = eval(cplex_solution_line)
            except Exception:
                error_exit("Error evaluating the CPLEX solution line: {}\n".format(cplex_solution_line))

            sys.stderr.write("{}\n".format(cplex_solution))
            #if cplex_solution[0] not in (1, 101, 102):
             #   error_exit("CPLEX didn't find the optimal solution: {} {} {}\n".format(cplex_solution[0], cplex_solution[1], cplex_solution[2]))

            #if cplex_solution[0] == 102:
            optimal_sol_upp_bound = int(math.ceil(cplex_solution[2] / (1 - cplex_solution[3])))
            #else:
            #    optimal_sol_upp_bound = cplex_solution[0]

            # Compute candidate empirical VC-dimension
            emp_vc_dim = int(math.floor(math.log2(optimal_sol_upp_bound))) +1

            sys.stderr.write("cand_len={} longer_equal={} emp_vc_dim={} optimal_sol_upp_bound={}\n".format(cand_len, longer_equal, emp_vc_dim,
                optimal_sol_upp_bound))
            sys.stderr.flush()

            # Stopping condition satisfied, we can exit
            if emp_vc_dim <= longer_equal:
                break
        #sys.stderr.write("{} {} {}\n".format(vc_dim_cand, vc_dim_cand2, vc_dim_cand3))
        os.remove(tmpfile_name)

        # Compute the second candidate for epsilon_2
        emp_epsilon_2 = epsilon.get_eps_emp_vc_dim(delta_2,
                ds_stats[dataset_name]['size'], emp_vc_dim,
                ds_stats[dataset_name]['maxsupp'] /ds_stats[dataset_name]['size'])
        sys.stderr.write("{} {} {} {}\n".format(cand_len, optimal_sol_upp_bound,
            emp_vc_dim, emp_epsilon_2))

        epsilon_2 = min(emp_epsilon_2, not_emp_epsilon_2)

        #sys.stderr.write("{} {} {} {}\n".format(len(maximal_itemsets), negative_border_size, vc_dim, epsilon_second))

    else: # end of 'if phases == 3'
        epsilon_2 = epsilon_1

    # Extract FI's with frequency at least theta-epsilon_2
    freq_itemsets_2_dict = dict()
    freq_itemsets_2 = []
    freq_items_2 = set()
    for itemset in freq_itemsets_1_dict:
        if freq_itemsets_1_dict[itemset] >= min_freq - epsilon_2:
            freq_itemsets_2_dict[itemset] = freq_itemsets_1_dict[itemset]
            freq_itemsets_2.append(itemset)
            if len(itemset) == 1:
                freq_items_2 |= itemset
    freq_itemsets_2_set = frozenset(freq_itemsets_2)
    freq_items_2_num = len(freq_items_2)
    freq_items_2_sorted = sorted(freq_items_2)
    non_freq_items_2 = items - freq_items_2

    # Compute maximal itemsets. We will use them to compute the negative border.
    # An itemset is maximal frequent if none of its immediate supersets is frequent.
    sys.stderr.write("Computing maximal itemsets...")
    maximal_itemsets = []
    for candidate in freq_itemsets_2:
        to_add = True
        for item in freq_items_2:
            if item in candidate:
                continue # as we are not creating a superset.
            superset = candidate | frozenset([item]) # create superset by adding a frequent item
            if superset in freq_itemsets_2_set:
                to_add = False
                break
        if to_add:
            #assert check_neg_border(maximal_itemsets, candidate)
            maximal_itemsets.append(candidate)
    maximal_itemsets_size = len(maximal_itemsets)
    sys.stderr.write("done. Found {} maximal itemsets\n".format(maximal_itemsets_size))
    sys.stderr.flush()

    # Compute the negative border 
    sys.stderr.write("Computing negative border...")
    sys.stderr.flush()
    negative_border = set()
    # The idea is to look for "children" of maximal itemsets, and for "siblings"
    # of maximal itemsets
    for maximal in maximal_itemsets:
        for item_to_remove_from_maximal in maximal:
            reduced_maximal = maximal - frozenset([item_to_remove_from_maximal])
            for item in freq_items_2:
                if item in maximal:
                    continue
                candidate = reduced_maximal | frozenset([item]) # create sibling
                if candidate in freq_itemsets_2_set:
                    continue
                if candidate in negative_border:
                    continue
                to_add = True
                for item_to_remove in candidate:
                    subset = candidate - frozenset([item_to_remove])
                    if subset not in freq_itemsets_2_set:
                        to_add = False
                        break
                if to_add:
                    #assert check_neg_bord(negative_border, candidate)
                    negative_border.add(candidate)
                if not to_add: # if we added the sibling, there's no way we can add the child
                    candidate2 = maximal | frozenset([item]) # create child
                    if candidate2 in negative_border:
                        continue
                    to_add = True
                    for item_to_remove in candidate2:
                        subset = candidate2 - frozenset([item_to_remove])
                        if subset not in freq_itemsets_2_set:
                            to_add = False
                            break
                    if to_add:
                        #assert check_neg_bord(negative_border, candidate2)
                        negative_border.add(candidate2)
    sys.stderr.write("done. Length now: {}\n".format(len(negative_border)))
    sys.stderr.flush()

    # Add the "base set" (terrible name), that is the set of itemsets with
    # frequency < min_freq + epsilon_2
    # This is part of what makes it a superset.
    sys.stderr.write("Creating negative border base set...")
    max_freq = 0
    for itemset in freq_itemsets_2_dict:
        if freq_itemsets_2_dict[itemset] < min_freq + epsilon_2: 
            negative_border.add(itemset)
            if freq_itemsets_2_dict[itemset] > max_freq:
                max_freq = freq_itemsets_2_dict[itemset]
    sys.stderr.write("done. Length now: {} ({} non-freq items)\n".format(len(negative_border), len(non_freq_items_2)))
    sys.stderr.flush()
    negative_border = sorted(negative_border,key=len, reverse=True)
    negative_border_size = len(negative_border)

    # Create the graph that we will use to compute the chain constraints.
    # The nodes are the itemsets in negative_border. There is an edge between
    # two nodes if one is contained in the other or vice-versa.
    # Cliques on this graph are chains.
    sys.stderr.write("Creating graph...")
    sys.stderr.flush()
    graph = nx.Graph()
    graph.add_nodes_from(negative_border)

    negative_border_items_in_sets_dict = dict()
    negative_border_itemset_index = 0
    itemset_indexes_dict = dict()
    for first_itemset_index in range(negative_border_size):
        #sys.stderr.write("{} out of {}\n".format(first_itemset_index, negative_border_size))
        #sys.stderr.flush()
        first_itemset = negative_border[first_itemset_index]
        for second_itemset_index in range(first_itemset_index +1,
                negative_border_size):
            #sys.stderr.write("\t{} out of {}\n".format(second_itemset_index, negative_border_size - first_itemset_index))
            #sys.stderr.flush()
            second_itemset = negative_border[second_itemset_index]
            if first_itemset < second_itemset or second_itemset < first_itemset:
                graph.add_edge(first_itemset, second_itemset)
        for item in first_itemset:
            if item in negative_border_items_in_sets_dict:
                negative_border_items_in_sets_dict[item].append(negative_border_itemset_index)
            else:
                negative_border_items_in_sets_dict[item] = [negative_border_itemset_index]
        itemset_indexes_dict[first_itemset] = negative_border_itemset_index
        negative_border_itemset_index += 1
    sys.stderr.write("done\n")
    sys.stderr.flush()

    vars_num = negative_border_size + freq_items_2_num
    constr_names = []

    (tmpfile_handle, tmpfile_name) = tempfile.mkstemp(prefix="cplx", dir=os.environ['PWD'], text=True)
    os.close(tmpfile_handle)
    with open(tmpfile_name, 'wt') as cplex_script:
        cplex_script.write("capacity = {}\n".format(freq_items_2_num - 1))
        cplex_script.write("import cplex, os, sys\n")
        cplex_script.write("from cplex.exceptions import CplexError\n")
        cplex_script.write("\n")
        cplex_script.write("\n")
        cplex_script.write("os.environ[\"ILOG_LICENSE_FILE\"] = \"/local/projects/cplex/ilm/site.access.ilm\"\n") 
        cplex_script.write("vals = [-1.0, 1.0]\n")
        cplex_script.write("sets_num = {}\n".format(negative_border_size))
        cplex_script.write("items_num = {}\n".format(freq_items_2_num))
        cplex_script.write("vars_num = {}\n".format(vars_num))
        cplex_script.write("my_ub = [1.0] * vars_num\n")
        cplex_script.write("my_types = \"\".join(\"I\" for i in range(vars_num))\n")
        cplex_script.write("my_obj = ([1.0] * sets_num) + ([0.0] * items_num)\n")
        cplex_script.write("my_colnames = [\"set{0}\".format(i) for i in range(sets_num)] + [\"item{0}\".format(j) for j in range(items_num)]\n")
        cplex_script.write("rows = [ ")

        sys.stderr.write("Writing knapsack constraints...")
        constr_num = 0
        for item_index in range(freq_items_2_num):
            try:
                for itemset_index in negative_border_items_in_sets_dict[freq_items_2_sorted[item_index]]:
                    constr_str = "".join((constr_start_str,
                            "\"set{}\",\"item{}\"".format(itemset_index,item_index), constr_end_str))
                    cplex_script.write("{},".format(constr_str))
                    constr_num += 1
                    name = "s{}i{}".format(item_index, itemset_index)
                    constr_names.append(name)
            except KeyError:
                sys.stderr.write("item_index={} sorted_items[item_index]={}\n".format(item_index, sorted_items[item_index]))
                sys.stderr.write("{} in items: {}\n".format(sorted_items[item_index], sorted_items[item_index] in items))
                sys.stderr.write("{} in freq_items_2: {}\n".format(sorted_items[item_index], sorted_items[item_index] in
                        freq_items_2))
                sys.stderr.write("{} in non_freq_items_2: {}\n".format(sorted_items[item_index], sorted_items[item_index] in
                        non_freq_items_2))
                in_pos_border = False
                pos_border_itemset = frozenset()
                for itemset in maximal_itemsets:
                    if freq_items_2_sorted[item_index] in itemset:
                        in_pos_border = True
                        pos_border_itemset = itemset
                        break
                sys.stderr.write("{} in maximal_itemsets: {}. Itemset: {}\n".format(freq_items_2_sorted[item_index], in_pos_border, pos_border_itemset))
                in_neg_border = False
                neg_border_itemset = frozenset()
                for itemset in negative_border:
                    if freq_items_2_sorted[item_index] in itemset:
                        in_neg_border = True
                        neg_border_itemset = itemset
                        break
                sys.stderr.write("{} in negative_border: {}. Itemset: {}\n".format(freq_items_2_sorted[item_index], in_neg_border, neg_border_itemset))
                sys.exit(1)

        # Create capacity constraints and write it to script
        constr_str = "".join((constr_start_str, ",".join("\"item{}\"".format(j) for
            j in range(freq_items_2_num)), "], val=[", ",".join("1.0" for j in range(freq_items_2_num)), "])"))
        cplex_script.write(constr_str)
        last_tell = cplex_script.tell()
        cplex_script.write(",")
        cap_constr_name = "capacity"
        constr_names.append(cap_constr_name)
        sys.stderr.write("done\n")
        sys.stderr.flush()

        # Create chain constraints and write them to script
        sys.stderr.write("Writing chain constraints...")
        chains_index = 0
        for clique in nx.find_cliques(graph):
            if len(clique) == 1:
                continue
            constr_str = "".join((constr_start_str, ",".join("\"set{}\"".format(j)
                for j in map(lambda x: itemset_indexes_dict[x], clique)), "], val=[1.0] * {}".format(len(clique)), ")")) 
            cplex_script.write(constr_str)
            last_tell = cplex_script.tell()
            cplex_script.write(",")
            name = "chain{}".format(chains_index)
            constr_names.append(name)
            chains_index += 1
        sys.stderr.write("done\n")
        sys.stderr.flush()

        sys.stderr.write("vars_num={} negative_border_size={} items_num={} constr_num={} chains_index={}\n".format(vars_num, negative_border_size, freq_items_2_num, constr_num, chains_index))
        sys.stderr.flush()

        cplex_script.seek(last_tell) # go back one character to remove last comma ","
        cplex_script.write("]\n")
        cplex_script.write("my_rownames = {}\n".format(constr_names))
        cplex_script.write("constr_num = {}\n".format(constr_num))
        cplex_script.write("chain_constr_num = {}\n".format(chains_index))
        cplex_script.write("my_senses = [\"G\"] * constr_num + [\"L\"] + [\"L\"] * chain_constr_num\n")
        cplex_script.write("my_rhs = [0.0] * constr_num + [capacity] + [1.0] * chain_constr_num\n")
        cplex_script.write("\n")
        cplex_script.write("try:\n")
        cplex_script.write("    prob = cplex.Cplex()\n")
        cplex_script.write("    prob.set_error_stream(sys.stderr)\n")
        cplex_script.write("    prob.set_log_stream(sys.stderr)\n")
        cplex_script.write("    prob.set_results_stream(sys.stderr)\n")
        cplex_script.write("    prob.set_warning_stream(sys.stderr)\n")
        #cplex_script.write("    prob.parameters.mip.strategy.file.set(2)\n")
        cplex_script.write("    prob.parameters.mip.tolerances.mipgap.set({})\n".format(gap))
        cplex_script.write("    prob.parameters.timelimit.set({})\n".format(600))
        #cplex_script.write("    prob.parameters.mip.strategy.variableselect.set(3) # strong branching\n")
        cplex_script.write("    prob.objective.set_sense(prob.objective.sense.maximize)\n")
        cplex_script.write("    prob.variables.add(obj = my_obj, ub = my_ub, types = my_types, names = my_colnames)\n")
        cplex_script.write("    prob.linear_constraints.add(lin_expr = rows, senses = my_senses, rhs = my_rhs, names = my_rownames)\n")
        cplex_script.write("    prob.MIP_starts.add(cplex.SparsePair(ind = [i for i in range(vars_num)], val = [1.0] * vars_num), prob.MIP_starts.effort_level.auto)\n")
        cplex_script.write("    prob.solve()\n")
        cplex_script.write("    print (prob.solution.get_status(),prob.solution.status[prob.solution.get_status()],prob.solution.MIP.get_best_objective(),prob.solution.MIP.get_mip_relative_gap())\n")
        cplex_script.write("except CplexError, exc:\n")
        cplex_script.write("    print exc\n")

    # Run script, solve optimization problem, extract solution
    my_environ = os.environ
    if "ILOG_LICENSE_FILE" not in my_environ:
        my_environ["ILOG_LICENSE_FILE"] = "/local/projects/cplex/ilm/site.access.ilm"
    try:
        cplex_output_binary_str = subprocess.check_output(["python2.6", tmpfile_name], env = my_environ, cwd=os.environ["PWD"])
    except subprocess.CalledProcessError as err:
        os.remove(tmpfile_name)
        error_exit("CPLEX exited with error code {}: {}\n".format(err.returncode, err.output))
    #finally:
    #    os.remove(tmpfile_name)

    cplex_output = cplex_output_binary_str.decode(locale.getpreferredencoding())
    cplex_output_lines = cplex_output.split("\n")
    cplex_solution_line = cplex_output_lines[-1 if len(cplex_output_lines[-1]) > 0 else -2]
    try:
        cplex_solution = eval(cplex_solution_line)
    except Exception:
        error_exit("Error evaluating the CPLEX solution line: {}\n".format(cplex_solution_line))

    sys.stderr.write("{}\n".format(cplex_solution))
    #if cplex_solution[0] not in (1, 101, 102):
    #    error_exit("CPLEX didn't find the optimal solution: {} {} {}\n".format(cplex_solution[0], cplex_solution[1], cplex_solution[2]))

    optimal_sol_upp_bound = int(math.ceil(cplex_solution[2] / (1 - cplex_solution[3])))

    #Compute non-empirical VC-dimension and first candidate to epsilon_3
    not_emp_vc_dim = int(math.floor(math.log2(optimal_sol_upp_bound))) +1
    not_emp_epsilon_3 = epsilon.get_eps_vc_dim(delta_3,
            ds_stats[dataset_name]['size'], not_emp_vc_dim, max_freq)
    sys.stderr.write("{} {} {} {}\n".format(items_num - 1, optimal_sol_upp_bound, not_emp_vc_dim, not_emp_epsilon_3))

    # Loop to compute empirical VC-dimension using lengths distribution
    items_num_str_len = len(str(freq_items_2_num-1))
    longer_equal = 0
    for i in range(len(lengths)):
        cand_len = lengths[i]
        # XXX THINK ABOUT THIS
        if cand_len == items_num: 
            continue
        longer_equal += lengths_dict[cand_len]
        if cand_len >= freq_items_2_num:
            cand_len = freq_items_2_num - 1

        # Modify the script to use the new capacity.
        with open(tmpfile_name, 'r+t') as cplex_script:
            cplex_script.seek(0)
            cplex_script.write("capacity = {}\n".format(str(cand_len).ljust(items_num_str_len)))
        # Run the script, solve optimization problem, extract solution
        my_environ = os.environ
        if "ILOG_LICENSE_FILE" not in my_environ:
            my_environ["ILOG_LICENSE_FILE"] = "/local/projects/cplex/ilm/site.access.ilm"
        try:
            cplex_output_binary_str = subprocess.check_output(["python2.6", tmpfile_name], env = my_environ, cwd=os.environ["PWD"])
        except subprocess.CalledProcessError as err:
            os.remove(tmpfile_name)
            error_exit("CPLEX exited with error code {}: {}\n".format(err.returncode, err.output))
        #finally:
        #    os.remove(tmpfile_name)

        cplex_output = cplex_output_binary_str.decode(locale.getpreferredencoding())
        cplex_output_lines = cplex_output.split("\n")
        cplex_solution_line = cplex_output_lines[-1 if len(cplex_output_lines[-1]) > 0 else -2]
        try:
            cplex_solution = eval(cplex_solution_line)
        except Exception:
            error_exit("Error evaluating the CPLEX solution line: {}\n".format(cplex_solution_line))

        sys.stderr.write("{}\n".format(cplex_solution))
        #if cplex_solution[0] not in (1, 101, 102):
         #   error_exit("CPLEX didn't find the optimal solution: {} {} {}\n".format(cplex_solution[0], cplex_solution[1], cplex_solution[2]))

        #if cplex_solution[0] == 102:
        optimal_sol_upp_bound = int(math.ceil(cplex_solution[2] / (1 - cplex_solution[3])))
        #else:
        #    optimal_sol_upp_bound = cplex_solution[0]

        emp_vc_dim = int(math.floor(math.log2(optimal_sol_upp_bound))) +1

        sys.stderr.write("cand_len={} longer_equal={} emp_vc_dim={} optimal_sol_upp_bound={}\n".format(cand_len, longer_equal, emp_vc_dim,
            optimal_sol_upp_bound))
        sys.stderr.flush()

        # If stopping condition is satisfied, exit.
        if emp_vc_dim <= longer_equal:
            break
    #sys.stderr.write("{} {} {}\n".format(vc_dim_cand, vc_dim_cand2, vc_dim_cand3))
    os.remove(tmpfile_name)
    
    # Compute second candidate to epsilon_3
    emp_epsilon_3 = epsilon.get_eps_emp_vc_dim(delta_3,
            ds_stats[dataset_name]['size'], emp_vc_dim, max_freq)
    sys.stderr.write("{} {} {} {}\n".format(cand_len, optimal_sol_upp_bound, emp_vc_dim, emp_epsilon_3))

    epsilon_3 = min(emp_epsilon_3, not_emp_epsilon_3)

    #sys.stderr.write("{} {} {} {}\n".format(len(maximal_itemsets), negative_border_size, vc_dim, epsilon_second))

    # Extract TFIs using epsilon_3
    sample_res = dict()
    for itemset in freq_itemsets_2_dict:
        if freq_itemsets_2_dict[itemset] >= min_freq + epsilon_3:
            sample_res[itemset] = freq_itemsets_2_dict[itemset]

    # Do comparison between the original set of TFIs and the one extracted
    # from the sample.
    # TODO some of the following code could use the 'statistics' module from
    # Python-3.4.
    orig_res = create_results(orig_res_filename, min_freq)
    orig_res_set = set(orig_res.keys())
    sample_res_set = set(sample_res.keys())

    intersection = orig_res_set & sample_res_set

    false_negatives = len(orig_res_set - sample_res_set)

    false_positives = len(sample_res_set - orig_res_set)
    if false_positives > 0:
        for itemset in sample_res_set - orig_res_set:
            sys.stderr.write("{} {}\n".format(itemset, sample_res[itemset]))

    jaccard = len(intersection) / len(orig_res_set | sample_res_set) 

    max_absolute_error = 0.0
    absolute_error_sum = 0.0
    relative_error_sum = 0.0
    wrong_eps = 0
    for itemset in intersection:
        absolute_error = abs(sample_res[itemset] - orig_res[itemset])
        absolute_error_sum += absolute_error
        if absolute_error > max_absolute_error:
            max_absolute_error = absolute_error
        if absolute_error > epsilon_3:
            wrong_eps = wrong_eps + 1
        relative_error_sum += absolute_error / orig_res[itemset]

    if len(intersection) > 0:
        avg_absolute_error = absolute_error_sum / len(intersection)
        avg_relative_error = relative_error_sum / len(intersection)
    else:
        avg_absolute_error = 0.0
        avg_relative_error = 0.0

    print("large={},sample={},phases={},use_add_knowl={},e1={},e2={},e3={},d={},min_freq={},origFIs={}".format(os.path.basename(orig_res_filename),
        os.path.basename(sample_res_filename), phases,
        use_additional_knowledge, epsilon_1, epsilon_2, epsilon_3, delta, min_freq,
        len(orig_res_set)))
    print("inter={},fn={},fp={},jaccard={}".format(len(intersection),
        false_negatives, false_positives, jaccard))
    print("posbor={},negbor={},emp_vc_dim={},not_emp_vc_dim={}".format(maximal_itemsets_size,
        negative_border_size, emp_vc_dim, not_emp_vc_dim))
    print("we={},maxabserr={},avgabserr={},avgrelerr={}".format(wrong_eps,
        max_absolute_error, avg_absolute_error, avg_relative_error))
    sys.stderr.write("{}\n".format(",".join((str(i) for i in (os.path.basename(orig_res_filename),
        os.path.basename(sample_res_filename), phases, use_additional_knowledge,
        epsilon_1, epsilon_2, epsilon_3, delta, min_freq,len(orig_res_set),
        len(intersection), false_negatives, false_positives, jaccard,
        maximal_itemsets_size, negative_border_size, emp_vc_dim,
        not_emp_vc_dim, wrong_eps, max_absolute_error, avg_absolute_error,
        avg_relative_error)))))

if __name__ == "__main__":
    main()

