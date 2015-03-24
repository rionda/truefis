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


def get_trueFIs(exp_res_filename, eval_res_filename, min_freq, delta, pvalue_mode, first_epsilon=1.0):
    """ Compute the True Frequent Itemsets using the 'holdout-VC' method with
    the binomial test

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
    stats['lowered_delta'] = 1.0 - math.sqrt(1 - delta)

    stats['filter_epsilon'] = first_epsilon

    sys.stderr.write("Computing candidates...")
    sys.stderr.flush()
    freq_bound = min_freq + stats['filter_epsilon']
    exp_res_filtered = set()
    exp_res_filtered_items = set()
    trueFIs = dict()
    for itemset in exp_res:
        if exp_res[itemset] < freq_bound:
            exp_res_filtered.add(itemset)
            exp_res_filtered_items |= itemset
        else:
            # Add itemsets with frequency at last freq_bound to the TFIs
            trueFIs[itemset] = exp_res[itemset]
    sys.stderr.write("done: {} exp_res_filtered ({} items)\n".format(len(exp_res_filtered),
        len(exp_res_filtered_items)))
    sys.stderr.flush()
    stats['tfis_from_exp'] = len(trueFIs)
    stats['exp_res_filtered'] = len(exp_res_filtered)

    supposed_freq = (math.ceil( stats['orig_size'] * min_freq) - 1) / stats['orig_size']
    if stats['exp_res_filtered'] > 0:
        eval_res = utils.create_results(eval_res_filename, min_freq)
        eval_res_set = set(eval_res.keys())
        stats['eval_res'] = len(eval_res)

        intersection = exp_res_filtered & eval_res_set
        stats['holdout_intersection'] = len(intersection)
        stats['holdout_false_negatives'] = len(exp_res_filtered - eval_res_set)

        # Bonferroni correction (Union bound). We work in the log space.
        stats['critical_value'] = math.log(stats['lowered_delta']) - math.log(stats['exp_res_filtered'])

        # Add TFIs from eval
        last_accepted_freq = 1.0
        last_non_accepted_freq = min_freq
        for itemset in sorted(intersection, key=lambda x : eval_res[x], reverse=True):
            p_value = utils.pvalue(pvalue_mode, eval_res[itemset],
                    stats['eval_size'], supposed_freq)
            if p_value <= stats['critical_value']:
                trueFIs[itemset] = eval_res[itemset]
                last_accepted_freq = eval_res[itemset]
            else:
                last_non_accepted_freq = eval_res[itemset]
                break

        # Compute epsilon for the binomial
        min_diff = 5e-6 # controls when to stop the binary search
        while last_accepted_freq - last_non_accepted_freq > min_diff:
            mid_point = (last_accepted_freq - last_non_accepted_freq) / 2
            test_freq = last_non_accepted_freq + mid_point
            p_value = utils.pvalue(pvalue_mode, test_freq,
                    stats['eval_size'], supposed_freq)
            if p_value <= stats['critical_value']:
                last_accepted_freq = test_freq
            else:
                last_non_accepted_freq = test_freq

        stats['epsilon'] = last_non_accepted_freq + ((last_accepted_freq -
            last_non_accepted_freq) / 2) - min_freq
        stats['removed'] = len(intersection) - len(trueFIs)
    else: # stats['exp_res_filtered'] == 0
        stats['eval_res'] = 0
        stats['holdout_false_negatives'] = 0
        stats['holdout_intersection'] = 0
        stats['critical_value'] = 0
        stats['epsilon'] = 0
        stats['removed'] = 0

    return (trueFIs, stats)


def main():
    # Verify arguments
    if len(sys.argv) != 7: 
        utils.error_exit("Usage: {} first_epsilon delta min_freq pvalue_mode exploreres evalres\n".format(os.path.basename(sys.argv[0])))
    exp_res_filename = sys.argv[5]
    if not os.path.isfile(exp_res_filename):
        utils.error_exit("{} does not exist, or is not a file\n".format(exp_res_filename))
    eval_res_filename = sys.argv[6]
    if not os.path.isfile(eval_res_filename):
        utils.error_exit("{} does not exist, or is not a file\n".format(eval_res_filename))
    pvalue_mode = sys.argv[4].upper()
    if pvalue_mode != "C" and pvalue_mode != "E" and pvalue_mode != "W":
        utils.error_exit("p-value mode must be 'c', 'e', or 'w'. You passed {}\n".format(pvalue_mode))
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

    (trueFIs, stats) = get_trueFIs(exp_res_filename, eval_res_filename, min_freq,
            delta, pvalue_mode, first_epsilon)

    utils.print_itemsets(trueFIs, stats['orig_size'])

    sys.stderr.write("exp_res_file={},eval_res_file={},pvalue_mode={},d={},min_freq={},trueFIs={}\n".format(os.path.basename(exp_res_filename),os.path.basename(eval_res_filename), pvalue_mode, delta, min_freq, len(trueFIs)))
    sys.stderr.write("orig_size={},exp_size={},eval_size={}\n".format(stats['orig_size'],
        stats['exp_size'], stats['eval_size']))
    sys.stderr.write("exp_res={},exp_res_filtered={},eval_res={}\n".format(stats['exp_res'],
        stats['exp_res_filtered'], stats['eval_res']))
    sys.stderr.write("filter_epsilon={},tfis_from_exp={}\n".format(stats['filter_epsilon'], stats['tfis_from_exp']))
    sys.stderr.write("holdout_intersection={},holdout_false_negatives={}\n".format(stats['holdout_intersection'],
        stats['holdout_false_negatives']))
    sys.stderr.write("critical_value={},removed={},epsilon={}\n".format(stats['critical_value'],
        stats['removed'], stats['epsilon']))
    sys.stderr.write("exp_res_file,eval_res_file,pvalue_mode,delta,min_freq,trueFIs,orig_size,exp_size,eval_size,exp_res,exp_res_filtered,eval_res,filter_epsilon,tfis_from_exp,holdout_intersection,holdout_false_negatives,critical_value,removed,epsilon\n")
    sys.stderr.write("{}\n".format(",".join((str(i) for i in
        (os.path.basename(exp_res_filename), os.path.basename(eval_res_filename),
        pvalue_mode, delta, min_freq,len(trueFIs),
        stats['orig_size'], stats['exp_size'], stats['eval_size'],
        stats['exp_res'], stats['exp_res_filtered'], stats['eval_res'],
        stats['filter_epsilon'],
        stats['tfis_from_exp'], stats['holdout_intersection'],
        stats['holdout_false_negatives'], stats['critical_value'],
        stats['removed'], stats['epsilon'])))))

if __name__ == "__main__":
    main()

