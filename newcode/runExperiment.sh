#!/bin/sh
# Finding the True Frequent Itemsets
#
# Copyright 2015 Matteo Riondato <matteo@cs.brown.edu>
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


. ./conf.sh

if [ $# -ne 2 ]; then
	echo "USAGE: $0 {binom|holdout|vc|vcholdout} var_file" >&2
	exit 1
fi

ALGO=$1

if [ ${ALGO} != "binom" -a ${ALGO} != "holdout" -a ${ALGO} != "vc" -a ${ALGO} != "vcholdout" ]; then
    echo "Algorithm '${ALGO}' not recognized. Must be binom, holdout, vc, or vcholdout" >&2
	exit 1
fi

VAR_FILE=$2

if [ ! -r ${VAR_FILE} ]; then
	echo File '${VAR_FILE}' not readable. Exiting >&2
	exit 1
fi

. ./${VAR_FILE}

DATASET_BASE=`echo ${DATASET} | rev | cut -d "." -f 2- | rev`

for FREQ in `echo ${FREQS}`; do
	echo $FREQ
	RES_BASE="${DATASET_BASE}_d${DELTA}_t${FREQ}_${ALGO}"

	# Get the TFIs
    if [ ${ALGO} = "binom" ]; then
        ${SCRIPTS_BASE}/getTrueFIsBinom -v -s ${DS_SIZE} -i ${ITEMS} 0.${DELTA} 0.${FREQ} ${RESULTS_BASE}/${DATASET_BASE}_t${MIN_FREQ}.res ${SAMPLES_BASE}/${DATASET} > ${TFIS_BASE}/${RES_BASE}.res 2> ${LOGS_BASE}/${RES_BASE}_mine.log
		EPSILON=`grep "epsilon=" ${LOGS_BASE}/${RES_BASE}_mine.log | tail -1 | cut -d "," -f 2 | cut -d "=" -f 2`
    elif [ ${ALGO} = "holdout" ]; then
        ${SCRIPTS_BASE}/getTrueFIsHoldout -v -s ${HOLDOUT_SIZE} 0.${DELTA} 0.${FREQ} ${RESULTS_BASE}/${DATASET_BASE}_expl_t${MIN_FREQ}.res ${SAMPLES_BASE}/${DATASET_BASE}_expl.dat ${RESULTS_BASE}/${DATASET_BASE}_eval_t${MIN_FREQ}.res ${SAMPLES_BASE}/${DATASET_BASE}_eval.dat > ${TFIS_BASE}/${RES_BASE}.res 2> ${LOGS_BASE}/${RES_BASE}_mine.log
		EPSILON=`grep "epsilon=" ${LOGS_BASE}/${RES_BASE}_mine.log | tail -1 | cut -d "," -f 3 | cut -d "=" -f 2`
    elif [ ${ALGO} = "vc" ]; then
		RES_BASE="${RES_BASE}_${BOUND_METHOD_FIRST}_${COUNT_METHOD_SECOND}_${BOUND_METHOD_SECOND}"
        ${SCRIPTS_BASE}/getTrueFIsVC -v -s ${DS_SIZE} 0.${DELTA} 0.${FREQ} ${BOUND_METHOD_FIRST} ${COUNT_METHOD_SECOND} ${BOUND_METHOD_SECOND} ${RESULTS_BASE}/${DATASET_BASE}_t${MIN_FREQ}.res ${SAMPLES_BASE}/${DATASET} > ${TFIS_BASE}/${RES_BASE}.res 2> ${LOGS_BASE}/${RES_BASE}_mine.log
        EPSILON=`grep "e2=" ${LOGS_BASE}/${RES_BASE}_mine.log | tail -1 | cut -d "," -f 3 |cut -d "=" -f 2`
	elif [ ${ALGO} = "vcholdout" ]; then
		RES_BASE="${RES_BASE}_${BOUND_METHOD_FIRST}_${COUNT_METHOD_SECOND}_${BOUND_METHOD_SECOND}"
        ${SCRIPTS_BASE}/getTrueFIsVCHoldout -v -s ${HOLDOUT_SIZE} 0.${DELTA} 0.${FREQ} ${BOUND_METHOD_FIRST} ${COUNT_METHOD_SECOND} ${BOUND_METHOD_SECOND} ${RESULTS_BASE}/${DATASET_BASE}_expl_t${MIN_FREQ}.res ${SAMPLES_BASE}/${DATASET_BASE}_expl.dat ${RESULTS_BASE}/${DATASET_BASE}_eval_t${MIN_FREQ}.res ${SAMPLES_BASE}/${DATASET_BASE}_eval.dat > ${TFIS_BASE}/${RES_BASE}.res 2> ${LOGS_BASE}/${RES_BASE}_mine.log
        EPSILON=`grep "eval_epsilon=" ${LOGS_BASE}/${RES_BASE}_mine.log | tail -1 | cut -d "," -f 4 |cut -d "=" -f 2`
    else # unreached
        echo "You should not be here!" >&2
    fi

	# Compare the TFIs
	${SCRIPTS_BASE}/sortFIs ${TFIS_BASE}/${RES_BASE}.res > ${TFIS_BASE}/${RES_BASE}.res.sort
	mv ${TFIS_BASE}/${RES_BASE}.res.sort ${TFIS_BASE}/${RES_BASE}.res
	${SCRIPTS_BASE}/compareFIs 0.${FREQ} ${TFIS_BASE}/${RES_BASE}.res ${ORIG_RES} > /dev/null 2> ${LOGS_BASE}/${RES_BASE}_comp.log
	# Create 'global log with CSV values
	FIRST_CSV=`tail -2 ${LOGS_BASE}/${RES_BASE}_mine.log | head -1`
	SECOND_CSV=`tail -2 ${LOGS_BASE}/${RES_BASE}_comp.log | head -1`
	echo -n ${FIRST_CSV} > ${LOGS_BASE}/${RES_BASE}_glob.csv
	echo -n "," >> ${LOGS_BASE}/${RES_BASE}_glob.csv
	echo ${SECOND_CSV} >> ${LOGS_BASE}/${RES_BASE}_glob.csv
	FIRST_CSV=`tail -1 ${LOGS_BASE}/${RES_BASE}_mine.log`
	SECOND_CSV=`tail -1 ${LOGS_BASE}/${RES_BASE}_comp.log`
	echo -n ${FIRST_CSV} >> ${LOGS_BASE}/${RES_BASE}_glob.csv
	echo -n "," >> ${LOGS_BASE}/${RES_BASE}_glob.csv
	echo ${SECOND_CSV} >> ${LOGS_BASE}/${RES_BASE}_glob.csv
done
