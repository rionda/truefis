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
import getDatasetInfo, utils


def get_trueFIs(ds_stats, res_filename, min_freq, delta, pvalue_mode, use_additional_knowledge=False):
    """ Compute the True Frequent Itemsets using the Binomial test with a
    Bonferroni correction.

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

    sample_res = utils.create_results(res_filename, min_freq)

    # We work in the log-space
    if use_additional_knowledge:
        stats['union_bound_factor'] = utils.get_union_bound_factor(ds_stats['numitems'], 2 * \
                ds_stats['maxlen'])
    else:
        stats['union_bound_factor'] = ds_stats['numitems'] * math.log(2.0)

    # Bonferroni correction (Union bound)
    stats['critical_value'] = math.log(delta) - stats['union_bound_factor']
    supposed_freq = (math.ceil(ds_stats['size'] * min_freq) - 1) / ds_stats['size']
    trueFIs = dict()
    last_accepted_freq = 1.0
    last_non_accepted_freq = min_freq
    for itemset in sorted(sample_res.keys(),key=lambda x : sample_res[x], reverse=True):
        p_value = utils.pvalue(pvalue_mode, sample_res[itemset],
                ds_stats['size'], supposed_freq)
        if p_value <= stats['critical_value']:
            trueFIs[itemset] = sample_res[itemset]
            last_accepted_freq = sample_res[itemset]
        else:
            # Compute epsilon for the binomial
            last_non_accepted_freq = sample_res[itemset]
            break

    min_diff = 1e-5 # controls when to stop the binary search
    while last_accepted_freq - last_non_accepted_freq > min_diff:
        mid_point = (last_accepted_freq - last_non_accepted_freq) / 2
        test_freq = last_non_accepted_freq + mid_point
        p_value = utils.pvalue(pvalue_mode, test_freq,
                ds_stats['size'], supposed_freq)
        if p_value <= stats['critical_value']:
            last_accepted_freq = test_freq
        else:
            last_non_accepted_freq = test_freq

    stats['epsilon'] = last_non_accepted_freq + ((last_accepted_freq -
        last_non_accepted_freq) / 2) - min_freq
    stats['removed'] = len(sample_res) - len(trueFIs)

    return (trueFIs, stats)


def main():
    # Verify arguments
    if len(sys.argv) != 7: 
        utils.error_exit("Usage: {} use_additional_knowledge={{0|1}} delta min_freq mode={{c|e}} dataset results_filename\n".format(os.path.basename(sys.argv[0])))
    dataset = sys.argv[5]
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

    ds_stats = getDatasetInfo.get_ds_stats(dataset)

    (trueFIs, stats) = get_trueFIs(ds_stats, res_filename, min_freq, delta,
            pvalue_mode, use_additional_knowledge)

    utils.print_itemsets(trueFIs, ds_stats['size'])

    sys.stderr.write("res_file={},use_add_knowl={},pvalue_mode={},d={},min_freq={},trueFIs={}\n".format(os.path.basename(res_filename),
        use_additional_knowledge, pvalue_mode, delta, min_freq, len(trueFIs)))
    sys.stderr.write("union_bound_factor={},critical_value={},removed={},epsilon={}\n".format(stats['union_bound_factor'],stats['critical_value'],stats['removed'],stats['epsilon']))
    sys.stderr.write("res_file,add_knowl,pvalue_mode,delta,min_freq,trueFIs,union_bound_factor,critical_value,removed,epsilon\n")
    sys.stderr.write("{}\n".format(",".join((str(i) for i in (os.path.basename(res_filename),
        use_additional_knowledge, pvalue_mode, delta, min_freq,len(trueFIs),
        stats['union_bound_factor'], stats['critical_value'], stats['removed'],
        stats['epsilon'])))))


if __name__ == "__main__":
    main()

