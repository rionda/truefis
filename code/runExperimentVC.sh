#!/bin/sh
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


. ./conf.sh

if [ $# -ne 1 ]; then
	echo "USAGE: $0 var_file" >&2
	exit 1
fi

VAR_FILE=$1

if [ ! -r ${VAR_FILE} ]; then
	echo File '${VAR_FILE}' not readable. Exiting >&2
	exit 1
fi

. ./${VAR_FILE} 

DATASET_BASE=`echo ${DATASET} | rev | cut -d "." -f 2- | rev`

for FREQ in `echo ${FREQS}`; do
	RES_BASE="${DATASET_BASE}_d${DELTA}_t${FREQ}_vc"
	# Get the TFIs
	sh ${SCRIPTS_BASE}/getTrueFIsVC.sh ${USE_ADDIT_KNOWL} ${DELTA} ${FREQ} ${GAP} ${DATASET} > ${TFIS_BASE}/${RES_BASE}.res 2> ${LOGS_BASE}/${RES_BASE}_mine.log
	# Compare the TFIs
	EPSILON=`grep "e2=" ${LOGS_BASE}/${RES_BASE}_mine.log | tail -1 | cut -d "," -f 4 |cut -d "=" -f 2`
	${PYTHON3} ${SCRIPTS_BASE}/compareFIs.py 0.${FREQ} ${EPSILON} ${TFIS_BASE}/${RES_BASE}.res ${ORIG_RES} > /dev/null 2> ${LOGS_BASE}/${RES_BASE}_comp.log 
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

