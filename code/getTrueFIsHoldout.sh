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

. ./conf.sh

DO_FILTER=$1
# We keep only the decimal part
DELTA=`echo $2 | cut -d "." -f 2`
MIN_FREQ=`echo $3 | cut -d "." -f 2`
MODE=`echo $4 | cut -d "." -f 2`

DATASET=$5
echo -n "Getting dataset stats..." >&2
DS_STATS=`grep ${DATASET} ${SCRIPTS_BASE}/datasetsinfo.py`
if [ "${DS_STATS:-empty}" = "empty" ]; then
	DS_STATS=`${PYTHON3} ${SCRIPTS_BASE}/getDatasetInfo.py ${DATASET}`
fi
BASEDATASETNAME=`echo ${DS_STATS} | cut -d ":" -f 1 | cut -d "'" -f 2 | rev | cut -d . -f 2- |rev`
BASE_STATS=`echo ${DS_STATS} | cut -d ":" -f 2- | rev | cut -d "," -f 2- | rev`
SIZE_COMMAND="stats=${BASE_STATS}; print(stats['size'])"
SIZE=`${PYTHON3} -c "${SIZE_COMMAND}"`
echo "done" >&2

# Only split the dataset and mine the two parts if there results are not yet
# available for a frequency less than ${MIN_FREQ}.
echo -n "Getting FIs..." >&2
for RES in `ls ${RESULTS_BASE}/${BASEDATASETNAME}_t*_eval.res 2> /dev/null || echo ""`; do
	FREQ=`basename ${RES} | rev | cut -d "_" -f 2 | rev| cut -d "t" -f 2`
	# Floating point comparison
	DIFFERENCE=`echo 0.${MIN_FREQ} - 0.${FREQ} | bc | cut -d "." -f 1`
	if [ ${DIFFERENCE:-empty} != "-" ]; then
		echo -n "found results for freq=${FREQ}..." >&2
		RESULTS_FILE_BASE="${BASEDATASETNAME}_t${FREQ}"
		EVAL_RES="${RESULTS_FILE_BASE}_eval.res"
		EXPL_RES="${RESULTS_FILE_BASE}_expl.res"
		break
	fi
done

if [ ${EVAL_RES:-empty} = "empty" ]; then
	echo -n "must split the dataset and mine the parts..." >&2
	RESULTS_FILE=${RESULTS_BASE}/${BASEDATASETNAME}_t${MIN_FREQ}.res
	if [ -r ${DATASET} ]; then
		DS=${DATASET}
	else
		DS="${SAMPLES_BASE}/${DATASET}"
	fi
	# Only split the dataset if we actually need to
	if [ ! -r ${SAMPLES_BASE}/${BASEDATASETNAME}_expl.dat -o ! -r ${SAMPLES_BASE}/${BASEDATASETNAME}_eval.dat ]; then
		${PYTHON3} ${SCRIPTS_BASE}/splitDataset.py ${SIZE} ${DS} ${SAMPLES_BASE}/${BASEDATASETNAME}_expl.dat ${SAMPLES_BASE}/${BASEDATASETNAME}_eval.dat
	fi
	SUPP=`echo "scale=scale(0.${MIN_FREQ}); supp=${SIZE} * 0.${MIN_FREQ}; print supp" | bc | cut -d. -f 1`
	RESULTS_FILE_BASE="${BASEDATASETNAME}_t${FREQ}"
	EVAL_RES="${RESULTS_FILE_BASE}_eval.res"
	EXPL_RES="${RESULTS_FILE_BASE}_expl.res"
	sh ${SCRIPTS_BASE}/minedb-gra.sh ${SUPP} ${SAMPLES_BASE}/${BASEDATASETNAME}_expl.dat ${RESULTS_BASE}/${EXPL_RES} > /dev/null
	sh ${SCRIPTS_BASE}/minedb-gra.sh ${SUPP} ${SAMPLES_BASE}/${BASEDATASETNAME}_eval.dat ${RESULTS_BASE}/${EVAL_RES} > /dev/null
fi
echo "done" >&2

# Compute the True FIs
echo "Getting TFIs..." >&2
${PYTHON3} ${SCRIPTS_BASE}/getTrueFIsHoldout.py ${DO_FILTER} 0.${DELTA} 0.${MIN_FREQ} ${MODE} ${RESULTS_BASE}/${EXPL_RES} ${RESULTS_BASE}/${EVAL_RES}

