#! /bin/sh
#
# Call Grahne and Zhu's implementation for Frequent Itemset mining and sort the
# results. Print runtime of mining to stderr on exit
#
#   Copyright 2014 Matteo Riondato <matteo@cs.brown.edu>
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

if [ $# -ne 3 ]; then
    echo "$0: extract the Frequent Itemsets w.r.t. theta from dataset and write them to outfile" >&2
    echo "USAGE: $0 theta dataset outfile" >&2
    exit 1
fi

MIN_FREQ=$1
DS=`readlink -f $2` 
touch $3
OUT_FILE=`readlink -f $3`

OS=`uname`
if [ ${OS} = "Linux" ]; then
	TAC="tac" # tac is a GNU utility
else
	TAC="less -r" # less -r works on Mac OS X
fi

SIZE=`grep -c -v '^#' ${DS}`

THRESHOLD=`awk -v thres="${MIN_FREQ}" -v size="${SIZE}" '
  function ceiling(x) {return x%1 ? int(x)+1 : x}
  BEGIN{ print ceiling(thres * size) }'`

MY_TMPDIR="./"
TMP_BASE=`basename $0 | rev | cut -d "." -f 2- | rev`
TMP_FILE_LOCAL=`mktemp --tmpdir=${MY_TMPDIR} ${TMP_BASE}-XXXXXX`
TMP_FILE=`readlink -f ${TMP_FILE_LOCAL}`
if [ $? -ne 0 ]; then
    echo "$0: Can't create temp file, exiting..." 1>&2
    exit 1
fi

time -p ./grahne/fim_all ${DS} ${THRESHOLD} ${OUT_FILE} > /dev/null 2> ${TMP_FILE} || (echo "fim_all failed" >&2; exit 1)
SEC_RUNTIME=`head -2 ${TMP_FILE} |tail -1 | cut -d " " -f 2`
RUNTIME=`echo "scale=0; (${SEC_RUNTIME} * 1000)/1" | bc`

# External sorting is great if we have really many frequent itemsets,
# but it has a little bug if there are itemsets which appear in all
# transactions, as they may come before the "length" line in the
# sorted order
#python3 external_sort.py -k "int(line[line.index('(')+1:line.index(')')])" -t ${MY_TMPDIR} ${OUT_FILE} ${TMP_FILE} && \
#${TAC} ${TMP_FILE} > ${OUT_FILE}
#rm ${TMP_FILE}
./sortFIs ${OUT_FILE} > ${TMP_FILE} && mv ${TMP_FILE} ${OUT_FILE} && echo "theta,mining_time" 1>&2 && echo "$1,${RUNTIME}" 1>&2

