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

import itertools, os, random, sys
from timeit import Timer
import getDatasetInfo, util

sample_size = 0
population_size = 0
dataset = ""


def create_sample():
    # Compute indexes of sample lines
    _random, _int = random.random, int  # speed hack XXX really?
    sample_lines = sorted([_int(_random() * population_size) for i in itertools.repeat(None, sample_size)])

    index_sample = 0
    index_lines = 0
    with open(dataset, "rt") as largeFILE:
        while index_sample < sample_size:
            while index_lines < sample_lines[index_sample]:
                line = largeFILE.readline()
                index_lines = index_lines + 1
            line = largeFILE.readline()
            index_lines = index_lines + 1
            index_sample = index_sample + 1
            sys.stdout.write(line)

            # Handle the case when a line is sampled multiple times.
            while index_sample < sample_size and sample_lines[index_sample] == sample_lines[index_sample - 1]:
                sys.stdout.write(line)
                index_sample = index_sample + 1


def main():
    global sample_size 
    global population_size
    global dataset
    # Verify arguments
    if len(sys.argv) != 3: 
        util.error_exit("Usage: {} samplesize dataset\n".format(os.path.basename(sys.argv[0])))
    dataset = sys.argv[2]
    try:
        sample_size = int(sys.argv[1])
    except ValueError:
        util.error_exit("{} is not a number\n".format(sys.argv[1]))

    ds_stats = getDatasetInfo.get_stats(dataset)
    population_size = ds_stats['size']

    random.seed()

    t = Timer("create_sample()", "from __main__ import create_sample")
    sys.stderr.write("Creating the sample took: {} ms \n".format(t.timeit(1) * 1000))


if __name__ == "__main__":
    main()

