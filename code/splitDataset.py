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

import os.path, random, sys
import utils


def main():
    """ Partition a dataset in two equal parts. """
    # Verify arguments
    if len(sys.argv) != 5: 
        utils.error_exit("Usage: {} dataset_size dataset_file expl_file eval_file\n".format(os.path.basename(sys.argv[0])))
    dataset = sys.argv[2]
    expl = sys.argv[3]
    eval = sys.argv[4]
    try:
       dataset_size = int(sys.argv[1])
    except ValueError:
        utils.error_exit("{} is not a number\n".format(sys.argv[1]))

    random.seed()
    expl_lines = frozenset(random.sample(range(dataset_size), dataset_size // 2))

    with open(dataset, "rt") as largeFILE, open(expl, "wt") as explFILE, open(eval, "wt") as evalFILE:
        index = 0
        for line in largeFILE:
            if index in expl_lines:
                explFILE.write(line)
            else:
                evalFILE.write(line)
            index += 1


if __name__ == "__main__":
    main()

