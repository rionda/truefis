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

import os.path, sys
import utils


def compare(orig_res, other_res, epsilon=1.0):
    """Compare two sets of FIs and return statistics about them.

    'orig_res' and 'other_res' are dict whose keys are itemsets (frozensets)
    and values are frequencies, like those returned by utils.create_results().
    
    Returns a dict with the following keys (and meanings):
        intersection: size of the intersection
        false_negatives: size of orig_res \setminus other_res
        false_positives: size of other_res \setminus orig_res 
        false_positives_set: other_res \setminus orig_res
        jaccard: jaccard index (size of intersection over size of union)
        max_absolute_error: maximum absolute difference in frequency
        avg_absolute_error: average absolute difference in frequency
        avg_relative_error: average relative difference in frequency
        wrong_eps: number of itemsets for which the absolute difference in
        frequency is greater than epsilon 
    """
    stats = dict()
    orig_res_set = set(orig_res.keys())
    other_res_set = set(other_res.keys())

    intersection = orig_res_set & other_res_set
    stats['intersection'] = len(intersection)

    stats['false_negatives'] = len(orig_res_set - other_res_set)

    stats['false_positives_set'] = other_res_set - orig_res_set
    stats['false_positives'] = len(stats['false_positives_set'])
    if stats['false_positives'] > 0:
        for itemset in stats['false_positives_set']:
            sys.stderr.write("WARNING! FALSE POSITIVE: '{}', freq={}\n".format(" ".join(str(item) for item in itemset), other_res[itemset]))

    stats['jaccard'] = len(intersection) / len(orig_res_set | other_res_set) 

    stats['max_absolute_error'] = 0.0
    absolute_error_sum = 0.0
    relative_error_sum = 0.0
    stats['wrong_eps'] = 0
    for itemset in intersection:
        absolute_error = abs(other_res[itemset] - orig_res[itemset])
        absolute_error_sum += absolute_error
        if absolute_error > stats['max_absolute_error']:
            stats['max_absolute_error'] = absolute_error
        if absolute_error > epsilon:
            stats['wrong_eps'] += 1
        relative_error_sum += absolute_error / orig_res[itemset]

    if stats['intersection'] > 0:
        stats['avg_absolute_error'] = absolute_error_sum / stats['intersection']
        stats['avg_relative_error'] = relative_error_sum / stats['intersection']
    else:
        stats['avg_absolute_error'] = 0.0
        stats['avg_relative_error'] = 0.0

    return stats


def main():
    if len(sys.argv) != 5:
        utils.error_exit("USAGE: {} min_freq epsilon sampleRes origRes\n".format(sys.argv[0]))
    orig_res_filename = os.path.expanduser(sys.argv[4])
    if not os.path.isfile(orig_res_filename):
        utils.error_exit("{} does not exist, or is not a file\n".format(orig_res_filename))
    sample_res_filename = os.path.expanduser(sys.argv[3])
    if not os.path.isfile(sample_res_filename):
        utils.error_exit("{} does not exist, or is not a file\n".format(sample_res_filename))
    try:
        min_freq = float(sys.argv[1])
    except ValueError:
        utils.error_exit("{} is not a number\n".format(sys.argv[1]))
    try:
        epsilon = float(sys.argv[2])
    except ValueError:
        utils.error_exit("{} is not a number\n".format(sys.argv[2]))

    origFIs = utils.create_results(orig_res_filename, min_freq)
    sampleFIs = utils.create_results(sample_res_filename, min_freq)

    stats = compare(origFIs, sampleFIs, epsilon)

    print("large={},sample={},min_freq={},epsilon={},origFIs={}".format(os.path.basename(orig_res_filename),
        os.path.basename(sample_res_filename), min_freq, epsilon, len(origFIs)))
    print("inter={},fn={},fp={},jaccard={}".format(stats['intersection'],
        stats['false_negatives'], stats['false_positives'], stats['jaccard']))
    print("we={},maxabserr={},avgabserr={},avgrelerr={}".format(stats['wrong_eps'],
        stats['max_absolute_error'], stats['avg_absolute_error'], stats['avg_relative_error']))
    sys.stderr.write("orig_res,sample_res,min_freq,epsilon,origFIs,intersect,false_neg,false_pos,jaccard,wrong_eps,max_abs_err,avg_abs_err,avg_rel_err\n")
    sys.stderr.write("{}\n".format(",".join((str(i) for i in (os.path.basename(orig_res_filename),
        os.path.basename(sample_res_filename), min_freq, epsilon, len(origFIs),
        stats['intersection'], stats['false_negatives'],
        stats['false_positives'], stats['jaccard'], stats['wrong_eps'],
        stats['max_absolute_error'], stats['avg_absolute_error'],
        stats['avg_relative_error'])))))


if __name__ == "__main__":
    main()

