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

import math, os.path, sys
import util


def get_trueFIs(explore_res_filename, eval_res_filename, min_freq, delta, do_filter=False):
    """ Compute the True Frequent Itemsets.
    
    Returns a pair (trueFIs, stats) where trueFIs is a dict whose keys are
    itemsets (frozensets) and values are frequencies. 'stats' is also a dict
    with the following keys (and meanings): FIXME."""

    stats = dict()

    # We use the following method to compute the size as the exploration and
    # the evaluation datasets may not be available in datasetsinfo.py
    with open(explore_res_filename) as FILE:
        exp_size_line = FILE.readline()
        try:
            size_str = eval_size_line.split("(")[1].split(")")
            exp_size = int(size_str)
        except ValueError:
            util.error_exit("Cannot compute size of the explore dataset: {} is not a number\n".format(size_str))

    with open(eval_res_filename) as FILE:
        eval_size_line = FILE.readline()
        try:
            size_str = eval_size_line.split("(")[1].split(")")
            stats['eval_size'] = int(size_str)
        except ValueError:
            util.error_exit("Cannot compute size of the eval dataset: {} is not a number\n".format(size_str))

    stats['orig_size'] = stats['exp_size'] + stats['eval_size']

    supposed_freq = (math.ceil( stats['orig_size'] * min_freq) - 1) / stats['orig_size']

    explore_res = util.create_results(explore_res_filename, min_freq)
    stats['explorer_res'] = len(explore_res)

    if do_filter:
        explore_res_filtered = dict()
        for itemset in explore_res:
            if util.pvalue(explore_res[itemset], exp_size, supposed_freq) <= delta:
                explore_res_filtered[itemset] = explore_res[itemset]
    else:
        explore_res_filtered = explore_res
    explore_res_filtered_set = set(explore_res_filtered.keys())
    stats['explore_res_filtered'] = len(explore_res_filtered_set)

    eval_res = util.create_results(eval_res_filename, min_freq)
    eval_res_set = set(eval_res.keys())
    stats['eval_res'] = len(eval_res)

    intersection = explore_res_filtered_set & eval_res_set
    stats['intersection'] = len(intersection)
    stats['false_negatives'] = len(explore_res_filtered_set - eval_res_set)
    stats['false_positives'] = len(eval_res_set - explore_res_filtered_set)
    stats['jaccard'] = len(intersection) / len(explore_res_filtered_set | eval_res_set) 

    stats['critical_value'] = math.log(delta) - math.log(len(explore_res_filtered_set))

    trueFIs = dict()
    stats['removed'] = 0
    for itemset in intersection:
        p_value = util.pvalue(pvalue_mode, eval_res[itemset], eval_size, supposed_freq)
        if p_value <= stats['critical_value']:
            trueFIs[itemset] = eval_res[itemset]
        else:
            stats['removed'] +=1

    return (trueFIs, stats)


def main():
    # Verify arguments
    if len(sys.argv) != 7: 
        util.error_exit("Usage: {} do_filter={{0|1}} delta min_freq pvalue_mode={{e|c}} exploreres evalres\n".format(os.path.basename(sys.argv[0])))
    explore_res_filename = sys.argv[5]
    if not os.path.isfile(explore_res_filename):
        util.error_exit("{} does not exist, or is not a file\n".format(explore_res_filename))
    eval_res_filename = sys.argv[6]
    if not os.path.isfile(eval_res_filename):
        util.error_exit("{} does not exist, or is not a file\n".format(eval_res_filename))
    pvalue_mode = sys.argv[4].upper()
    if pvalue_mode != "C" and pvalue_mode != "E":
        util.error_exit("p-value mode must be either 'c' or 'e'. You passed {}\n".format(pvalue_mode))
    try:
        do_filter = int(sys.argv[1])
    except ValueError:
        util.error_exit("{} is not a number\n".format(sys.argv[1]))
    try:
        delta = float(sys.argv[2])
    except ValueError:
        util.error_exit("{} is not a number\n".format(sys.argv[2]))
    try:
        min_freq = float(sys.argv[3])
    except ValueError:
        util.error_exit("{} is not a number\n".format(sys.argv[3]))

    (trueFIs, stats) = get_trueFIs(explore_res_filename, eval_res_filename, delta, min_freq, pvalue_mode, do_filter)

    util.print_itemsets(trueFIs, stats['orig_size'])

    # TODO print more stats
    sys.stderr.write("explore_res_file={},eval_res_file={},do_filter={},pvalue_mode={},d={},min_freq={},trueFIs={}\n".format(os.path.basename(explore_res_filename),os.path.basename(eval_res_filename), do_filter, pvalue_mode, delta, min_freq, len(trueFIs)))
    sys.stderr.write("critical_value={},removed={}\n".format(stats['critical_value'],stats['removed']))
    sys.stderr.write("explore_res_file,eval_res_file,do_filter,pvalue_mode,delta,min_freq,trueFIs,critical_value,removed\n")
    sys.stderr.write("{}\n".format(",".join((str(i) for i in
        (os.path.basename(explore_res_filename), os.path.basename(eval_res_filename),
        do_filter, pvalue_mode, delta, min_freq,len(trueFIs),
        stats['critical_value'],stats['removed'])))))


if __name__ == "__main__":
    main()

