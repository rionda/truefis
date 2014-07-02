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
from datasetsinfo import ds_stats


def get_trueFIs(dataset_name, res_filename, min_freq, delta, pvalue_mode, use_additional_knowledge=False):
    """ Compute the True Frequent Itemsets.
    
    Returns a pair (trueFIs, stats) where trueFIs is a dict whose keys are
    itemsets (frozensets) and values are frequencies. 'stats' is also a dict
    with the following keys (and meanings): FIXME."""

    stats = dict()

    if use_additional_knowledge:
        stats['union_bound_factor'] = utils.get_union_bound_factor(ds_stats[dataset_name]['numitems'], 2 * \
                ds_stats[dataset_name]['maxlen']) 
    else:
        stats['union_bound_factor'] = ds_stats[dataset_name]['numitems']

    stats['critical_value'] = math.log(delta) - (stats['union_bound_factor'] / math.log2(math.e))

    supposed_freq = (math.ceil(ds_stats[dataset_name]['size'] * min_freq) - 1) / ds_stats[dataset_name]['size']

    sample_res = utils.create_results(res_filename, min_freq)

    survivors = dict()
    stats['removed'] = 0
    for itemset in sample_res.keys():
        p_value = utils.pvalue(pvalue_mode, sample_res[itemset],
                ds_stats[dataset_name]['size'], supposed_freq)
        if p_value <= stats['critical_value']:
            survivors[itemset] = sample_res[itemset]
        else:
            stats['removed'] +=1

    return (survivors, stats)


def main():
    # Verify arguments
    if len(sys.argv) != 7: 
        utils.error_exit("Usage: {} use_additional_knowledge={{0|1}} delta min_freq mode={{c|e}} dataset results_filename\n".format(os.path.basename(sys.argv[0])))
    dataset_name = sys.argv[5]
    res_filename = sys.argv[6]
    if not os.path.isfile(res_filename):
        utils.error_exit("{} does not exist, or is not a file\n".format(res_filename))
    pvalue_mode = sys.argv[4].upper()
    if pvalue_mode != "C" and pvalue_mode != "E":
        utils.error_exit("p-value mode must be either 'c' or 'e'. You passed {}\n".format(pvalue_mode))
    try:
        use_additional_knowledge = int(sys.argv[1])
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

    (trueFIs, stats) = get_trueFIs(dataset_name, res_filename, min_freq, delta,
            pvalue_mode, use_additional_knowledge)

    utils.print_itemsets(trueFIs, ds_stats[dataset_name]['size'])

    sys.stderr.write("res_file={},use_add_knowl={},pvalue_mode={},d={},min_freq={},trueFIs={}\n".format(os.path.basename(res_filename),
        use_additional_knowledge, pvalue_mode, delta, min_freq, len(trueFIs)))
    sys.stderr.write("union_bound_factor={},critical_value={},removed={}\n".format(stats['union_bound_factor'],stats['critical_value'],stats['removed']))
    sys.stderr.write("res_file,add_knowl,pvalue_mode,delta,min_freq,trueFIs,union_bound_factor,critical_value,removed\n")
    sys.stderr.write("{}\n".format(",".join((str(i) for i in (os.path.basename(res_filename),
        use_additional_knowledge, pvalue_mode, delta, min_freq,len(trueFIs),
        stats['union_bound_factor'],stats['critical_value'],stats['removed'])))))


if __name__ == "__main__":
    main()

