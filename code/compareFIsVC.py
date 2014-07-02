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

import os, os.path, sys
import compareFIs, getTrueFIsVC, utils


def main():
    # Verify arguments
    if len(sys.argv) != 8: 
        utils.error_exit("Usage: {} ADDITIONALKNOWLEDGE={{0|1}} DELTA MINFREQ GAP DATASETNAME ORIGRES SAMPLERES\n".format(os.path.basename(sys.argv[0])))
    dataset_name = sys.argv[5]
    orig_res_filename = os.path.expanduser(sys.argv[6])
    if not os.path.isfile(orig_res_filename):
        utils.error_exit("{} does not exist, or is not a file\n".format(orig_res_filename))
    sample_res_filename = os.path.expanduser(sys.argv[7])
    if not os.path.isfile(sample_res_filename):
        utils.error_exit("{} does not exist, or is not a file\n".format(sample_res_filename))
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
    try:
        gap = float(sys.argv[4])
    except ValueError:
        utils.error_exit("{} is not a number\n".format(sys.argv[4]))

    # Get the TFIs
    (sample_res, tfi_stats) = getTrueFIsVC.get_trueFIs(dataset_name, sample_res_filename, min_freq, delta, gap, use_additional_knowledge)

    # Do comparison between the original set of TFIs and the one extracted
    # from the sample.
    orig_res = utils.create_results(orig_res_filename, min_freq)
    comp_stats = compareFIs.compare(orig_res, sample_res, tfi_stats['epsilon_2'])

    print("large={},sample={},use_add_knowl={},e1={},e2={},d={},min_freq={},origFIs={}".format(os.path.basename(orig_res_filename),
        os.path.basename(sample_res_filename), use_additional_knowledge,
        tfi_stats['epsilon_1'], tfi_stats['epsilon_2'], delta, min_freq,
        len(orig_res)))
    print("inter={},fn={},fp={},jaccard={}".format(comp_stats['intersection'],
        comp_stats['false_negatives'], comp_stats['false_positives'],
        comp_stats['jaccard']))
    print("posbor={},negbor={},emp_vc_dim={},not_emp_vc_dim={}".format(tfi_stats['maximal_itemsets'],
        tfi_stats['negative_border'], tfi_stats['emp_vc_dim'],
        tfi_stats['not_emp_vc_dim']))
    print("we={},maxabserr={},avgabserr={},avgrelerr={}".format(comp_stats['wrong_eps'],
        comp_stats['max_absolute_error'], comp_stats['avg_absolute_error'],
        comp_stats['avg_relative_error']))
    sys.stderr.write("orig_res,sample_res,add_knowl,e_1,e_2,delta,min_freq,origFIs,intersect,false_neg,false_pos,jaccard,maximal_itemsets,negative_border,emp_vc_dim,not_emp_vc_dim,wrong_eps,max_abs_err,avg_abs_err,avg_rel_err\n")
    sys.stderr.write("{}\n".format(",".join((str(i) for i in (os.path.basename(orig_res_filename),
        os.path.basename(sample_res_filename), use_additional_knowledge,
        tfi_stats['epsilon_1'], tfi_stats['epsilon_2'], delta,
        min_freq,len(orig_res), comp_stats['intersection'],
        comp_stats['false_negatives'], comp_stats['false_positives'],
        comp_stats['jaccard'], tfi_stats['maximal_itemsets'],
        tfi_stats['negative_border'], tfi_stats['emp_vc_dim'],
        tfi_stats['not_emp_vc_dim'], comp_stats['wrong_eps'],
        comp_stats['max_absolute_error'], comp_stats['avg_absolute_error'],
        comp_stats['avg_relative_error'])))))


if __name__ == "__main__":
    main()

