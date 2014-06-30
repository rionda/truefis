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
from datasetsinfo import dsStats
from timeit import Timer

sampleSize = 0
populationSize = 0
largeFileName = ""
sampleFileName = ""

def errorExit(msg):
    sys.stderr.write(msg)
    sys.exit(1)


def createSample():
    # Compute indexes of sample lines
    _random, _int = random.random, int  # speed hack XXX really?
    sampleLines = sorted([_int(_random() * populationSize) for i in itertools.repeat(None, sampleSize)])

    indexSample = 0
    indexLines = 0
    with open(largeFileName, "rt") as largeFILE, open(sampleFileName, "wt") as sampleFILE:
        while indexSample < sampleSize:
            while indexLines < sampleLines[indexSample]:
                line = largeFILE.readline()
                indexLines = indexLines + 1
            line = largeFILE.readline()
            indexLines = indexLines + 1
            indexSample = indexSample + 1
            sampleFILE.write(line)

            # Handle the case when a line is sampled multiple times.
            while indexSample < sampleSize and sampleLines[indexSample] == sampleLines[indexSample - 1]:
                sampleFILE.write(line)
                indexSample = indexSample + 1
    return(0)


def main():
    global sampleSize 
    global populationSize
    global largeFileName
    global sampleFileName
    # Verify arguments
    if len(sys.argv) != 4: 
        errorExit("Usage: {} SAMPLESIZE SAMPLEFILE DATASETFILE\n".format(os.path.basename(sys.argv[0])))
    sampleFileName = sys.argv[2]
    largeFileName = sys.argv[3]
    if not os.path.isfile(largeFileName):
        errorExit("{} does not exist, or is not a file\n".format(largeFileName))
    try:
        sampleSize = int(sys.argv[1])
    except ValueError:
        errorExit("{} is not a number\n".format(sys.argv[1]))

    baseLargeFileName = os.path.basename(largeFileName)
    populationSize = dsStats[baseLargeFileName]['size']

    random.seed()

    t = Timer("createSample()", "from __main__ import createSample")
    sys.stderr.write("Creating the sample took: {} ms \n".format(t.timeit(1) * 1000))


if __name__ == "__main__":
    main()

