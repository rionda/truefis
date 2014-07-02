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
from datasetsinfo import ds_stats


def error_exit(msg):
    sys.stderr.write(msg)
    sys.exit(1)


def get_eps_vc_dim(delta, ds_size, vc_dim, max_freq=1.0, c=0.5):
    """Return the epsilon computed using vc_dim as bound to the VC-dimension,
    given a 'sample' of size ds_size, where the maximum frequency of an item is
    max_freq, and a confidence parameter delta. The parameter 'c' is the
    universal constant, which we set to 0.5 by default as suggested by Loffler
    and Phillips (see paper)"""
    return math.sqrt( (c / ds_size) * ( vc_dim + math.log(1 / delta)))


def get_eps_shattercoeff_bound(delta, ds_size, bound, max_freq=1.0):
    """Return the epsilon computed using 'bound' as bound to the shatter
    coefficient, given a 'sample' of size ds_size', where the maximum frequency
    of an item is max_freq, and a confidence paramter delta."""
    return 2 * math.sqrt( max_freq *  2 * bound / ds_size) + \
            math.sqrt((2 * math.log(2 / delta)) / ds_size)


def get_eps_emp_vc_dim(delta, ds_size, emp_vc_dim, max_freq=1.0):
    """Return the epsilon computed using emp_vc_dim as bound to the *empirical*
    VC-dimension, given a 'sample' of size ds_size, where the maximum frequency
    of an item is max_freq, and a confidence parameter delta."""
    return get_eps_shattercoeff_bound(delta, ds_size, emp_vc_dim *
            math.log(ds_size + 1), max_freq)


def epsilons(delta, ds_size, vc_dim, emp_vc_dim, max_freq=1.0):
    """Return a tuple containing the epsilon w.r.t. the VC-dimension, the
    epsilon w.r.t. the empirical VC-dimension, and a string that points out
    which one is the smallest (or 'equal' if they are equal)."""
    eps_vc_dim = get_eps_vc_dim(delta, ds_size, vc_dim, max_freq)
    eps_emp_vc_dim = get_eps_emp_vc_dim(delta, ds_size, emp_vc_dim, max_freq)

    if eps_vc_dim < eps_emp_vc_dim:
        returned = "vc_dim"
    elif eps_vc_dim > eps_emp_vc_dim:
        returned = "emp_vc_dim"
    else:
        returned = "equal"
    return (eps_vc_dim, eps_emp_vc_dim, returned)


def epsilon_dataset(delta, dataset, use_additional_knowledge=False):
    """ Call epsilons() filling in the appropriate values for the parameters
    depending whether to use additional knowledge or not. See below for type
    descriptions.
    """
    
    assert use_additional_knowledge >= 0 and use_additional_knowledge < 2
    assert delta > 0 and delta < 1
    assert dataset in ds_stats
    
    if use_additional_knowledge:
        # make no assumption on the generative process. VC-dimension is number
        # of items - 1.
        (eps_vc_dim, eps_emp_vc_dim, returned) = epsilons(delta,
                ds_stats[dataset]['size'], ds_stats[dataset]['numitems'] -1,
                ds_stats[dataset]['dindex'], ds_stats[dataset]['maxsupp'] /
                ds_stats[dataset]['size'])
    else: 
        # incorporate available information about the unknown probability
        # distribution, more precisely assuming that it cannot generate
        # transactions longer than twice the longest transactions available in
        # the dataset (using this quantity as bound to the VC-dimension).
        (eps_vc_dim, eps_emp_vc_dim, returned) = epsilons(delta,
                ds_stats[dataset]['size'], 2*(ds_stats[dataset]['maxlen']) -1, 
                ds_stats[dataset]['dindex'], ds_stats[dataset]['maxsupp'] /
                ds_stats[dataset]['size'])
 
    return (eps_vc_dim, eps_emp_vc_dim, returned)


if __name__ == "__main__":
    """When invoked as standalone, compute the epsilons and print them."""
    if len(sys.argv) != 4:
        error_exit("Usage: {} {{0|1}} delta dataset.dat (no path, must be in datasetsinfo.py)\n".format(sys.argv[0]))
    try:
        phase = int(sys.argv[1])
    except ValueError:
        error_exit("{} is not an integer\n".format(sys.argv[1]))
    try:
        delta = float(sys.argv[2])
    except ValueError:
        error_exit("{} is not an integer\n".format(sys.argv[2]))

    (eps_vc_dim, eps_emp_vc_dim, returned) = epsilon_dataset(phase, delta, sys.argv[3])

    print("{} {}".format(eps_vc_dim, eps_emp_vc_dim))
    print("{}\t{}".format(min(eps_vc_dim, eps_emp_vc_dim), returned))
