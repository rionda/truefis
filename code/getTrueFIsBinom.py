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

import math, sys
from operator import itemgetter
from os.path import basename, isfile
from scipy.stats import binom
from scipy.misc import logsumexp 
from datasetsinfo import ds_stats
import my_utils as my

def log_factorial(m,n):
    """ Compute the logarithm of m * (m+1) * ... * n """
    sum = 0
    for i in range(m,n+1): sum += math.log(i)
    return sum


def log_binomial(n,k): 
    """ Compute the logarithm of n choose k """
    if k > (n-k):
        return (log_factorial(n-k+1,n)-log_factorial(2,k))
    else:
        return (log_factorial(k+1,n)-log_factorial(2,n-k))


def get_union_bound_factor(n, d):
    """ Compute the logarithm of the number of itemsets """
    binoms = []
    for i in range(1,d+1):
        binoms.append(log_binomial(n, i))
    return logsumexp(binoms) / math.log(2.0)


def pvalue_exact(support, size, supposed_freq):
    """ Compute p-value using the exact binomial distribution """
    #return binom.pmf(support, size, supposed_freq) + binom.sf(support, size, supposed_freq)
    return binom.logsf(support -1, size, supposed_freq)


def pvalue_chernoff(support, size, supposed_freq):
    """ Compute p-value using Chernoff bounds """
    mu = supposed_freq * size
    delta = (support - mu) / mu
    return mu * ( delta - ((1+ delta) * math.log(1 + delta)))
    #return - pow(support - mu, 2) / (3 * mu)


def pvalue(mode, support, size, supposed_freq):
    """ Compute the p-value using the selected method """
    if mode == "E":
        return pvalue_exact(support, size, supposed_freq)
    elif mode == "C":
        return pvalue_chernoff(support, size, supposed_freq)
    else: # NOT REACHED
        assert False


def main():
    # Verify arguments
    if len(sys.argv) != 6: 
        my.error_exit("Usage: {} MODE DELTA MINFREQ ORIGDS SAMPLERES\n".format(basename(sys.argv[0])))
    orig_ds = sys.argv[4]
    sample_res_filename = sys.argv[5]
    if not isfile(sample_res_filename):
        my.error_exit("{} does not exist, or is not a file\n".format(sample_res_filename))
    pvalue_mode = sys.argv[1].upper()
    if pvalue_mode != "C" and pvalue_mode != "E":
        my.error_exit("p-value mode must be either 'c' or 'e'. You passed {}\n".format(pvalue_mode))
    try:
        delta = float(sys.argv[2])
    except ValueError:
        my.error_exit("{} is not a number\n".format(sys.argv[2]))
    try:
        min_freq = float(sys.argv[3])
    except ValueError:
        my.error_exit("{} is not a number\n".format(sys.argv[3]))

    with open(sample_res_filename) as FILE:
        size_line = FILE.readline()
        try:
            size = int(size_line[1:-2])
        except ValueError:
            my.error_exit("Cannot compute size of the original dataset: {} is not a number\n".format(size_line[1:-2]))

    print("({})".format(size))
    supposed_freq = (math.ceil( size * min_freq) - 1) / size

    # Compute results
    sample_res = my.create_results(sample_res_filename, min_freq)

    # Compute critical value
    critical_value = math.log(delta) - (get_union_bound_factor(ds_stats[orig_ds]['numitems'], 2 * ds_stats[orig_ds]['maxlen']) / math.log2(math.e))

    # Compute TFIs according to this method
    survivors = []
    removed = 0
    for itemset in sample_res.keys():
        p_value = pvalue(pvalue_mode, sample_res[itemset], size, supposed_freq)
        if p_value <= critical_value:
            survivors.append((itemset, sample_res[itemset]))
        else:
            removed +=1

    sys.stderr.write("{} {} {} {}\n".format(supposed_freq, critical_value, len(sample_res), removed ))
    for (itemset, support) in sorted(survivors, key=itemgetter(1), reverse=True):
            print("{} ({})".format(" ".join(str(item) for item in itemset), support))


if __name__ == "__main__":
    main()

