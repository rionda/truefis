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
import utils


def get_trueFIs(exp_res_filename, eval_res_filename, min_freq, delta, pvalue_mode, do_filter=False):
    """ Compute the True Frequent Itemsets using the holdout method.
    
    The holdout method is described in Geoffrey I. Webb, "Discovering
    significant patterns" in Machine Learning, Vol. 68, Issue (1), pp. 1-3,
    2007.

    The dataset is split in two parts, an exploratory part and an evaluation
    part. Each are mined separately at frequency 'min_freq'. The results are
    contained in 'exp_res_filename' and 'eval_res_filename' respectively.
    The parameter 'do_filter' controls a variant of the algorithm where the
    results from the exploratory part are filtered more.
    
    The p-values for the Binomial tests are computed using the mode specified
    by pvalue_mode, eiter 'c' for Chernoff or 'e' for exact. The parameter
    'use_additional_knowledge' can be used to incorporate additional knowledge
    about the data generation process.
    
    Returns a pair (trueFIs, stats). 
    'trueFIs' is a dict whose keys are itemsets (frozensets) and values are
    frequencies. This collection of itemsets contains only TFIs with
    probability at least 1 - delta.
    'stats' is a dict containing various statistics used in computing the
    collection of itemsets."""

    stats = dict()

    with open(exp_res_filename) as FILE:
        exp_size_line = FILE.readline()
        try:
            size_str = exp_size_line.split("(")[1].split(")")
            exp_size = int(size_str)
        except ValueError:
            utils.error_exit("Cannot compute size of the explore dataset: {} is not a number\n".format(size_str))

    with open(eval_res_filename) as FILE:
        eval_size_line = FILE.readline()
        try:
            size_str = eval_size_line.split("(")[1].split(")")
            stats['eval_size'] = int(size_str)
        except ValueError:
            utils.error_exit("Cannot compute size of the eval dataset: {} is not a number\n".format(size_str))

    stats['orig_size'] = stats['exp_size'] + stats['eval_size']

    exp_res = utils.create_results(exp_res_filename, min_freq)
    stats['explorer_res'] = len(exp_res)

    supposed_freq = (math.ceil( stats['orig_size'] * min_freq) - 1) / stats['orig_size']
    if do_filter:
        exp_res_filtered = dict()
        for itemset in exp_res:
            if utils.pvalue(pvalue_mode, exp_res[itemset], exp_size, supposed_freq) <= delta:
                exp_res_filtered[itemset] = exp_res[itemset]
    else:
        exp_res_filtered = exp_res
    exp_res_filtered_set = set(exp_res_filtered.keys())
    stats['exp_res_filtered'] = len(exp_res_filtered_set)

    eval_res = utils.create_results(eval_res_filename, min_freq)
    eval_res_set = set(eval_res.keys())
    stats['eval_res'] = len(eval_res)

    intersection = exp_res_filtered_set & eval_res_set
    stats['holdout_intersection'] = len(intersection)
    stats['holdout_false_negatives'] = len(exp_res_filtered_set - eval_res_set)
    stats['holdout_false_positives'] = len(eval_res_set - exp_res_filtered_set)
    stats['holdout_jaccard'] = len(intersection) / len(exp_res_filtered_set | eval_res_set) 

    # Bonferroni correction (Union bound). We work in the log space.
    stats['critical_value'] = math.log(delta) - math.log(len(exp_res_filtered_set))

    trueFIs = dict()
    stats['removed'] = 0
    for itemset in sorted(intersection, key=lambda x : eval_res[x], reverse=True):
        p_value = utils.pvalue(pvalue_mode, eval_res[itemset],
                stats['eval_size'], supposed_freq)
        if p_value <= stats['critical_value']:
            trueFIs[itemset] = eval_res[itemset]
        else:
            stats['removed'] = len(intersection) - len(trueFIs)
            break

    return (trueFIs, stats)


def main():
    # Verify arguments
    if len(sys.argv) != 7: 
        utils.error_exit("Usage: {} do_filter={{0|1}} delta min_freq pvalue_mode={{e|c}} exploreres evalres\n".format(os.path.basename(sys.argv[0])))
    exp_res_filename = sys.argv[5]
    if not os.path.isfile(exp_res_filename):
        utils.error_exit("{} does not exist, or is not a file\n".format(exp_res_filename))
    eval_res_filename = sys.argv[6]
    if not os.path.isfile(eval_res_filename):
        utils.error_exit("{} does not exist, or is not a file\n".format(eval_res_filename))
    pvalue_mode = sys.argv[4].upper()
    if pvalue_mode != "C" and pvalue_mode != "E":
        utils.error_exit("p-value mode must be either 'c' or 'e'. You passed {}\n".format(pvalue_mode))
    try:
        do_filter = int(sys.argv[1])
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

    (trueFIs, stats) = get_trueFIs(exp_res_filename, eval_res_filename, delta, min_freq, pvalue_mode, do_filter)

    utils.print_itemsets(trueFIs, stats['orig_size'])

    sys.stderr.write("exp_res_file={},eval_res_file={},do_filter={},pvalue_mode={},d={},min_freq={},trueFIs={}\n".format(os.path.basename(exp_res_filename),os.path.basename(eval_res_filename), do_filter, pvalue_mode, delta, min_freq, len(trueFIs)))
    sys.stderr.write("orig_size={},exp_size={},eval_size={}\n".format(stats['orig_size'],
        stats['exp_size'], stats['eval_size']))
    sys.stderr.write("exp_res={},exp_res_filtered={},eval_res={}\n".format(stats['exp_res'],
        stats['exp_res_filtered'], stats['eval_res']))
    sys.stderr.write("holdout_intersection={},holdout_false_positives={},holdout_false_negatives={},holdout_jaccard={}\n".format(stats['holdout_intersection'],
        stats['holdout_false_positives'], stats['holdout_false_negatives'], stats['holdout_jaccard']))
    sys.stderr.write("critical_value={},removed={}\n".format(stats['critical_value'],stats['removed']))
    sys.stderr.write("exp_res_file,eval_res_file,do_filter,pvalue_mode,delta,min_freq,trueFIs,orig_size,exp_size,eval_size,exp_res,exp_res_filtered,eval_res,holdout_intersection,holdout_false_positives,holdout_false_negatives,holdout_jaccard,critical_value,removed\n")
    sys.stderr.write("{}\n".format(",".join((str(i) for i in
        (os.path.basename(exp_res_filename), os.path.basename(eval_res_filename),
        do_filter, pvalue_mode, delta, min_freq,len(trueFIs),
        stats['orig_size'], stats['exp_size'], stats['eval_size'],
        stats['exp_res'], stats['exp_res_filtered'], stats['eval_res'],
        stats['holdout_intersection'], stats['holdout_false_positives'],
        stats['holdout_false_negatives'], stats['holdout_jaccard'],
        stats['critical_value'],stats['removed'])))))


if __name__ == "__main__":
    main()

