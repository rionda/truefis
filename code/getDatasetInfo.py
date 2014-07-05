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
import datasetsinfo, utils


def compute_ds_stats(dataset):
    """Compute various stats about the dataset. 
    
    Returns a dict where the keys are stats names and values are the stats
    values. The keys of the dict and their meanings are the following:
    'dindex': upper bound to the d-index
    'size' : the number of transactions in the dataset
    'numitems': the number of items in the dataset
    'maxlen': the maximum length of a transaction in the dataset
    'maxsupp': the maximum support of an item in the dataset
    'lengths': a dict whose keys are integers and values are the number of
    transactions of that length in the dataset
    'items': a set of the items in the dataset

    Example:
     {'dindex': 1, 'lengths': {1: 5}, 'items': {1, 2, 3, 4, 5}, 'maxsupp': 4, 'size': 6, 'numitems': 5, 'maxlen': 1}

    """
    with open(dataset, 'rt') as DS:
        max_len = 0
        item_supp = dict()
        items = set() 
        T = [frozenset(map(int,DS.readline().split()))]
        size = 1
        d_index = 1
        transaction_lengths = dict()
        transaction_lengths[len(T[0])] = 1
        for item in T[0]:
            item_supp[item] = 1

        for line in DS:
            t = frozenset(map(int,line.split()))
            size += 1
            line_length = len(t)
            if line_length in transaction_lengths:
                transaction_lengths[line_length] += 1
            else:
                transaction_lengths[line_length] = 1
            for item in t:
                if item in item_supp:
                    item_supp[item] += 1
                else:
                    item_supp[item] = 1

            if len(t) > d_index:
                process = True
                for p in T:
                    if t.issubset(p):
                        process = False
                        break
                if not process:
                    continue
                T.append(t)
                T.sort(key=len, reverse=True)
                d_index = 0
                for p in T:
                    if len(p) <= d_index:
                        break
                    d_index += 1
                T = T[:d_index]

            if len(t) > max_len:
                max_len = len(t)
            items = items.union(t)

    return {'size': size, 'dindex': d_index, 'maxlen': max_len, 'maxsupp':
            max(item_supp.values()), 'numitems': len(items), 'lengths':
            transaction_lengths, 'items': items}


def get_ds_stats(dataset, force_compute = False):
    """ Return a dict containing the statistics about the dataset.
    
    Look up 'dataset' in datasetsinfo.ds_stat. If present, return that dict,
    otherwise, compute the stats.

    See the comment at the beginning of compute_ds_stats() for info about the
    dict."""

    if dataset in datasetsinfo.ds_stats and force_compute == False:
        return datasetsinfo.ds_stats[dataset]
    else:
        if not os.path.isfile(dataset):
            utils.error_exit("{} not found in datasetsinfo.py and does not exist or is not a file\n".format(dataset))
        return compute_ds_stats(dataset)


def main():
    if len(sys.argv) != 2:
        utils.error_exit("Usage: {} dataset\n".format(os.path.basename(sys.argv[0])))

    stats = get_ds_stats(sys.argv[1])
    print("'{}': {},".format(os.path.basename(sys.argv[1]), stats))


if __name__ == '__main__':
    main()

