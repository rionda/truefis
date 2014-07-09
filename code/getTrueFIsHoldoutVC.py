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

import locale, math, os.path, subprocess, sys, tempfile
import epsilon, utils


def get_trueFIs(exp_res_filename, eval_res_filename, min_freq, delta, gap=0.0, first_epsilon=1.0, use_additional_knowledge=False):
    """ Compute the True Frequent Itemsets using the 'holdout-VC' method.

    TODO Add more details."""

    stats = dict()

    with open(exp_res_filename) as FILE:
        size_line = FILE.readline()
        try:
            size_str = size_line.split("(")[1].split(")")[0]
        except IndexError:
            utils.error_exit("Cannot compute size of the explore dataset: '{}' is not in the recognized format\n".format(size_line))
        try:
            stats['exp_size'] = int(size_str)
        except ValueError:
            utils.error_exit("Cannot compute size of the explore dataset: {} is not a number\n".format(size_str))

    with open(eval_res_filename) as FILE:
        size_line = FILE.readline()
        try:
            size_str = size_line.split("(")[1].split(")")[0]
        except IndexError:
            utils.error_exit("Cannot compute size of the eval dataset: '{}' is not in the recognized format\n".format(size_line))
        try:
            stats['eval_size'] = int(size_str)
        except ValueError:
            utils.error_exit("Cannot compute size of the eval dataset: '{}' is not a number\n".format(size_str))

    stats['orig_size'] = stats['exp_size'] + stats['eval_size']

    exp_res = utils.create_results(exp_res_filename, min_freq)
    stats['exp_res'] = len(exp_res)
    exp_res_set = set(exp_res.keys())
    eval_res = utils.create_results(eval_res_filename, min_freq)
    stats['eval_res'] = len(eval_res)
    eval_res_set = set(eval_res.keys())
    intersection = exp_res_set & eval_res_set
    stats['holdout_intersection'] = len(intersection)
    stats['holdout_false_negatives'] = len(exp_res_set - eval_res_set)
    stats['holdout_false_positives'] = len(eval_res_set - exp_res_set)
    stats['holdout_jaccard'] = len(intersection) / len(exp_res_set | eval_res_set) 

    # One may want to play with giving different values for the different error
    # probabilities, but there isn't really much point in it.
    lower_delta = 1.0 - math.sqrt(1 - delta)

    stats['epsilon_1'] = first_epsilon

    sys.stderr.write("Computing candidates...")
    sys.stderr.flush()
    freq_bound = min_freq + stats['epsilon_1']
    candidates = []
    candidates_items = set()
    trueFIs = dict()
    for itemset in exp_res:
        if exp_res[itemset] < freq_bound:
            candidates.append(itemset)
            candidates_items |= itemset
        else:
            # Add itemsets with frequency at last freq_bound to the TFIs
            trueFIs[itemset] = exp_res[itemset]
    sys.stderr.write("done: {} candidates ({} items)\n".format(len(candidates),
        len(candidates_items)))
    sys.stderr.flush()

    if len(candidates) > 0 and use_additional_knowledge:
        sys.stderr.write("Using additional knowledge\n")
        candidates_items_sorted = sorted(candidates_items)
        candidates_items_in_sets_dict = dict()
        candidates_itemset_index = 0
        itemset_indexes_dict = dict()
        for first_itemset_index in range(len(candidates)):
            first_itemset = candidates[first_itemset_index]
            for item in first_itemset:
                if item in candidates_items_in_sets_dict:
                    candidates_items_in_sets_dict[item].append(candidates_itemset_index)
                else:
                    candidates_items_in_sets_dict[item] = [candidates_itemset_index]
            itemset_indexes_dict[first_itemset] = candidates_itemset_index
            candidates_itemset_index += 1

        # Compute an upper-bound to the VC-dimension of the set of candidates.
        constr_start_str = "cplex.SparsePair(ind = ["
        constr_end_str = "], val = vals)"
        vars_num = len(candidates) + len(candidates_items)
        constr_names = []

        (tmpfile_handle, tmpfile_name) = tempfile.mkstemp(prefix="cplx", dir=os.environ['PWD'], text=True)
        os.close(tmpfile_handle)
        with open(tmpfile_name, 'wt') as cplex_script:
            cplex_script.write("capacity = {}\n".format(len(candidates_items) - 1))
            cplex_script.write("import cplex, os, sys\n")
            cplex_script.write("from cplex.exceptions import CplexError\n")
            cplex_script.write("\n")
            cplex_script.write("\n")
            cplex_script.write("os.environ[\"ILOG_LICENSE_FILE\"] = \"/local/projects/cplex/ilm/site.access.ilm\"\n") 
            cplex_script.write("vals = [-1.0, 1.0]\n")
            cplex_script.write("sets_num = {}\n".format(len(candidates)))
            cplex_script.write("items_num = {}\n".format(len(candidates_items)))
            cplex_script.write("vars_num = {}\n".format(vars_num))
            cplex_script.write("my_ub = [1.0] * vars_num\n")
            cplex_script.write("my_types = \"\".join(\"I\" for i in range(vars_num))\n")
            cplex_script.write("my_obj = ([1.0] * sets_num) + ([0.0] * items_num)\n")
            cplex_script.write("my_colnames = [\"set{0}\".format(i) for i in range(sets_num)] + [\"item{0}\".format(j) for j in range(items_num)]\n")
            cplex_script.write("rows = [ ")

            sys.stderr.write("Writing knapsack constraints...")
            sys.stderr.flush()
            constr_num = 0
            for item_index in range(len(candidates_items)):
                try:
                    for itemset_index in candidates_items_in_sets_dict[candidates_items_sorted[item_index]]:
                        constr_str = "".join((constr_start_str,
                                "\"set{}\",\"item{}\"".format(itemset_index,item_index), constr_end_str))
                        cplex_script.write("{},".format(constr_str))
                        constr_num += 1
                        name = "s{}i{}".format(item_index, itemset_index)
                        constr_names.append(name)
                except KeyError:
                    sys.stderr.write("item_index={} candidates_items_sorted[item_index]={}\n".format(item_index,
                                candidates_items_sorted[item_index]))
                    in_candidates = False
                    candidates_itemset = frozenset()
                    for itemset in candidates:
                        if candidates_items_sorted[item_index] in itemset:
                            in_candidates = True
                            candidates_itemset = itemset
                            break
                    sys.stderr.write("{} in negative_border: {}. Itemset: {}\n".format(candidates_items_sorted[item_index], in_candidates, candidates_itemset))
                    sys.exit(1)

            # Create capacity constraints and write it to script
            constr_str = "".join((constr_start_str, ",".join("\"item{}\"".format(j) for 
                j in range(len(candidates_items))), "], val=[", ",".join("1.0" for j in range(len(candidates_items))), "])"))
            cplex_script.write(constr_str)
            cplex_script.write("]\n")
            cap_constr_name = "capacity"
            constr_names.append(cap_constr_name)
            sys.stderr.write("done\n")
            sys.stderr.flush()

            sys.stderr.write("vars_num={} candidates={} candidates_items_num={} constr_num={}\n".format(vars_num, len(candidates), len(candidates_items), constr_num))
            sys.stderr.flush()

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
            utils.error_exit("CPLEX exited with error code {}: {}\n".format(err.returncode, err.output))
        #finally:
        #    os.remove(tmpfile_name)

        cplex_output = cplex_output_binary_str.decode(locale.getpreferredencoding())
        cplex_output_lines = cplex_output.split("\n")
        cplex_solution_line = cplex_output_lines[-1 if len(cplex_output_lines[-1]) > 0 else -2]
        try:
            cplex_solution = eval(cplex_solution_line)
        except Exception:
            utils.error_exit("Error evaluating the CPLEX solution line: {}\n".format(cplex_solution_line))

        sys.stderr.write("cplex_solution={}\n".format(cplex_solution))
        sys.stderr.flush()
        #if cplex_solution[0] not in (1, 101, 102):
        #    utils.error_exit("CPLEX didn't find the optimal solution: {} {} {}\n".format(cplex_solution[0], cplex_solution[1], cplex_solution[2]))

        optimal_sol_upp_bound = int(math.floor(cplex_solution[2] * (1 + cplex_solution[3])))
        stats['vcdim'] = int(math.floor(math.log2(optimal_sol_upp_bound))) + 1
        if stats['vcdim'] > math.log2(len(candidates)):
            sys.stderr.write("Lowering VC-dimension to maximum value\n")
            sys.stderr.flush()
            stats['vcdim'] = int(math.floor(math.log2(len(candidates))))
        stats['epsilon_2'] = epsilon.get_eps_vc_dim(lower_delta,
                stats['orig_size'], stats['vcdim'])

    elif len(candidates) > 0 and not use_additional_knowledge:
        sys.stderr.write("Not using additional knowledge\n")
        sys.stderr.flush()
        stats['vcdim'] = int(math.floor(math.log2(len(candidates))))
        stats['epsilon_2'] = epsilon.get_eps_vc_dim(lower_delta,
                stats['orig_size'], stats['vcdim'])

    else:
        sys.stderr.write("There are no candidates\n")
        sys.stderr.flush()
        stats['vcdim'] = 0
        stats['epsilon_2'] = 0

    if len(candidates) > 0:
        sys.stderr.write("Computing the candidates that are TFIs...")
        sys.stderr.flush()
        freq_bound = min_freq + stats['epsilon_2']
        eval_res_itemsets = frozenset(eval_res.keys())
        for itemset in sorted(frozenset(candidates) & eval_res_itemsets,
                key=lambda x : eval_res[x], reverse=True) :
            if eval_res[itemset] >= freq_bound:
                trueFIs[itemset] = eval_res[itemset]
        sys.stderr.write("done\n")
        sys.stderr.flush()

    return (trueFIs, stats)


def main():
    # Verify arguments
    if len(sys.argv) != 7: 
        utils.error_exit("Usage: {} first_epsilon delta min_freq gap exploreres evalres\n".format(os.path.basename(sys.argv[0])))
    exp_res_filename = sys.argv[5]
    if not os.path.isfile(exp_res_filename):
        utils.error_exit("{} does not exist, or is not a file\n".format(exp_res_filename))
    eval_res_filename = sys.argv[6]
    if not os.path.isfile(eval_res_filename):
        utils.error_exit("{} does not exist, or is not a file\n".format(eval_res_filename))
    try:
        first_epsilon = float(sys.argv[1])
    except ValueError:
        utils.error_exit("{} is not a number\n".format(sys.argv[1]))
    try:
        delta = float(sys.argv[2])
    except ValueError:
        utils.error_exit("{} is not a number\n".format(sys.argv[2]))
    try:
        min_freq = float(sys.argv[3])
    except ValueError:
        utils.error_exit("{} is not a number\n".format(sys.argv[3]))
    try:
        gap = float(sys.argv[4])
    except ValueError:
        utils.error_exit("{} is not a number\n".format(sys.argv[4]))

    (trueFIs, stats) = get_trueFIs(exp_res_filename, eval_res_filename, min_freq,
            delta, gap, first_epsilon)

    utils.print_itemsets(trueFIs, stats['orig_size'])

    sys.stderr.write("exp_res_file={},eval_res_file={},d={},min_freq={},trueFIs={}\n".format(os.path.basename(exp_res_filename),os.path.basename(eval_res_filename), delta, min_freq, len(trueFIs)))
    sys.stderr.write("orig_size={},exp_size={},eval_size={}\n".format(stats['orig_size'],
        stats['exp_size'], stats['eval_size']))
    sys.stderr.write("exp_res={},eval_res={}\n".format(stats['exp_res'], stats['eval_res']))
    sys.stderr.write("holdout_intersection={},holdout_false_positives={},holdout_false_negatives={},holdout_jaccard={}\n".format(stats['holdout_intersection'],
        stats['holdout_false_positives'], stats['holdout_false_negatives'], stats['holdout_jaccard']))
    sys.stderr.write("e1={},e2={},vcdim={}\n".format(stats['epsilon_1'],
        stats['epsilon_2'], stats['vcdim']))
    sys.stderr.write("exp_res_file,eval_res_file,delta,min_freq,trueFIs,orig_size,exp_size,eval_size,exp_res,eval_res,holdout_intersection,holdout_false_positives,holdout_false_negatives,holdout_jaccard,e1,e2,vcdim\n")
    sys.stderr.write("{}\n".format(",".join((str(i) for i in
        (os.path.basename(exp_res_filename),
        os.path.basename(eval_res_filename), delta, min_freq,len(trueFIs),
        stats['orig_size'], stats['exp_size'], stats['eval_size'],
        stats['exp_res'], stats['eval_res'], stats['holdout_intersection'],
        stats['holdout_false_positives'], stats['holdout_false_negatives'],
        stats['holdout_jaccard'], stats['epsilon_1'], stats['epsilon_2'],
        stats['vcdim'])))))


if __name__ == "__main__":
    main()

