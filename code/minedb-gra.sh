#! /bin/sh
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

. conf.sh

OS=`uname`
if [ ${OS} = "Linux" ]; then
    TAC="tac"
else
    TAC="tail -r"
fi

if [ $# -ne 3 ]; then
  echo "Usage: $0 MINSUPP DATASET OUTFILE" >&2
  exit 1
fi

TMPBASE=`basename $0`
TMPFILE=`mktemp --tmpdir=${RESULTS_BASE} ${TMPBASE}.XXXXXXXXXX`
if [ $? -ne 0 ]; then
  echo "$0: Can't create temp file, exiting..." >&2
  exit 1
fi

${SCRIPTS_BASE}/grahne/fim_all $2 $1 $3 && \
${PYTHON3} ${SCRIPTS_BASE}/external_sort.py -k "int(line[line.index('(')+1:line.index(')')])" -t ${RESULTS_BASE} $3 ${TMPFILE} && \
${TAC} ${TMPFILE} > $3
rm ${TMPFILE}

