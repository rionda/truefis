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
from scipy.stats import binom as scipy_binom
from scipy.misc import logsumexp as scipy_logsumexp


def error_exit(msg):
    """Print message msg to stderr and exit."""
    sys.stderr.write(msg)
    sys.exit(1)


def get_closed_itemsets(itemsets):
    """Compute the closed itemsets.
    
    Return the set of closed itemsets in 'itemsets', which is a dictionary
    whose keys are itemsets (frozensets) and whose values are the frequencies.
    Return a similar dict.

    """
    closed_itemsets = dict()
    border = set()
    itemsets_sorted_by_size = sorted(itemsets, key=len)
    for itemset in itemsets_sorted_by_size:
        to_remove = set()
        to_remove_border = set()
        for closed in border:
            if itemset > closed:
                if itemsets[closed] == itemsets[itemset]:
                    to_remove.add(closed)
                to_remove_border.add(closed)
        border -= to_remove_border
        border.add(itemset)
        for key in to_remove:
            del closed_itemsets[key]
        closed_itemsets[itemset] = itemsets[itemset]
        
    #check_closed_itemsets(closed_itemsets)
    return closed_itemsets


def check_closed_itemsets(closed_itemsets):
    """ Check that closed_itemsets is a collection of closed itemsets. """
    for itemset1 in closed_itemsets:
        for itemset2 in closed_itemsets:
            if itemset1 < itemset2:
                assert closed_itemsets[itemset1] > closed_itemsets[itemset2]


def get_maximal_itemsets(itemsets):
    """Compute the maximal itemsets.

    Return the set of maximal itemsets in 'itemsets', which is a dictionary
    whose keys are itemsets (frozensets) and whose values are the frequencies.
    Return a similar dict.

    """
    maximal_itemsets = dict()
    maximal_itemsets_list = []
    itemsets_revsorted_by_size = sorted(itemsets.keys(), key=len, reverse=True)
    for itemset in itemsets_revsorted_by_size:
        to_add = True
        for maximal in maximal_itemsets_list:
            if itemset < maximal:
                to_add = False
                break
        if to_add:
            maximal_itemsets[itemset] = itemsets[itemset]
            maximal_itemsets_list.append(itemset)
    return maximal_itemsets


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
            size_str = size_line.split("(")[1].split(")")[0]
        except IndexError:
            error_exit("Cannot compute size of the original dataset: '{}' is not in the recognized format\n".format(size_line))
        try:
            size = int(size_str)
        except ValueError:
            error_exit("Cannot compute size of the original dataset: '{}' is not a number\n".format(size_line.split("(")[1].split(")")[0]))
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


def print_itemset(itemset, frequency, ds_size=1):
    """ Print an itemset and its support (frequency * ds_size).
    
    Uses the 'standard' FIMI format: 'item1 item2 item3 (support)'"""
    print("{} ({})".format(" ".join(str(item) for item in itemset),
        int(frequency * ds_size)))


def print_itemsets(itemsets, ds_size=1):
    """ Print a collection of itemsets with their support. 

    The first line to be printed is the size of the dataset in parentheses,
    then come the itemsets, in reverse sorted order by support, printed in the
    'standard' FIMI format: 'item1 item2 item3 (support)'."""

    print(" ({})".format(ds_size)) # The space at the beginning makes sense.

    for itemset in sorted(itemsets, key=lambda x: itemsets[x], reverse=True):
        print_itemset(itemset, itemsets[itemset], ds_size)

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
    """ Compute the natural logarithm of the number of itemsets """
    binoms = []
    for i in range(1,d+1):
        binoms.append(log_binomial(n, i))
    return scipy_logsumexp(binoms)


def pvalue_exact(support, size, supposed_freq):
    """ Compute p-value using the exact binomial distribution. 
    
    We work in the log space, so this is the logarithm of the real p-value.
    """
    return scipy_binom.logsf(support -1, size, supposed_freq)


def pvalue_chernoff(support, size, supposed_freq):
    """ Compute p-value using Chernoff bounds.

    We work in the log space, so this is the logarithm of the real p-value.
    
    We use Equation 4.1 from Thm. 4.4 in Mitzenmacher and Upfal, 'Probability
    and Computing", Cambridge University Press, 2005.
    """
    mu = supposed_freq * size
    delta = (support - mu) / mu
    one_plus_delta = support / mu
    return mu * ( delta - ((one_plus_delta) * math.log(one_plus_delta)))


def pvalue(mode, support, size, supposed_freq):
    """ Compute the p-value using the selected method.
    
    We work in the log space, so this is the logarithm of the real p-value.
    """
    if mode == "E":
        return pvalue_exact(support, size, supposed_freq)
    elif mode == "C":
        return pvalue_chernoff(support, size, supposed_freq)
    else: # NOT REACHED
        assert False

